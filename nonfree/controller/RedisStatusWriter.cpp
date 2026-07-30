/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#include "RedisStatusWriter.hpp"

#include "../../node/Metrics.hpp"
#include "../../osdep/OSUtils.hpp"
#include "CtlUtil.hpp"

#include <nlohmann/json.hpp>
#include <set>

namespace ZeroTier {

RedisStatusWriter::RedisStatusWriter(std::shared_ptr<sw::redis::Redis> redis, std::string controller_id)
	: _controller_id(controller_id)
	, _redis(redis)
	, _mode(REDIS_MODE_STANDALONE)
{
}

RedisStatusWriter::RedisStatusWriter(std::shared_ptr<sw::redis::RedisCluster> cluster, std::string controller_id)
	: _controller_id(controller_id)
	, _cluster(cluster)
	, _mode(REDIS_MODE_CLUSTER)
{
}

RedisStatusWriter::~RedisStatusWriter()
{
	writePending();
}

void RedisStatusWriter::updateNodeStatus(
	const std::string& network_id,
	const std::string& node_id,
	const std::string& os,
	const std::string& arch,
	const std::string& version,
	const InetAddress& address,
	int64_t last_seen,
	const std::string& /* frontend unused */)
{
	std::lock_guard<std::mutex> l(_lock);
	_pending.push_back({ network_id, node_id, os, arch, version, address, last_seen, "" });
}

size_t RedisStatusWriter::queueLength() const
{
	std::lock_guard<std::mutex> l(_lock);
	return _pending.size();
}

void RedisStatusWriter::writePending()
{
	std::vector<PendingStatusEntry> toWrite;
	{
		std::lock_guard<std::mutex> l(_lock);
		toWrite.swap(_pending);
	}
	if (toWrite.empty()) {
		return;
	}

	try {
		if (_mode == REDIS_MODE_STANDALONE) {
			auto tx = _redis->transaction(true, false);
			_doWritePending(tx, toWrite);
		}
		else if (_mode == REDIS_MODE_CLUSTER) {
			auto tx = _cluster->transaction(_controller_id, true, false);
			_doWritePending(tx, toWrite);
		}
	}
	catch (const sw::redis::Error& e) {
		ZTC_LOG("Error writing to Redis: %s\n", e.what());
		// Don't drop the batch on a transient failure — re-queue it for the next cycle.
		requeuePendingStatus(_pending, _lock, std::move(toWrite), "RedisStatusWriter");
	}
}

void RedisStatusWriter::_doWritePending(sw::redis::Transaction& tx, const std::vector<PendingStatusEntry>& toWrite)
{
	if (toWrite.empty()) {
		return;
	}

	std::set<std::string> networksUpdated;
	uint64_t updateCount = 0;
	for (const auto& entry : toWrite) {
		char iptmp[64] = { 0 };
		std::string ipAddr = entry.address.toIpString(iptmp);
		std::unordered_map<std::string, std::string> record = {
			{ "id", entry.node_id }, { "address", ipAddr },	 { "last_updated", std::to_string(entry.last_seen) },
			{ "os", entry.os },		 { "arch", entry.arch }, { "version", entry.version }
		};

		tx.zadd("nodes-online:{" + _controller_id + "}", entry.node_id, entry.last_seen)
			.zadd("nodes-online2:{" + _controller_id + "}", entry.network_id + "-" + entry.node_id, entry.last_seen)
			.zadd("network-nodes-online:{" + _controller_id + "}:" + entry.network_id, entry.node_id, entry.last_seen)
			.zadd("active-networks:{" + _controller_id + "}", entry.network_id, entry.last_seen)
			.sadd("network-nodes-all:{" + _controller_id + "}:" + entry.network_id, entry.node_id)
			.hmset(
				"member:{" + _controller_id + "}:" + entry.network_id + ":" + entry.node_id, record.begin(),
				record.end());
		networksUpdated.insert(entry.network_id);
		++updateCount;
		Metrics::redis_node_checkin++;
	}

	// expire records from all-nodes and network-nodes member list
	uint64_t expireOld = OSUtils::now() - 300000;

	tx.zremrangebyscore(
		"nodes-online:{" + _controller_id + "}",
		sw::redis::RightBoundedInterval<double>(expireOld, sw::redis::BoundType::LEFT_OPEN));
	tx.zremrangebyscore(
		"nodes-online2:{" + _controller_id + "}",
		sw::redis::RightBoundedInterval<double>(expireOld, sw::redis::BoundType::LEFT_OPEN));
	tx.zremrangebyscore(
		"active-networks:{" + _controller_id + "}",
		sw::redis::RightBoundedInterval<double>(expireOld, sw::redis::BoundType::LEFT_OPEN));

	for (const auto& nwid : networksUpdated) {
		tx.zremrangebyscore(
			"network-nodes-online:{" + _controller_id + "}:" + nwid,
			sw::redis::RightBoundedInterval<double>(expireOld, sw::redis::BoundType::LEFT_OPEN));
	}

	ZTC_LOG("Updated online status of %llu members\n", (unsigned long long)updateCount);
	tx.exec();
}

}	// namespace ZeroTier
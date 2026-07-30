/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifdef ZT_CONTROLLER_USE_LIBPQ

#include "RedisListener.hpp"

#include "../../node/Metrics.hpp"
#include "CtlUtil.hpp"
#include "nlohmann/json.hpp"
#include "opentelemetry/trace/provider.h"

#include <memory>
#include <string>
#include <vector>

namespace ZeroTier {

using Attrs = std::vector<std::pair<std::string, std::string> >;
using Item = std::pair<std::string, Attrs>;
using ItemStream = std::vector<Item>;

RedisListener::RedisListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis)
	: _controller_id(controller_id)
	, _redis(redis)
	, _is_cluster(false)
	, _run(false)
{
}

RedisListener::RedisListener(std::string controller_id, std::shared_ptr<sw::redis::RedisCluster> cluster)
	: _controller_id(controller_id)
	, _cluster(cluster)
	, _is_cluster(true)
	, _run(false)
{
}

void RedisListener::stop()
{
	_run = false;
	if (_listenThread.joinable()) {
		_listenThread.join();
	}
}

RedisListener::~RedisListener()
{
	// Safety net: each derived destructor should already have called stop() so the
	// listen thread is joined while the derived object is still intact. Idempotent.
	stop();
}

RedisNetworkListener::RedisNetworkListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis, DB* db)
	: RedisListener(controller_id, redis)
	, _db(db)
{
	// Additional initialization for network listener if needed
}

RedisNetworkListener::RedisNetworkListener(
	std::string controller_id,
	std::shared_ptr<sw::redis::RedisCluster> cluster,
	DB* db)
	: RedisListener(controller_id, cluster)
	, _db(db)
{
	// Additional initialization for network listener if needed
}

RedisNetworkListener::~RedisNetworkListener()
{
	stop();	  // join the listen thread before this object's members are destroyed
}

void RedisNetworkListener::listen()
{
	std::string key = "network-stream:{" + _controller_id + "}";
	std::string lastID = "0";
	while (_run) {
		auto provider = opentelemetry::trace::Provider::GetTracerProvider();
		auto tracer = provider->GetTracer("RedisNetworkListener");
		auto span = tracer->StartSpan("RedisNetworkListener::listen");
		auto scope = tracer->WithActiveSpan(span);

		try {
			nlohmann::json tmp;
			std::unordered_map<std::string, ItemStream> result;
			if (_is_cluster) {
				_cluster->xread(key, lastID, std::chrono::seconds(1), 0, std::inserter(result, result.end()));
			}
			else {
				_redis->xread(key, lastID, std::chrono::seconds(1), 0, std::inserter(result, result.end()));
			}

			if (! result.empty()) {
				for (auto element : result) {
					for (auto rec : element.second) {
						std::string id = rec.first;
						auto attrs = rec.second;

						for (auto a : attrs) {
							try {
								tmp = nlohmann::json::parse(a.second);
								nlohmann::json& ov = tmp["old_val"];
								nlohmann::json& nv = tmp["new_val"];
								nlohmann::json oldConfig, newConfig;
								if (ov.is_object())
									oldConfig = ov;
								if (nv.is_object())
									newConfig = nv;
								if (oldConfig.is_object() || newConfig.is_object()) {
									_db->_networkChanged(oldConfig, newConfig, true);
								}
							}
							catch (const nlohmann::json::parse_error& e) {
								ZTC_LOG("JSON parse error: %s\n", e.what());
							}
							catch (const std::exception& e) {
								ZTC_LOG("Exception in Redis network listener: %s\n", e.what());
							}
						}
						if (_is_cluster) {
							_cluster->xdel(key, id);
						}
						else {
							_redis->xdel(key, id);
						}
						lastID = id;
					}
					Metrics::redis_net_notification++;
				}
			}
		}
		catch (sw::redis::Error& e) {
			ZTC_LOG("Error in Redis network listener: %s\n", e.what());
		}
		catch (const std::exception& e) {
			ZTC_LOG("Exception in Redis network listener: %s\n", e.what());
		}
	}
}

NotificationResult RedisNetworkListener::onNotification(const std::string& payload)
{
	// Handle notifications if needed
	return NotificationResult::Ok;
}

RedisMemberListener::RedisMemberListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis, DB* db)
	: RedisListener(controller_id, redis)
	, _db(db)
{
	// Additional initialization for member listener if needed
}

RedisMemberListener::RedisMemberListener(
	std::string controller_id,
	std::shared_ptr<sw::redis::RedisCluster> cluster,
	DB* db)
	: RedisListener(controller_id, cluster)
	, _db(db)
{
	// Additional initialization for member listener if needed
}

RedisMemberListener::~RedisMemberListener()
{
	stop();	  // join the listen thread before this object's members are destroyed
}

void RedisMemberListener::listen()
{
	std::string key = "member-stream:{" + _controller_id + "}";
	std::string lastID = "0";
	ZTC_LOG("Listening to Redis member stream: %s\n", key.c_str());
	while (_run) {
		auto provider = opentelemetry::trace::Provider::GetTracerProvider();
		auto tracer = provider->GetTracer("RedisMemberListener");
		auto span = tracer->StartSpan("RedisMemberListener::listen");
		auto scope = tracer->WithActiveSpan(span);

		try {
			nlohmann::json tmp;
			std::unordered_map<std::string, ItemStream> result;
			if (_is_cluster) {
				_cluster->xread(key, lastID, std::chrono::seconds(1), 0, std::inserter(result, result.end()));
			}
			else {
				_redis->xread(key, lastID, std::chrono::seconds(1), 0, std::inserter(result, result.end()));
			}

			if (! result.empty()) {
				for (auto element : result) {
					for (auto rec : element.second) {
						std::string id = rec.first;
						auto attrs = rec.second;

						for (auto a : attrs) {
							try {
								tmp = nlohmann::json::parse(a.second);
								nlohmann::json& ov = tmp["old_val"];
								nlohmann::json& nv = tmp["new_val"];
								nlohmann::json oldConfig, newConfig;
								if (ov.is_object())
									oldConfig = ov;
								if (nv.is_object())
									newConfig = nv;
								if (oldConfig.is_object() || newConfig.is_object()) {
									_db->_memberChanged(oldConfig, newConfig, true);
								}
							}
							catch (const nlohmann::json::parse_error& e) {
								ZTC_LOG("JSON parse error: %s\n", e.what());
							}
							catch (const std::exception& e) {
								ZTC_LOG("Exception in Redis member listener: %s\n", e.what());
							}
						}
						if (_is_cluster) {
							_cluster->xdel(key, id);
						}
						else {
							_redis->xdel(key, id);
						}
						lastID = id;
					}
					Metrics::redis_mem_notification++;
				}
			}
		}
		catch (sw::redis::Error& e) {
			ZTC_LOG("Error in Redis member listener: %s\n", e.what());
		}
		catch (const std::exception& e) {
			ZTC_LOG("Exception in Redis member listener: %s\n", e.what());
		}
	}
}

NotificationResult RedisMemberListener::onNotification(const std::string& payload)
{ return NotificationResult::Ok; }

}	// namespace ZeroTier

#endif	 // ZT_CONTROLLER_USE_LIBPQ
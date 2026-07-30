/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef REDIS_STATUS_WRITER_HPP
#define REDIS_STATUS_WRITER_HPP

#include "Redis.hpp"
#include "StatusWriter.hpp"

#include <memory>
#include <mutex>
#include <sw/redis++/redis++.h>

namespace ZeroTier {

class RedisStatusWriter : public StatusWriter {
  public:
	RedisStatusWriter(std::shared_ptr<sw::redis::Redis> redis, std::string controller_id);
	RedisStatusWriter(std::shared_ptr<sw::redis::RedisCluster> cluster, std::string controller_id);
	virtual ~RedisStatusWriter();

	virtual void updateNodeStatus(
		const std::string& network_id,
		const std::string& node_id,
		const std::string& os,
		const std::string& arch,
		const std::string& version,
		const InetAddress& address,
		int64_t last_seen,
		const std::string& /* frontend unused */) override;
	virtual size_t queueLength() const override;
	virtual void writePending() override;

  private:
	void _doWritePending(sw::redis::Transaction& tx, const std::vector<PendingStatusEntry>& toWrite);

	std::string _controller_id;

	enum RedisMode { REDIS_MODE_STANDALONE, REDIS_MODE_CLUSTER };
	std::shared_ptr<sw::redis::Redis> _redis;
	std::shared_ptr<sw::redis::RedisCluster> _cluster;
	RedisMode _mode = REDIS_MODE_STANDALONE;

	mutable std::mutex _lock;
	std::vector<PendingStatusEntry> _pending;
};

}	// namespace ZeroTier

#endif	 // REDIS_STATUS_WRITER_HPP
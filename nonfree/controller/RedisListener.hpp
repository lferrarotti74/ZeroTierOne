/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifdef ZT_CONTROLLER_USE_LIBPQ

#ifndef ZT_CONTROLLER_REDIS_LISTENER_HPP
#define ZT_CONTROLLER_REDIS_LISTENER_HPP

#include "DB.hpp"
#include "NotificationListener.hpp"
#include "Redis.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <sw/redis++/redis++.h>
#include <thread>

namespace ZeroTier {

class RedisListener : public NotificationListener {
  public:
	RedisListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis);
	RedisListener(std::string controller_id, std::shared_ptr<sw::redis::RedisCluster> cluster);

	virtual ~RedisListener();

	virtual void listen() = 0;
	virtual NotificationResult onNotification(const std::string& payload) override
	{ return NotificationResult::Ok; }

	void start()
	{
		_run = true;
		_listenThread = std::thread(&RedisListener::listen, this);
	}

	// Stop and join the listen thread. Must be called from the most-derived destructor
	// (before its members are torn down) since listen() lives in the derived class, and
	// by the owning DB before it tears down state the callback touches.
	// Idempotent; the base destructor also calls it as a safety net.
	void stop() override;

  protected:
	std::string _controller_id;
	std::shared_ptr<sw::redis::Redis> _redis;
	std::shared_ptr<sw::redis::RedisCluster> _cluster;
	bool _is_cluster = false;
	std::atomic<bool> _run { false };

  private:
	std::thread _listenThread;
};

class RedisNetworkListener : public RedisListener {
  public:
	RedisNetworkListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis, DB* db);
	RedisNetworkListener(std::string controller_id, std::shared_ptr<sw::redis::RedisCluster> cluster, DB* db);
	virtual ~RedisNetworkListener();

	virtual void listen() override;
	virtual NotificationResult onNotification(const std::string& payload) override;

  private:
	DB* _db;
};

class RedisMemberListener : public RedisListener {
  public:
	RedisMemberListener(std::string controller_id, std::shared_ptr<sw::redis::Redis> redis, DB* db);
	RedisMemberListener(std::string controller_id, std::shared_ptr<sw::redis::RedisCluster> cluster, DB* db);
	virtual ~RedisMemberListener();

	virtual void listen() override;
	virtual NotificationResult onNotification(const std::string& payload) override;

  private:
	DB* _db;
};

}	// namespace ZeroTier

#endif	 // ZT_CONTROLLER_REDIS_LISTENER_HPP

#endif	 // ZT_CONTROLLER_USE_LIBPQ
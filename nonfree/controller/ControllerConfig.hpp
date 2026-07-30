/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef CONTROLLER_CONFIG_HPP
#define CONTROLLER_CONFIG_HPP

#include "Redis.hpp"

#include <map>
#include <string>

namespace ZeroTier {

struct PubSubConfig {
	std::string project_id;
	std::string member_change_recv_topic;
	std::string member_change_send_topic;
	std::string network_change_recv_topic;
	std::string network_change_send_topic;
	std::string sso_nonce_publish_topic;
	std::string sso_auth_subscribe_topic;
};

struct BigTableConfig {
	std::string project_id;
	std::string instance_id;
	std::string table_id;
};

struct ControllerConfig {
	bool ssoEnabled;
	std::string listenMode;
	std::string statusMode;
	std::string assignedCentralVersion;
	// Central SSO redirect URLs keyed by frontend ("cv1" or "cv2")
	std::map<std::string, std::string> ssoRedirectURLs;
	RedisConfig* redisConfig;
	PubSubConfig* pubSubConfig;
	BigTableConfig* bigTableConfig;

	ControllerConfig()
		: ssoEnabled(false)
		, listenMode("")
		, statusMode("")
		, assignedCentralVersion("all")
		, redisConfig(nullptr)
		, pubSubConfig(nullptr)
		, bigTableConfig(nullptr)
	{
	}

	// Owns the raw config pointers and frees them in the destructor, so a shallow copy
	// would double-free. This object is only ever heap-allocated or held by value and
	// passed by pointer, never copied, so disable copying outright.
	ControllerConfig(const ControllerConfig&) = delete;
	ControllerConfig& operator=(const ControllerConfig&) = delete;

	~ControllerConfig()
	{
		if (redisConfig) {
			delete redisConfig;
			redisConfig = nullptr;
		}
		if (pubSubConfig) {
			delete pubSubConfig;
			pubSubConfig = nullptr;
		}
		if (bigTableConfig) {
			delete bigTableConfig;
			bigTableConfig = nullptr;
		}
	}
};

}	// namespace ZeroTier
#endif	 // CONTROLLER_CONFIG_HPP
/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef ZT_CONTROLLER_PUBSUBWRITER_HPP
#define ZT_CONTROLLER_PUBSUBWRITER_HPP

#include <google/cloud/pubsub/publisher.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace ZeroTier {

class PubSubWriter {
  public:
	PubSubWriter(std::string project, std::string topic, std::string controller_id);
	virtual ~PubSubWriter();

	bool publishNetworkChange(
		const nlohmann::json& oldNetwork,
		const nlohmann::json& newNetwork,
		const std::string& frontend);

	bool
	publishMemberChange(const nlohmann::json& oldMember, const nlohmann::json& newMember, const std::string& frontend);

	bool publishStatusChange(
		std::string frontend,
		std::string network_id,
		std::string node_id,
		std::string os,
		std::string arch,
		std::string version,
		int64_t last_seen);

	bool publishSSONonceUpdate(
		const std::string& nonce,
		uint64_t nonceExpiration,
		const std::string& networkId,
		const std::string& deviceId,
		const std::string& frontend);

  protected:
	bool publishMessage(const std::string& payload, const std::string& frontend, const std::string& orderingKey);

  private:
	std::string _controller_id;
	std::string _project;
	std::string _topic;
	std::shared_ptr<google::cloud::pubsub::Publisher> _publisher;
};

}	// namespace ZeroTier

#endif	 // ZT_CONTROLLER_PUBSUBWRITER_HPP
/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef CONTROLLERCHANGENOTIFIER_HPP
#define CONTROLLERCHANGENOTIFIER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace ZeroTier {

class PubSubWriter;

class ControllerChangeNotifier {
  public:
	virtual ~ControllerChangeNotifier() = default;

	virtual void notifyNetworkChange(
		const nlohmann::json& oldNetwork,
		const nlohmann::json& newNetwork,
		const std::string& frontend = "") = 0;

	virtual void notifyMemberChange(
		const nlohmann::json& oldMember,
		const nlohmann::json& newMember,
		const std::string& frontend = "") = 0;
};

class PubSubChangeNotifier : public ControllerChangeNotifier {
  public:
	PubSubChangeNotifier(
		std::string controllerID,
		std::string project,
		std::string memberChangeTopic,
		std::string networkChangeTopic);
	virtual ~PubSubChangeNotifier();

	virtual void notifyNetworkChange(
		const nlohmann::json& oldNetwork,
		const nlohmann::json& newNetwork,
		const std::string& frontend = "") override;

	virtual void notifyMemberChange(
		const nlohmann::json& oldMember,
		const nlohmann::json& newMember,
		const std::string& frontend = "") override;

  private:
	std::shared_ptr<PubSubWriter> _networkChangeWriter;
	std::shared_ptr<PubSubWriter> _memberChangeWriter;
};

}	// namespace ZeroTier

#endif	 // CONTROLLERCHANGENOTIFIER_HPP
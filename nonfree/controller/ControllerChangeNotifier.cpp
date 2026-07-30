/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#include "ControllerChangeNotifier.hpp"

#include "PubSubWriter.hpp"

namespace ZeroTier {

PubSubChangeNotifier::PubSubChangeNotifier(
	std::string controllerID,
	std::string project,
	std::string memberChangeTopic,
	std::string networkChangeTopic)
	: ControllerChangeNotifier()
	, _networkChangeWriter(std::make_shared<PubSubWriter>(project, networkChangeTopic, controllerID))
	, _memberChangeWriter(std::make_shared<PubSubWriter>(project, memberChangeTopic, controllerID))
{
}

PubSubChangeNotifier::~PubSubChangeNotifier()
{
}

void PubSubChangeNotifier::notifyNetworkChange(
	const nlohmann::json& oldNetwork,
	const nlohmann::json& newNetwork,
	const std::string& frontend)
{
	_networkChangeWriter->publishNetworkChange(oldNetwork, newNetwork, frontend);
}

void PubSubChangeNotifier::notifyMemberChange(
	const nlohmann::json& oldMember,
	const nlohmann::json& newMember,
	const std::string& frontend)
{
	_memberChangeWriter->publishMemberChange(oldMember, newMember, frontend);
}

}	// namespace ZeroTier
/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef STATUS_WRITER_HPP
#define STATUS_WRITER_HPP

#include "../../node/InetAddress.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace ZeroTier {

/**
 * Abstract interface for writing status information somewhere.
 *
 * Implementations might write to a database, a file, or something else.
 */
class StatusWriter {
  public:
	virtual ~StatusWriter() = default;

	virtual void updateNodeStatus(
		const std::string& network_id,
		const std::string& node_id,
		const std::string& os,
		const std::string& arch,
		const std::string& version,
		const InetAddress& address,
		int64_t last_seen,
		const std::string& target) = 0;
	virtual size_t queueLength() const = 0;
	virtual void writePending() = 0;
};

struct PendingStatusEntry {
	std::string network_id;
	std::string node_id;
	std::string os;
	std::string arch;
	std::string version;
	InetAddress address;
	int64_t last_seen;
	std::string target;
};

/**
 * Return a batch that failed to flush back onto the pending queue so the next cycle
 * retries it instead of dropping the updates on a transient backend outage.
 *
 * @param pending  the writer's pending queue (protected by @p lock)
 * @param lock     the mutex guarding @p pending
 * @param failed   the batch whose write failed (moved from)
 * @param who      writer name, used only for the backlog-cap log line
 *
 * The failed (older) batch is placed ahead of anything queued since the swap so the
 * freshest check-in is still applied last. The backlog is capped (oldest dropped
 * first) so a prolonged outage cannot grow memory without bound.
 */
void requeuePendingStatus(std::vector<PendingStatusEntry>& pending,
						  std::mutex& lock,
						  std::vector<PendingStatusEntry>&& failed,
						  const char* who);

}	// namespace ZeroTier

#endif	 // STATUS_WRITER_HPP

/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#include "StatusWriter.hpp"

#include "CtlUtil.hpp"

#include <cstdio>
#include <iterator>
#include <utility>

namespace ZeroTier {

// Maximum status-update backlog to retain across failed flushes. Bounds memory if a
// backend stays down for an extended period; beyond this the oldest (stalest) entries
// are dropped first.
static const size_t kMaxPendingStatus = 100000;

void requeuePendingStatus(std::vector<PendingStatusEntry>& pending,
						  std::mutex& lock,
						  std::vector<PendingStatusEntry>&& failed,
						  const char* who)
{
	if (failed.empty()) {
		return;
	}

	std::lock_guard<std::mutex> l(lock);
	// Put the failed (older) batch ahead of anything queued since the swap, preserving
	// chronological order so the freshest check-in is still applied last on the backend.
	// (updateNodeStatus and writePending run on the same thread today, so `pending` is
	// normally empty here; the merge is defensive.)
	if (! pending.empty()) {
		failed.insert(failed.end(), std::make_move_iterator(pending.begin()), std::make_move_iterator(pending.end()));
	}

	if (failed.size() > kMaxPendingStatus) {
		size_t drop = failed.size() - kMaxPendingStatus;
		ZTC_LOG("%s: status backlog exceeded %zu entries; dropping %zu oldest\n", who, kMaxPendingStatus, drop);
		failed.erase(failed.begin(), failed.begin() + drop);
	}

	pending = std::move(failed);
}

}	// namespace ZeroTier

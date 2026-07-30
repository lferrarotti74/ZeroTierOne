/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef BIGTABLESTATUSWRITER_HPP
#define BIGTABLESTATUSWRITER_HPP

#include "StatusWriter.hpp"

#include <cstdint>
#include <google/cloud/bigtable/table.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ZeroTier {

class PubSubWriter;

class BigTableStatusWriter : public StatusWriter {
  public:
	BigTableStatusWriter(const std::string& project_id, const std::string& instance_id, const std::string& table_id);
	virtual ~BigTableStatusWriter();

	virtual void updateNodeStatus(
		const std::string& network_id,
		const std::string& node_id,
		const std::string& os,
		const std::string& arch,
		const std::string& version,
		const InetAddress& address,
		int64_t last_seen,
		const std::string& frontend) override;
	virtual size_t queueLength() const override;
	virtual void writePending() override;

  private:
	const std::string _project_id;
	const std::string _instance_id;
	const std::string _table_id;

	mutable std::mutex _lock;
	std::vector<PendingStatusEntry> _pending;
	google::cloud::bigtable::Table* _table;

	// In-process record of the node_info (os/arch/version) we last wrote for each
	// row, so we can skip rewriting it when it hasn't changed -- no read RPC needed.
	// Keyed by a 64-bit hash of the row key to keep the map compact.  Only touched
	// from writePending(), which is serialized on a single thread, so it needs no
	// lock of its own.
	struct NodeInfoState {
		uint64_t valueHash;		// hash of the os|arch|version last written
		int64_t lastWrittenMs;	// when node_info was last written (periodic refresh)
		int64_t lastSeenMs;		// when this row last appeared in a batch (eviction)
	};
	std::unordered_map<uint64_t, NodeInfoState> _lastNodeInfo;
	int64_t _lastEvictionMs;
};

}	// namespace ZeroTier

#endif
/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#include "PostgresStatusWriter.hpp"

#include "../../node/Metrics.hpp"
#include "CtlUtil.hpp"

#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

namespace ZeroTier {

PostgresStatusWriter::PostgresStatusWriter(std::shared_ptr<ConnectionPool<PostgresConnection> > pool) : _pool(pool)
{
}

PostgresStatusWriter::~PostgresStatusWriter()
{
	writePending();
}

void PostgresStatusWriter::updateNodeStatus(
	const std::string& network_id,
	const std::string& node_id,
	const std::string& os,
	const std::string& arch,
	const std::string& version,
	const InetAddress& address,
	int64_t last_seen,
	const std::string& /* frontend unused */)
{
	std::lock_guard<std::mutex> l(_lock);
	_pending.push_back({ network_id, node_id, os, arch, version, address, last_seen, "" });
}

size_t PostgresStatusWriter::queueLength() const
{
	std::lock_guard<std::mutex> l(_lock);
	return _pending.size();
}

void PostgresStatusWriter::writePending()
{
	std::vector<PendingStatusEntry> toWrite;
	{
		std::lock_guard<std::mutex> l(_lock);
		toWrite.swap(_pending);
	}
	if (toWrite.empty()) {
		return;
	}

	try {
		// RAII guard: returns the connection to the pool on every exit path (commit,
		// throw from pqxx::work/pipeline/commit, etc.) so a write failure can't leak it.
		PooledConnection<PostgresConnection> conn(_pool);
		pqxx::work w(*conn->c);

		pqxx::pipeline pipe(w);
		for (const auto& entry : toWrite) {
			char iptmp[64] = { 0 };
			nlohmann::json record = {
				{ entry.address.toIpString(iptmp), entry.last_seen },
			};

			std::string insert_statement =
				"INSERT INTO network_memberships_ctl (device_id, network_id, last_seen, os, arch) "
				"VALUES ('"
				+ w.esc(entry.node_id) + "', '" + w.esc(entry.network_id) + "', '" + w.esc(record.dump())
				+ "'::JSONB, "
				  "'"
				+ w.esc(entry.os) + "', '" + w.esc(entry.arch)
				+ "') "
				  "ON CONFLICT (device_id, network_id) DO UPDATE SET os = EXCLUDED.os, arch = EXCLUDED.arch, "
				  "last_seen = network_memberships_ctl.last_seen || EXCLUDED.last_seen";

			pipe.insert(insert_statement);
			Metrics::pgsql_node_checkin++;
		}

		pipe.complete();
		w.commit();
		// conn returned to the pool by PooledConnection's destructor on scope exit.
	}
	catch (const std::exception& e) {
		ZTC_LOG("Error writing to Postgres: %s\n", e.what());
		// Don't drop the batch on a transient failure — re-queue it for the next cycle.
		requeuePendingStatus(_pending, _lock, std::move(toWrite), "PostgresStatusWriter");
	}
}

}	// namespace ZeroTier
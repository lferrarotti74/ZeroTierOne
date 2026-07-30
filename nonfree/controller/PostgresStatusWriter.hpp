/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef POSTGRES_STATUS_WRITER_HPP
#define POSTGRES_STATUS_WRITER_HPP

#include "PostgreSQL.hpp"
#include "StatusWriter.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ZeroTier {

class PostgresStatusWriter : public StatusWriter {
  public:
	PostgresStatusWriter(std::shared_ptr<ConnectionPool<PostgresConnection> > pool);
	virtual ~PostgresStatusWriter();

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
	std::shared_ptr<ConnectionPool<PostgresConnection> > _pool;

	mutable std::mutex _lock;
	std::vector<PendingStatusEntry> _pending;
};

}	// namespace ZeroTier

#endif	 // POSTGRES_STATUS_WRITER_HPP
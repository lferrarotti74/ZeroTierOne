/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef ZT_CONNECTION_POOL_H_
#define ZT_CONNECTION_POOL_H_

#ifndef _DEBUG
#define _DEBUG(x)
#endif

#include "../../node/Metrics.hpp"
#include "opentelemetry/trace/provider.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <set>

namespace ZeroTier {

struct ConnectionUnavailable : std::exception {
	char const* what() const throw()
	{
		return "Unable to allocate connection";
	};
};

class Connection {
  public:
	virtual ~Connection() {};

	/**
	 * @return false if this connection is known to be dead/unusable, so the pool
	 * can discard and replace it rather than handing it out again.  Defaults to
	 * true for connection types that don't implement a health check.
	 */
	virtual bool alive() const
	{
		return true;
	}
};

class ConnectionFactory {
  public:
	virtual ~ConnectionFactory() {};
	virtual std::shared_ptr<Connection> create() = 0;
};

struct ConnectionPoolStats {
	size_t pool_size;
	size_t borrowed_size;
};

template <class T> class ConnectionPool {
  public:
	/**
	 * @param max_pool_size    hard cap on live connections (idle + borrowed).
	 * @param min_pool_size    connections pre-created up front and replenished on borrow().
	 * @param factory          creates new connections on demand.
	 * @param borrow_timeout_ms how long borrow() blocks for a free slot once at capacity
	 *   before throwing ConnectionUnavailable.  0 restores the old fail-fast behavior.
	 */
	ConnectionPool(size_t max_pool_size,
				   size_t min_pool_size,
				   std::shared_ptr<ConnectionFactory> factory,
				   unsigned long borrow_timeout_ms = 5000)
		: m_maxPoolSize(max_pool_size)
		, m_minPoolSize(min_pool_size)
		, m_borrowTimeout(borrow_timeout_ms)
		, m_factory(factory)
	{
		Metrics::max_pool_size += max_pool_size;
		Metrics::min_pool_size += min_pool_size;
		while (m_pool.size() < m_minPoolSize) {
			m_pool.push_back(m_factory->create());
			Metrics::pool_avail++;
		}
	};

	ConnectionPoolStats get_stats()
	{
		std::unique_lock<std::mutex> lock(m_poolMutex);

		ConnectionPoolStats stats;
		stats.pool_size = m_pool.size();
		stats.borrowed_size = m_borrowed.size();

		return stats;
	};

	~ConnectionPool() {};

	/**
	 * Borrow a connection for temporary use.
	 *
	 * Hands out an idle connection if one is available, otherwise creates a new one while
	 * under max_pool_size.  At capacity it first tries to reclaim an abandoned connection
	 * (one only the pool still references), then blocks until an unborrow() frees a slot or
	 * the borrow timeout elapses.  Dead connections (alive() == false) are discarded and
	 * replaced rather than handed out, so the pool recovers after the database drops every
	 * connection (e.g. an AlloyDB maintenance restart).
	 *
	 * When done, either call unborrow() to return it (safe even if it went bad), or just let
	 * it go out of scope -- an abandoned connection is reclaimed and replaced on a later
	 * borrow().  Prefer PooledConnection for exception safety.
	 *
	 * @throws ConnectionUnavailable if the borrow timeout elapses with no slot free, or if
	 *   creating a new connection fails.
	 * @return a shared_ptr to the connection object
	 */
	std::shared_ptr<T> borrow()
	{
		auto provider = opentelemetry::trace::Provider::GetTracerProvider();
		auto tracer = provider->GetTracer("connection_pool");
		auto span = tracer->StartSpan("connection_pool::borrow");
		auto scope = tracer->WithActiveSpan(span);

		std::unique_lock<std::mutex> l(m_poolMutex);

		// Block (rather than fail immediately) until a connection becomes available or
		// the borrow timeout elapses. Connections are reliably returned via unborrow(),
		// so a brief wait absorbs transient bursts above the pool size instead of
		// throwing. A zero timeout reproduces the old fail-fast behavior.
		const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + m_borrowTimeout;

		for (;;) {
			// Discard any dead connections sitting idle in the pool so we never hand
			// one out.  This forces the size checks below to replenish with fresh
			// connections, which is what lets the pool recover after the DB server
			// drops every connection (e.g. an AlloyDB maintenance restart).
			for (auto it = m_pool.begin(); it != m_pool.end();) {
				if (! (*it)->alive()) {
					it = m_pool.erase(it);
					Metrics::pool_avail--;
				}
				else {
					++it;
				}
			}

			while ((m_pool.size() + m_borrowed.size()) < m_minPoolSize) {
				std::shared_ptr<Connection> conn = m_factory->create();
				m_pool.push_back(conn);
				Metrics::pool_avail++;
			}

			// An idle connection is available: hand out the front of the pool.
			if (! m_pool.empty()) {
				std::shared_ptr<Connection> conn = m_pool.front();
				m_pool.pop_front();
				Metrics::pool_avail--;
				m_borrowed.insert(conn);
				Metrics::pool_in_use++;
				return std::static_pointer_cast<T>(conn);
			}

			// No idle connection but we're under the cap: create a fresh one.
			if ((m_pool.size() + m_borrowed.size()) < m_maxPoolSize) {
				try {
					std::shared_ptr<Connection> conn = m_factory->create();
					m_borrowed.insert(conn);
					Metrics::pool_in_use++;
					return std::static_pointer_cast<T>(conn);
				}
				catch (std::exception& e) {
					span->SetStatus(opentelemetry::trace::StatusCode::kError, e.what());
					Metrics::pool_errors++;
					throw ConnectionUnavailable();
				}
			}

			// At capacity: reclaim an abandoned connection (one only the pool still
			// references) if there is one.
			for (auto it = m_borrowed.begin(); it != m_borrowed.end(); ++it) {
				if (it->use_count() == 1) {
					// This connection has been abandoned! Destroy it and create a new connection
					try {
						_DEBUG("Creating new connection to replace discarded connection");
						std::shared_ptr<Connection> conn = m_factory->create();
						m_borrowed.erase(it);
						m_borrowed.insert(conn);
						return std::static_pointer_cast<T>(conn);
					}
					catch (std::exception& e) {
						span->SetStatus(opentelemetry::trace::StatusCode::kError, e.what());
						// Error creating a replacement connection
						Metrics::pool_errors++;
						throw ConnectionUnavailable();
					}
				}
			}

			// Everything is in active use. Wait for an unborrow() to free a slot, then
			// retry. Give up (throw, as before) once the deadline passes.
			if (m_cond.wait_until(l, deadline) == std::cv_status::timeout) {
				span->SetStatus(opentelemetry::trace::StatusCode::kError,
								"Timed out waiting for an available connection");
				Metrics::pool_errors++;
				throw ConnectionUnavailable();
			}
		}
	};

	/**
	 * Return a borrowed connection to the pool.
	 *
	 * Safe to call regardless of the connection's health: only live connections
	 * (alive() == true) are placed back in the idle pool, while a dead one is dropped so its
	 * slot is replaced with a fresh connection on the next borrow().  Wakes one waiter
	 * blocked in borrow().
	 *
	 * @param conn the connection previously obtained from borrow()
	 */
	void unborrow(std::shared_ptr<T> conn)
	{
		auto provider = opentelemetry::trace::Provider::GetTracerProvider();
		auto tracer = provider->GetTracer("connection_pool");
		auto span = tracer->StartSpan("connection_pool::unborrow");
		auto scope = tracer->WithActiveSpan(span);

		// Lock
		std::unique_lock<std::mutex> lock(m_poolMutex);
		m_borrowed.erase(conn);
		Metrics::pool_in_use--;
		// Only return live connections to the pool.  A dead one (e.g. the caller
		// hit an error because the DB dropped it) is let go here so its slot is
		// replaced with a fresh connection on the next borrow().
		if (conn->alive() && (m_pool.size() + m_borrowed.size()) < m_maxPoolSize) {
			Metrics::pool_avail++;
			m_pool.push_back(conn);
		}
		// A borrowed slot just freed up (and possibly an idle connection too); wake one
		// waiter blocked in borrow().
		m_cond.notify_one();
	};

  protected:
	size_t m_maxPoolSize;
	size_t m_minPoolSize;
	std::chrono::milliseconds m_borrowTimeout;
	std::shared_ptr<ConnectionFactory> m_factory;
	std::deque<std::shared_ptr<Connection> > m_pool;
	std::set<std::shared_ptr<Connection> > m_borrowed;
	std::mutex m_poolMutex;
	std::condition_variable m_cond;
};

/**
 * RAII guard for a borrowed connection.
 *
 * Borrows from the pool on construction and returns it on destruction, so the
 * connection can never be leaked on an exception path (borrow() throws rather than
 * returning null, and unborrow() must run on every path or the pool leaks a slot).
 * Call unborrow() to return it early; the destructor is then a no-op.
 */
template <class T> class PooledConnection {
  public:
	explicit PooledConnection(const std::shared_ptr<ConnectionPool<T> >& pool) : m_pool(pool), m_conn(pool->borrow())
	{
	}

	~PooledConnection()
	{ unborrow(); }

	PooledConnection(const PooledConnection&) = delete;
	PooledConnection& operator=(const PooledConnection&) = delete;

	const std::shared_ptr<T>& get() const
	{ return m_conn; }

	T* operator->() const
	{ return m_conn.get(); }

	/**
	 * Return the connection to the pool now.  Idempotent.
	 */
	void unborrow()
	{
		if (m_conn) {
			m_pool->unborrow(m_conn);
			m_conn.reset();
		}
	}

  private:
	std::shared_ptr<ConnectionPool<T> > m_pool;
	std::shared_ptr<T> m_conn;
};

}	// namespace ZeroTier

#endif

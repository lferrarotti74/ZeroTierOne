/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef NOTIFICATION_LISTENER_HPP
#define NOTIFICATION_LISTENER_HPP

#include <string>

namespace ZeroTier {

/**
 * Outcome of handling a single notification.
 *
 * Drives the ack/nack decision for delivery mechanisms that support it (GCP PubSub).
 * Mechanisms without redelivery semantics (Postgres LISTEN/NOTIFY, Redis) ignore it.
 */
enum class NotificationResult {
	Ok,					// processed successfully -> ack
	PermanentFailure,	// message is unprocessable (parse/validation) -> ack to drop, avoid poison redelivery
	TransientFailure,	// retryable error (DB unavailable, connection pool exhausted) -> nack to redeliver
};

/**
 * Base class for notification listeners
 *
 * This class is used to receive notifications from various sources such as Redis, PostgreSQL, etc.
 */
class NotificationListener {
  public:
	NotificationListener() = default;
	virtual ~NotificationListener()
	{
	}

	/**
	 * Called when a notification is received.
	 *
	 * Payload should be parsed and passed to the database handler's save method.
	 *
	 * @param payload The payload of the notification.
	 * @return how the message should be handled (ack/drop vs. nack/redeliver).
	 */
	virtual NotificationResult onNotification(const std::string& payload) = 0;

	/**
	 * Stop and join any background thread so no further onNotification callbacks fire.
	 *
	 * Listeners hold a raw DB* and call back into it from their worker thread; the owner
	 * must be able to quiesce them before tearing down the DB. Idempotent. Default no-op
	 * for listeners that have no background thread.
	 */
	virtual void stop()
	{
	}
};

}	// namespace ZeroTier

#endif	 // NOTIFICATION_LISTENER_HPP
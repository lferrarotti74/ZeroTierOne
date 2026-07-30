/* (c) ZeroTier, Inc.
 * See LICENSE.txt in nonfree/
 */

#ifndef OTEL_CARRIER_HPP
#define OTEL_CARRIER_HPP

#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"

namespace nostd = opentelemetry::nostd;

namespace ZeroTier {

template <typename T> class OtelCarrier : public opentelemetry::context::propagation::TextMapCarrier {
  public:
	OtelCarrier<T>(T& headers) : headers_(headers)
	{
	}

	// A carrier with no backing storage would leave the headers_ reference dangling;
	// it's never default-constructed, so forbid it.
	OtelCarrier() = delete;

	virtual nostd::string_view Get(nostd::string_view key) const noexcept override
	{
		// key.data() is not guaranteed NUL-terminated; bound the std::string by size().
		std::string key_to_compare(key.data(), key.size());

		if (key == opentelemetry::trace::propagation::kTraceParent) {
			key_to_compare = "traceparent";
		}
		else if (key == opentelemetry::trace::propagation::kTraceState) {
			key_to_compare = "tracestate";
		}
		auto it = headers_.find(key_to_compare);
		if (it != headers_.end()) {
			return it->second;
		}

		return "";
	}

	virtual void Set(nostd::string_view key, nostd::string_view value) noexcept override
	{
		headers_.insert(std::pair<std::string, std::string>(std::string(key), std::string(value)));
	}

	T& headers_;
};

}	// namespace ZeroTier

#endif	 // OTEL_CARRIER_HPP
#include "TelemetryContext.hpp"

#include <memory>

namespace clp_s::search {
class TelemetryContext::Impl {};

TelemetryContext::TelemetryContext() : m_impl{std::make_unique<Impl>()} {}

TelemetryContext::~TelemetryContext() = default;
}  // namespace clp_s::search

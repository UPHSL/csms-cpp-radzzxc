#pragma once

#include <optional>
#include <string>
#include "models/ServiceRequest.h"

namespace csms
{

struct ServiceRequestStatusResult
{
    bool success{false};
    bool notFound{false};
    bool unsupportedStatus{false};
    bool invalidTransition{false};
    std::string message{};
    std::optional<ServiceRequest> serviceRequest{std::nullopt};
};

} // namespace csms

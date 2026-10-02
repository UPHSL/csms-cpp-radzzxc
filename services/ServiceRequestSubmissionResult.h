#pragma once

#include <optional>
#include <string>
#include <vector>
#include "models/ServiceRequest.h"

namespace csms
{

struct ServiceRequestSubmissionResult
{
    bool success{false};
    bool residentNotFound{false};
    bool residentInactive{false};
    std::vector<std::string> validationErrors{};
    std::optional<ServiceRequest> serviceRequest{std::nullopt};
};

} // namespace csms
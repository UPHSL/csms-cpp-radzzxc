#pragma once

#include <string>
#include "repositories/ServiceRequestRepository.h"
#include "ServiceRequestStatusResult.h"

namespace csms
{

class ServiceRequestStatusService
{
public:
    explicit ServiceRequestStatusService(ServiceRequestRepository& repository);

    ServiceRequestStatusResult updateStatus(int requestId, const std::string& newStatus);

    static bool isSupportedStatus(const std::string& status);
    static bool isAllowedTransition(const std::string& currentStatus, const std::string& targetStatus);

private:
    ServiceRequestRepository& repository_;
};

} // namespace csms

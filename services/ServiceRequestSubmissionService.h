#pragma once

#include "models/ServiceRequest.h"
#include "models/ServiceRequestValidator.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "ServiceRequestSubmissionResult.h"

namespace csms
{

class ServiceRequestSubmissionService
{
public:
    ServiceRequestSubmissionService(
        const ServiceRequestValidator& validator,
        ResidentRepository& residentRepository,
        ServiceRequestRepository& serviceRequestRepository
    );

    ServiceRequestSubmissionResult submitServiceRequest(const ServiceRequest& request);

private:
    const ServiceRequestValidator& validator_;
    ResidentRepository& residentRepository_;
    ServiceRequestRepository& serviceRequestRepository_;
};

} // namespace csms
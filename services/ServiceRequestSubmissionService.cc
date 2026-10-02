#include "ServiceRequestSubmissionService.h"
#include <utility>

namespace csms
{

ServiceRequestSubmissionService::ServiceRequestSubmissionService(
    const ServiceRequestValidator& validator,
    ResidentRepository& residentRepository,
    ServiceRequestRepository& serviceRequestRepository
)
    : validator_(validator),
      residentRepository_(residentRepository),
      serviceRequestRepository_(serviceRequestRepository)
{
}

ServiceRequestSubmissionResult ServiceRequestSubmissionService::submitServiceRequest(const ServiceRequest& request)
{
    // Step 1: Validate intrinsic service request data
    auto errors = validator_.validate(request);
    if (!errors.empty())
    {
        return ServiceRequestSubmissionResult{
            .success = false,
            .residentNotFound = false,
            .residentInactive = false,
            .validationErrors = std::move(errors),
            .serviceRequest = std::nullopt
        };
    }

    // Step 2: Verify that the referenced Resident exists in persistence
    auto residentOpt = residentRepository_.findById(request.getResidentId());
    if (!residentOpt.has_value())
    {
        return ServiceRequestSubmissionResult{
            .success = false,
            .residentNotFound = true,
            .residentInactive = false,
            .validationErrors = {},
            .serviceRequest = std::nullopt
        };
    }

    // Step 3: Verify that the referenced Resident is Active (reject Inactive)
    if (residentOpt->getStatus() != "Active")
    {
        return ServiceRequestSubmissionResult{
            .success = false,
            .residentNotFound = false,
            .residentInactive = true,
            .validationErrors = {},
            .serviceRequest = std::nullopt
        };
    }

    // Step 4: Persist the valid request into SQLite
    ServiceRequest saved = serviceRequestRepository_.save(request);

    return ServiceRequestSubmissionResult{
        .success = true,
        .residentNotFound = false,
        .residentInactive = false,
        .validationErrors = {},
        .serviceRequest = saved
    };
}

} // namespace csms
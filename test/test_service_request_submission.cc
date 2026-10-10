#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "models/ServiceRequestValidator.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/ServiceRequestSubmissionService.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

// Isolated temporary SQLite database cleaner
struct SubmissionTestDbCleaner
{
    std::filesystem::path path;

    explicit SubmissionTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~SubmissionTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

static csms::Resident makeActiveResident()
{
    return csms::Resident(
        "Juan",
        "Cruz",
        "123 Rizal Street",
        "09171234567",
        "juan.cruz@example.com",
        "Active"
    );
}

static csms::Resident makeInactiveResident()
{
    return csms::Resident(
        "Maria",
        "Santos",
        "456 Bonifacio Avenue",
        "09187654321",
        "maria.santos@example.com",
        "Inactive"
    );
}

static csms::ServiceRequest makeValidRequest(int residentId)
{
    return csms::ServiceRequest(
        residentId,
        "Barangay Clearance",
        "Request for employment requirement",
        "2026-10-02"
    );
}

// Test 1: Valid Service Request Submission Succeeds
DROGON_TEST(ValidServiceRequestSubmissionSucceeds)
{
    SubmissionTestDbCleaner cleaner("test_submission_valid.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request = makeValidRequest(resId);
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == true);
    CHECK(result.residentNotFound == false);
    CHECK(result.residentInactive == false);
    CHECK(result.validationErrors.empty() == true);
    CHECK(result.serviceRequest.has_value() == true);
}

// Test 2: Submitted Service Request Receives a Generated ID
DROGON_TEST(SubmittedServiceRequestReceivesGeneratedId)
{
    SubmissionTestDbCleaner cleaner("test_submission_generated_id.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request = makeValidRequest(resId);
    CHECK(request.getId().has_value() == false);

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == true);
    CHECK(result.serviceRequest.has_value() == true);
    CHECK(result.serviceRequest->getId().has_value() == true);
    CHECK(result.serviceRequest->getId().value() > 0);
}

// Test 3: Submitted Service Request Is Persisted and Retrievable
DROGON_TEST(SubmittedServiceRequestIsPersistedAndRetrievable)
{
    SubmissionTestDbCleaner cleaner("test_submission_retrievable.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(makeValidRequest(resId));
    int reqId = result.serviceRequest->getId().value();

    auto retrieved = reqRepo.findById(reqId);
    CHECK(retrieved.has_value() == true);
    CHECK(retrieved->getId().value() == reqId);
    CHECK(retrieved->getResidentId() == resId);
}

// Test 4: Submitted Service Request Information Is Preserved
DROGON_TEST(SubmittedServiceRequestInformationIsPreserved)
{
    SubmissionTestDbCleaner cleaner("test_submission_info_preserved.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request(
        resId,
        "Certificate Request",
        "Indigency certificate for scholarship application",
        "2026-10-02"
    );

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);
    int reqId = result.serviceRequest->getId().value();

    auto persisted = reqRepo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getResidentId() == resId);
    CHECK(persisted->getServiceType() == "Certificate Request");
    CHECK(persisted->getDescription() == "Indigency certificate for scholarship application");
    CHECK(persisted->getDateRequested() == "2026-10-02");
    CHECK(persisted->getStatus() == "Pending");
}

// Test 5: Submitted Service Request Status Is Pending
DROGON_TEST(SubmittedServiceRequestStatusIsPending)
{
    SubmissionTestDbCleaner cleaner("test_submission_status_pending.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(makeValidRequest(resId));

    CHECK(result.success == true);
    CHECK(result.serviceRequest->getStatus() == "Pending");

    auto persisted = reqRepo.findById(result.serviceRequest->getId().value());
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Pending");
}

// Test 6: Blank Service Type Fails Validation
DROGON_TEST(BlankServiceTypeFailsValidation)
{
    SubmissionTestDbCleaner cleaner("test_submission_blank_service_type.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request(resId, "   ", "Valid description", "2026-10-02");
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.serviceRequest.has_value() == false);
    CHECK(std::find(result.validationErrors.begin(), result.validationErrors.end(), "serviceType") != result.validationErrors.end());
}

// Test 7: Blank Description Fails Validation
DROGON_TEST(BlankDescriptionFailsValidation)
{
    SubmissionTestDbCleaner cleaner("test_submission_blank_description.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request(resId, "Permit Request", "", "2026-10-02");
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.serviceRequest.has_value() == false);
    CHECK(std::find(result.validationErrors.begin(), result.validationErrors.end(), "description") != result.validationErrors.end());
}

// Test 8: Invalid Date Requested Fails Validation
DROGON_TEST(InvalidDateRequestedFailsValidation)
{
    SubmissionTestDbCleaner cleaner("test_submission_invalid_date.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request(resId, "Permit Request", "Valid description", "2026/10/02");
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.serviceRequest.has_value() == false);
    CHECK(std::find(result.validationErrors.begin(), result.validationErrors.end(), "dateRequested") != result.validationErrors.end());
}

// Test 9: Invalid Request Does Not Reach Persistence
DROGON_TEST(InvalidRequestDoesNotReachPersistence)
{
    SubmissionTestDbCleaner cleaner("test_submission_no_partial_persist.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest invalidRequest(resId, "", "Some description", "2026-10-02");
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(invalidRequest);

    CHECK(result.success == false);

    // Verify nothing was saved in persistence
    auto persisted = reqRepo.findById(1);
    CHECK(persisted.has_value() == false);
}

// Test 10: Nonexistent Resident Prevents Submission
DROGON_TEST(NonexistentResidentPreventsSubmission)
{
    SubmissionTestDbCleaner cleaner("test_submission_nonexistent_resident.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    int nonexistentResidentId = 99999;
    csms::ServiceRequest request = makeValidRequest(nonexistentResidentId);

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.residentNotFound == true);
    CHECK(result.residentInactive == false);
    CHECK(result.serviceRequest.has_value() == false);

    auto persisted = reqRepo.findById(1);
    CHECK(persisted.has_value() == false);
}

// Test 11: Inactive Resident Cannot Submit a New Service Request
DROGON_TEST(InactiveResidentCannotSubmitNewServiceRequest)
{
    SubmissionTestDbCleaner cleaner("test_submission_inactive_resident.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident inactiveRes = resRepo.save(makeInactiveResident());
    int resId = inactiveRes.getId().value();

    csms::ServiceRequest request = makeValidRequest(resId);
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.residentInactive == true);
    CHECK(result.residentNotFound == false);
    CHECK(result.serviceRequest.has_value() == false);

    // Verify resident remains Inactive and unmodified
    auto residentInDb = resRepo.findById(resId);
    CHECK(residentInDb.has_value() == true);
    CHECK(residentInDb->getStatus() == "Inactive");

    // Verify no request was written to persistence
    auto persistedReq = reqRepo.findById(1);
    CHECK(persistedReq.has_value() == false);
}

// Test 12: Non-Pending Initial Status Is Rejected
DROGON_TEST(NonPendingInitialStatusIsRejected)
{
    SubmissionTestDbCleaner cleaner("test_submission_non_pending.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequest request(resId, "Community Assistance", "Aid details", "2026-10-02", "Completed");
    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(request);

    CHECK(result.success == false);
    CHECK(result.serviceRequest.has_value() == false);
    CHECK(std::find(result.validationErrors.begin(), result.validationErrors.end(), "status") != result.validationErrors.end());

    auto persisted = reqRepo.findById(1);
    CHECK(persisted.has_value() == false);
}

// Test 13: Service Request Persists Across Repository Access
DROGON_TEST(ServiceRequestPersistsAcrossRepositoryAccess)
{
    SubmissionTestDbCleaner cleaner("test_submission_across_repo.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo1(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo1);

    csms::Resident resident = resRepo.save(makeActiveResident());
    int resId = resident.getId().value();

    csms::ServiceRequestSubmissionResult result = service.submitServiceRequest(makeValidRequest(resId));
    int reqId = result.serviceRequest->getId().value();

    // Verify using a completely distinct repository instance
    csms::ServiceRequestRepository reqRepo2(cleaner.str());
    auto retrieved = reqRepo2.findById(reqId);
    CHECK(retrieved.has_value() == true);
    CHECK(retrieved->getId().value() == reqId);
    CHECK(retrieved->getResidentId() == resId);
    CHECK(retrieved->getServiceType() == "Barangay Clearance");
}

// Test 14: Submission Does Not Modify the Resident
DROGON_TEST(SubmissionDoesNotModifyResident)
{
    SubmissionTestDbCleaner cleaner("test_submission_resident_unmodified.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService service(validator, resRepo, reqRepo);

    csms::Resident original = resRepo.save(csms::Resident(
        "Pedro",
        "Penduko",
        "789 Matino Street",
        "09191234567",
        "pedro.penduko@example.com",
        "Active"
    ));
    int resId = original.getId().value();

    service.submitServiceRequest(makeValidRequest(resId));

    // Verify resident record is completely untouched
    auto residentAfter = resRepo.findById(resId);
    CHECK(residentAfter.has_value() == true);
    CHECK(residentAfter->getFirstName() == "Pedro");
    CHECK(residentAfter->getLastName() == "Penduko");
    CHECK(residentAfter->getAddress() == "789 Matino Street");
    CHECK(residentAfter->getContactNumber() == "09191234567");
    CHECK(residentAfter->getEmail() == "pedro.penduko@example.com");
    CHECK(residentAfter->getStatus() == "Active");
}
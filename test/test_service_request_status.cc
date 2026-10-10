#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/ServiceRequestStatusService.h"
#include <filesystem>
#include <string>
#include <optional>

// Isolated temporary SQLite database cleaner
struct StatusTestDbCleaner
{
    std::filesystem::path path;

    explicit StatusTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~StatusTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

static csms::ServiceRequest seedPendingRequest(csms::ServiceRequestRepository& repo, int residentId = 1)
{
    csms::ServiceRequest req(
        residentId,
        "Barangay Clearance",
        "Employment requirement",
        "2026-10-10",
        "Pending"
    );
    return repo.save(req);
}

// Test 1: Pending Can Move to In Progress
DROGON_TEST(PendingCanMoveToInProgress)
{
    StatusTestDbCleaner cleaner("test_status_pending_to_inprogress.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    csms::ServiceRequestStatusResult result = service.updateStatus(reqId, "In Progress");

    CHECK(result.success == true);
    CHECK(result.notFound == false);
    CHECK(result.unsupportedStatus == false);
    CHECK(result.invalidTransition == false);
    CHECK(result.serviceRequest.has_value() == true);
    CHECK(result.serviceRequest->getStatus() == "In Progress");

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "In Progress");
}

// Test 2: Pending Can Move to Cancelled
DROGON_TEST(PendingCanMoveToCancelled)
{
    StatusTestDbCleaner cleaner("test_status_pending_to_cancelled.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    csms::ServiceRequestStatusResult result = service.updateStatus(reqId, "Cancelled");

    CHECK(result.success == true);
    CHECK(result.serviceRequest.has_value() == true);
    CHECK(result.serviceRequest->getStatus() == "Cancelled");

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Cancelled");
}

// Test 3: In Progress Can Move to Completed
DROGON_TEST(InProgressCanMoveToCompleted)
{
    StatusTestDbCleaner cleaner("test_status_inprogress_to_completed.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Legitimate step 1: Pending -> In Progress
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);

    // Legitimate step 2: In Progress -> Completed
    auto step2 = service.updateStatus(reqId, "Completed");
    CHECK(step2.success == true);
    CHECK(step2.serviceRequest.has_value() == true);
    CHECK(step2.serviceRequest->getStatus() == "Completed");

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Completed");
}

// Test 4: In Progress Can Move to Cancelled
DROGON_TEST(InProgressCanMoveToCancelled)
{
    StatusTestDbCleaner cleaner("test_status_inprogress_to_cancelled.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Legitimate step 1: Pending -> In Progress
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);

    // Legitimate step 2: In Progress -> Cancelled
    auto step2 = service.updateStatus(reqId, "Cancelled");
    CHECK(step2.success == true);
    CHECK(step2.serviceRequest.has_value() == true);
    CHECK(step2.serviceRequest->getStatus() == "Cancelled");

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Cancelled");
}

// Test 5: Pending Cannot Move Directly to Completed
DROGON_TEST(PendingCannotMoveDirectlyToCompleted)
{
    StatusTestDbCleaner cleaner("test_status_pending_to_completed_invalid.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    csms::ServiceRequestStatusResult result = service.updateStatus(reqId, "Completed");

    CHECK(result.success == false);
    CHECK(result.invalidTransition == true);
    CHECK(result.serviceRequest.has_value() == false);

    // Verify persisted status is still Pending
    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Pending");
}

// Test 6: In Progress Cannot Return to Pending
DROGON_TEST(InProgressCannotReturnToPending)
{
    StatusTestDbCleaner cleaner("test_status_inprogress_to_pending_invalid.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Move to In Progress
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);

    // Attempt invalid reverse to Pending
    auto step2 = service.updateStatus(reqId, "Pending");
    CHECK(step2.success == false);
    CHECK(step2.invalidTransition == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "In Progress");
}

// Test 7: Completed Is Terminal
DROGON_TEST(CompletedIsTerminal)
{
    StatusTestDbCleaner cleaner("test_status_completed_terminal.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Legitimately move to Completed
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);
    auto step2 = service.updateStatus(reqId, "Completed");
    CHECK(step2.success == true);

    // Attempt transition away from Completed
    auto attempt1 = service.updateStatus(reqId, "In Progress");
    CHECK(attempt1.success == false);
    CHECK(attempt1.invalidTransition == true);

    auto attempt2 = service.updateStatus(reqId, "Pending");
    CHECK(attempt2.success == false);
    CHECK(attempt2.invalidTransition == true);

    auto attempt3 = service.updateStatus(reqId, "Cancelled");
    CHECK(attempt3.success == false);
    CHECK(attempt3.invalidTransition == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Completed");
}

// Test 8: Cancelled Is Terminal
DROGON_TEST(CancelledIsTerminal)
{
    StatusTestDbCleaner cleaner("test_status_cancelled_terminal.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Legitimately move to Cancelled
    auto step1 = service.updateStatus(reqId, "Cancelled");
    CHECK(step1.success == true);

    // Attempt transition away from Cancelled
    auto attempt1 = service.updateStatus(reqId, "Pending");
    CHECK(attempt1.success == false);
    CHECK(attempt1.invalidTransition == true);

    auto attempt2 = service.updateStatus(reqId, "In Progress");
    CHECK(attempt2.success == false);
    CHECK(attempt2.invalidTransition == true);

    auto attempt3 = service.updateStatus(reqId, "Completed");
    CHECK(attempt3.success == false);
    CHECK(attempt3.invalidTransition == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Cancelled");
}

// Test 9: Unsupported Status Is Rejected
DROGON_TEST(UnsupportedStatusIsRejected)
{
    StatusTestDbCleaner cleaner("test_status_unsupported.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    auto result1 = service.updateStatus(reqId, "Approved");
    CHECK(result1.success == false);
    CHECK(result1.unsupportedStatus == true);

    auto result2 = service.updateStatus(reqId, "Rejected");
    CHECK(result2.success == false);
    CHECK(result2.unsupportedStatus == true);

    auto result3 = service.updateStatus(reqId, "Processing");
    CHECK(result3.success == false);
    CHECK(result3.unsupportedStatus == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Pending");
}

// Test 10: Nonexistent Service Request Is Handled Safely
DROGON_TEST(NonexistentServiceRequestHandledSafely)
{
    StatusTestDbCleaner cleaner("test_status_nonexistent.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    // Seed one existing request so we can prove it is left untouched
    csms::ServiceRequest existing = seedPendingRequest(repo);
    int existingId = existing.getId().value();

    int unknownId = 99999;
    auto result = service.updateStatus(unknownId, "In Progress");

    CHECK(result.success == false);
    CHECK(result.notFound == true);
    CHECK(result.unsupportedStatus == false);
    CHECK(result.invalidTransition == false);
    CHECK(result.serviceRequest.has_value() == false);

    // No Service Request was created for the unknown ID or as a new row
    auto persisted = repo.findById(unknownId);
    CHECK(persisted.has_value() == false);
    auto nextRow = repo.findById(existingId + 1);
    CHECK(nextRow.has_value() == false);

    // The existing Service Request remains unchanged
    auto existingAfter = repo.findById(existingId);
    CHECK(existingAfter.has_value() == true);
    CHECK(existingAfter->getId().value() == existingId);
    CHECK(existingAfter->getResidentId() == 1);
    CHECK(existingAfter->getServiceType() == "Barangay Clearance");
    CHECK(existingAfter->getDescription() == "Employment requirement");
    CHECK(existingAfter->getDateRequested() == "2026-10-10");
    CHECK(existingAfter->getStatus() == "Pending");
}

// Test 11: Successful Transition Preserves Service Request Information
DROGON_TEST(SuccessfulTransitionPreservesServiceRequestInformation)
{
    StatusTestDbCleaner cleaner("test_status_preserves_info.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest original(
        42,
        "Certificate Request",
        "Indigency certificate for scholarship",
        "2026-10-10",
        "Pending"
    );
    csms::ServiceRequest saved = repo.save(original);
    int reqId = saved.getId().value();

    auto result = service.updateStatus(reqId, "In Progress");
    CHECK(result.success == true);

    // Verify through result
    CHECK(result.serviceRequest->getId().value() == reqId);
    CHECK(result.serviceRequest->getResidentId() == 42);
    CHECK(result.serviceRequest->getServiceType() == "Certificate Request");
    CHECK(result.serviceRequest->getDescription() == "Indigency certificate for scholarship");
    CHECK(result.serviceRequest->getDateRequested() == "2026-10-10");
    CHECK(result.serviceRequest->getStatus() == "In Progress");

    // Verify through database reload
    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getId().value() == reqId);
    CHECK(persisted->getResidentId() == 42);
    CHECK(persisted->getServiceType() == "Certificate Request");
    CHECK(persisted->getDescription() == "Indigency certificate for scholarship");
    CHECK(persisted->getDateRequested() == "2026-10-10");
    CHECK(persisted->getStatus() == "In Progress");
}

// Test 12: Invalid Transition Does Not Modify Persistence
DROGON_TEST(InvalidTransitionDoesNotModifyPersistence)
{
    StatusTestDbCleaner cleaner("test_status_no_partial_change.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo, 10);
    int reqId = req.getId().value();

    auto result = service.updateStatus(reqId, "Completed");
    CHECK(result.success == false);
    CHECK(result.invalidTransition == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Pending");
    CHECK(persisted->getId().value() == reqId);
    CHECK(persisted->getResidentId() == 10);
    CHECK(persisted->getServiceType() == "Barangay Clearance");
    CHECK(persisted->getDescription() == "Employment requirement");
    CHECK(persisted->getDateRequested() == "2026-10-10");
}

// Test 13: Same-Status Request Is Rejected
DROGON_TEST(SameStatusRequestIsRejected)
{
    StatusTestDbCleaner cleaner("test_status_same_status.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestStatusService service(repo);

    csms::ServiceRequest req = seedPendingRequest(repo);
    int reqId = req.getId().value();

    // Pending -> Pending
    auto result1 = service.updateStatus(reqId, "Pending");
    CHECK(result1.success == false);
    CHECK(result1.invalidTransition == true);

    // Move to In Progress
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);

    // In Progress -> In Progress
    auto result2 = service.updateStatus(reqId, "In Progress");
    CHECK(result2.success == false);
    CHECK(result2.invalidTransition == true);

    auto persisted = repo.findById(reqId);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "In Progress");
}

// Test 14 (Student-Designed Test): Existing Request Processes Even If Resident Is Inactive
DROGON_TEST(ExistingRequestProcessesEvenIfResidentIsInactive)
{
    StatusTestDbCleaner cleaner("test_status_inactive_resident_workflow.db");
    csms::ResidentRepository resRepo(cleaner.str());
    csms::ServiceRequestRepository reqRepo(cleaner.str());
    csms::ServiceRequestStatusService service(reqRepo);

    // 1. Create an active resident
    csms::Resident activeResident(
        "Roberto",
        "Dalisay",
        "101 Mabini Street",
        "09179998877",
        "roberto.dalisay@example.com",
        "Active"
    );
    csms::Resident savedResident = resRepo.save(activeResident);
    int resId = savedResident.getId().value();

    // 2. Submit an existing request for this resident
    csms::ServiceRequest request(
        resId,
        "Permit Request",
        "Business permit renewal",
        "2026-10-10",
        "Pending"
    );
    csms::ServiceRequest savedRequest = reqRepo.save(request);
    int reqId = savedRequest.getId().value();

    // 3. Deactivate the resident
    bool deactivated = resRepo.deactivateById(resId);
    CHECK(deactivated == true);

    auto residentInDb = resRepo.findById(resId);
    CHECK(residentInDb.has_value() == true);
    CHECK(residentInDb->getStatus() == "Inactive");

    // 4. Verify existing request can still legitimately move to In Progress
    auto step1 = service.updateStatus(reqId, "In Progress");
    CHECK(step1.success == true);
    CHECK(step1.serviceRequest.has_value() == true);
    CHECK(step1.serviceRequest->getStatus() == "In Progress");

    // 5. Verify existing request can still legitimately move to Completed
    auto step2 = service.updateStatus(reqId, "Completed");
    CHECK(step2.success == true);
    CHECK(step2.serviceRequest.has_value() == true);
    CHECK(step2.serviceRequest->getStatus() == "Completed");

    // 6. Verify resident remains strictly untouched and Inactive
    auto residentAfter = resRepo.findById(resId);
    CHECK(residentAfter.has_value() == true);
    CHECK(residentAfter->getStatus() == "Inactive");
    CHECK(residentAfter->getFirstName() == "Roberto");
}

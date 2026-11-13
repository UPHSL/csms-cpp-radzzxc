// T16 - Final System Release and Development Checkpoint
// Final Examination integration test and automated release version verification.

#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <future>
#include <memory>
#include <string>

#include "Version.h"
#include "controllers/HealthController.h"
#include "controllers/DashboardController.h"
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/DashboardService.h"
#include "services/ResidentRegistrationService.h"
#include "services/ResidentDeactivationService.h"
#include "services/ServiceRequestSubmissionService.h"
#include "services/ServiceRequestStatusService.h"
#include "models/ResidentValidator.h"
#include "models/ServiceRequestValidator.h"

namespace
{

struct FinalExamTestDb
{
    std::filesystem::path path;

    explicit FinalExamTestDb(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~FinalExamTestDb()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

drogon::HttpResponsePtr dispatch(std::function<void(std::function<void(const drogon::HttpResponsePtr&)>&&)> action)
{
    std::promise<drogon::HttpResponsePtr> p;
    auto f = p.get_future();
    action([&p](const drogon::HttpResponsePtr& resp) { p.set_value(resp); });
    return f.get();
}

} // namespace

// -----------------------------------------------------------------------------
// Part G: Automated Release Version Verification
// Proves that HealthController dynamically reports authoritative version 1.0.0
// matching Version.h without hardcoded discrepancies.
// -----------------------------------------------------------------------------
DROGON_TEST(FinalExam_ReleaseVersionAndHealthEndpointIntegrity)
{
    // 1. Verify single source of truth constants
    CHECK(csms::Version::kVersion == "1.0.0");
    CHECK(csms::Version::kDisplayVersion == "v1.0.0");
    CHECK(csms::Version::kShortName == "CSMS");
    CHECK(csms::Version::kApplication == "Community Services Management System");
    CHECK(csms::Version::kStatus == "ok");

    // 2. Invoke HealthController
    HealthController healthCtrl;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/health");
    req->setMethod(drogon::Get);

    auto resp = dispatch([&](auto&& cb) {
        healthCtrl.asyncHandleHttpRequest(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(resp->getContentType() == drogon::CT_APPLICATION_JSON);

    auto json = resp->getJsonObject();
    REQUIRE(json != nullptr);
    CHECK((*json)["status"].asString() == "ok");
    CHECK((*json)["application"].asString() == "Community Services Management System");
    CHECK((*json)["version"].asString() == "1.0.0");

    // Ensure health check exposes zero sensitive fields
    CHECK(!json->isMember("password"));
    CHECK(!json->isMember("token"));
    CHECK(!json->isMember("database"));
    CHECK(!json->isMember("env"));
}

// -----------------------------------------------------------------------------
// Part F: Final Examination Multi-Module Integration Test
// Crosses four distinct architectural modules with real SQLite disk persistence:
// 1. Resident Registration & Deactivation (T04, T07)
// 2. Service Request Submission & Eligibility Gate (T09)
// 3. Service Request Status Progression Workflow (T10)
// 4. Live Dashboard Aggregate Synchronization (T14)
// -----------------------------------------------------------------------------
DROGON_TEST(FinalExamIntegration_ResidentLifecycleServiceRequestAndDashboardFlow)
{
    FinalExamTestDb db("t16_final_exam_integration.db");
    auto residentRepo = std::make_shared<csms::ResidentRepository>(db.path.string());
    auto requestRepo = std::make_shared<csms::ServiceRequestRepository>(db.path.string());

    csms::ResidentValidator resValidator;
    csms::ServiceRequestValidator reqValidator;

    csms::ResidentRegistrationService registrationService(resValidator, *residentRepo);
    csms::ResidentDeactivationService deactivationService(*residentRepo);
    csms::ServiceRequestSubmissionService submissionService(reqValidator, *residentRepo, *requestRepo);
    csms::ServiceRequestStatusService statusService(*requestRepo);
    csms::DashboardService dashboardService(*residentRepo, *requestRepo);

    // Initial state: empty database
    auto summaryInit = dashboardService.getSummary();
    CHECK(summaryInit.totalResidents == 0);
    CHECK(summaryInit.totalServiceRequests == 0);
    CHECK(summaryInit.openServiceRequests == 0);

    // Step 1: Register an active resident
    csms::Resident resident("Radge Michael", "Emperador", "123 Academic Way", "09171234567", "radge@example.com");
    auto regResult = registrationService.registerResident(resident);
    CHECK(regResult.success == true);
    REQUIRE(regResult.resident.has_value());
    REQUIRE(regResult.resident->getId().has_value());
    const int residentId = *regResult.resident->getId();

    // Step 2: Submit a service request for this active resident
    csms::ServiceRequest request(residentId, "Certificate", "Barangay residency certificate", "2026-11-12");
    auto subResult = submissionService.submitServiceRequest(request);
    CHECK(subResult.success == true);
    REQUIRE(subResult.serviceRequest.has_value());
    REQUIRE(subResult.serviceRequest->getId().has_value());
    const int requestId = *subResult.serviceRequest->getId();
    CHECK(subResult.serviceRequest->getStatus() == "Pending");

    // Step 3: Advance workflow: Pending -> In Progress -> Completed
    auto progressResult = statusService.updateStatus(requestId, "In Progress");
    CHECK(progressResult.success == true);
    REQUIRE(progressResult.serviceRequest.has_value());
    CHECK(progressResult.serviceRequest->getStatus() == "In Progress");

    auto completeResult = statusService.updateStatus(requestId, "Completed");
    CHECK(completeResult.success == true);
    REQUIRE(completeResult.serviceRequest.has_value());
    CHECK(completeResult.serviceRequest->getStatus() == "Completed");

    // Step 4: Soft-deactivate the resident
    auto deactResult = deactivationService.deactivateResident(residentId);
    CHECK(deactResult.success == true);

    // Step 5: Verify live Dashboard reconciliation across all metrics
    auto summaryFinal = dashboardService.getSummary();
    CHECK(summaryFinal.totalResidents == 1);
    CHECK(summaryFinal.activeResidents == 0);
    CHECK(summaryFinal.inactiveResidents == 1);
    CHECK(summaryFinal.totalServiceRequests == 1);
    CHECK(summaryFinal.pendingServiceRequests == 0);
    CHECK(summaryFinal.inProgressServiceRequests == 0);
    CHECK(summaryFinal.completedServiceRequests == 1);
    CHECK(summaryFinal.cancelledServiceRequests == 0);
    CHECK(summaryFinal.openServiceRequests == 0); // Completed request is closed

    auto recentList = summaryFinal.recentServiceRequests;
    REQUIRE(recentList.size() == 1);
    CHECK(*recentList[0].getId() == requestId);
    CHECK(recentList[0].getStatus() == "Completed");

    // Step 6: Verify cross-feature eligibility protection: Inactive resident cannot submit new requests
    csms::ServiceRequest rejectedReq(residentId, "Clearance", "Barangay clearance attempt", "2026-11-12");
    auto rejectResult = submissionService.submitServiceRequest(rejectedReq);
    CHECK(rejectResult.success == false);
    CHECK(rejectResult.residentInactive == true);

    // Step 7: Verify SQLite disk persistence directly
    auto persistedRes = residentRepo->findById(residentId);
    REQUIRE(persistedRes.has_value());
    CHECK(persistedRes->getStatus() == "Inactive");

    auto persistedReq = requestRepo->findById(requestId);
    REQUIRE(persistedReq.has_value());
    CHECK(persistedReq->getStatus() == "Completed");

    // Confirm rejected submission was not persisted
    CHECK(requestRepo->countAll() == 1);
}
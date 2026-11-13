// T15 - System Hardening and Regression Testing
// Five student-designed tests covering all required hardening categories.

#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "controllers/ResidentController.h"
#include "controllers/ServiceRequestController.h"
#include "controllers/DashboardController.h"
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/DashboardService.h"
#include "services/ServiceRequestSubmissionService.h"
#include "models/ServiceRequestValidator.h"

namespace
{

struct HardeningTestDb
{
    std::filesystem::path path;

    explicit HardeningTestDb(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~HardeningTestDb()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

struct HardeningEnv
{
    HardeningTestDb db;
    std::shared_ptr<csms::ResidentRepository> residentRepo;
    std::shared_ptr<csms::ServiceRequestRepository> requestRepo;
    csms::ResidentController residentCtrl;
    csms::ServiceRequestController requestCtrl;
    csms::DashboardController dashCtrl;

    explicit HardeningEnv(const std::string& filename)
        : db(filename)
        , residentRepo(std::make_shared<csms::ResidentRepository>(db.path.string()))
        , requestRepo(std::make_shared<csms::ServiceRequestRepository>(db.path.string()))
        , residentCtrl(residentRepo)
        , requestCtrl(residentRepo, requestRepo)
        , dashCtrl(residentRepo, requestRepo)
    {
    }
};

using Params = std::map<std::string, std::string>;

drogon::HttpRequestPtr makeReq(drogon::HttpMethod method, const std::string& path, const Params& params = {})
{
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(method);
    req->setPath(path);
    for (const auto& [k, v] : params)
    {
        req->setParameter(k, v);
    }
    return req;
}

drogon::HttpResponsePtr dispatch(std::function<void(std::function<void(const drogon::HttpResponsePtr&)>&&)> action)
{
    std::promise<drogon::HttpResponsePtr> p;
    auto f = p.get_future();
    action([&p](const drogon::HttpResponsePtr& resp) { p.set_value(resp); });
    return f.get();
}

} // namespace

// -----------------------------------------------------------------------------
// Category 1: Invalid or Malformed Input
// Proves non-numeric / malformed route identifiers safely return controlled 404
// without throwing unhandled exceptions, crashing, or modifying database records.
// -----------------------------------------------------------------------------
DROGON_TEST(HardeningCategory1_MalformedIdentifiersHandledSafely)
{
    HardeningEnv env("t15_hardening_cat1.db");

    // Seed one valid resident
    auto resident = env.residentRepo->save(csms::Resident("Maria", "Clara", "123 Calle", "09171234567", "maria@example.com"));
    const int initialResidentCount = env.residentRepo->countAll();
    const int initialRequestCount = env.requestRepo->countAll();

    // 1. Malformed resident edit GET
    auto r1 = dispatch([&](auto&& cb) {
        env.residentCtrl.showEditForm(makeReq(drogon::Get, "/residents/abc/edit"), std::move(cb), "abc");
    });
    CHECK(r1->getStatusCode() == drogon::k404NotFound);
    CHECK(r1->getContentType() == drogon::CT_TEXT_HTML);
    CHECK(r1->getBody().find("Resident not found") != std::string::npos);

    // 2. Malformed resident update POST
    auto r2 = dispatch([&](auto&& cb) {
        Params params = {{"firstName", "Hacked"}, {"lastName", "Name"}, {"address", "Addr"}, {"contactNumber", "09171234567"}, {"email", "h@example.com"}};
        env.residentCtrl.updateResident(makeReq(drogon::Post, "/residents/not-a-number/edit", params), std::move(cb), "not-a-number");
    });
    CHECK(r2->getStatusCode() == drogon::k404NotFound);

    // 3. Malformed service request details GET
    auto r3 = dispatch([&](auto&& cb) {
        env.requestCtrl.show(makeReq(drogon::Get, "/service-requests/xyz123"), std::move(cb), "xyz123");
    });
    CHECK(r3->getStatusCode() == drogon::k404NotFound);
    CHECK(r3->getBody().find("Service Request not found") != std::string::npos);

    // 4. Malformed service request status POST
    auto r4 = dispatch([&](auto&& cb) {
        env.requestCtrl.startProcessing(makeReq(drogon::Post, "/service-requests/bad-id/start"), std::move(cb), "bad-id");
    });
    CHECK(r4->getStatusCode() == drogon::k404NotFound);

    // Verify persistence state is completely untouched
    CHECK(env.residentRepo->countAll() == initialResidentCount);
    CHECK(env.requestRepo->countAll() == initialRequestCount);
}

// -----------------------------------------------------------------------------
// Category 2: Rejected Mutation Protects Persistence
// Proves that when a form update is rejected due to invalid fields, the entire
// operation is atomic: zero partial updates occur, and persisted rows remain intact.
// -----------------------------------------------------------------------------
DROGON_TEST(HardeningCategory2_RejectedMutationProtectsPersistence)
{
    HardeningEnv env("t15_hardening_cat2.db");

    // Seed original resident
    auto saved = env.residentRepo->save(csms::Resident("OriginalFirst", "OriginalLast", "Original Address", "09181112222", "orig@example.com"));
    const int residentId = *saved.getId();

    // Attempt update with valid first name, but INVALID contact number format
    Params invalidUpdate = {
        {"firstName", "NewFirstName"},
        {"lastName", "OriginalLast"},
        {"address", "Original Address"},
        {"contactNumber", "invalid_phone_123"},
        {"email", "orig@example.com"}
    };

    auto resp = dispatch([&](auto&& cb) {
        env.residentCtrl.updateResident(makeReq(drogon::Post, "/residents/" + std::to_string(residentId) + "/edit", invalidUpdate), std::move(cb), std::to_string(residentId));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(resp->getBody().find("Contact number must follow the format 09XXXXXXXXX.") != std::string::npos);

    // Verify persisted database record was NOT partially updated
    auto refetched = env.residentRepo->findById(residentId);
    CHECK(refetched.has_value());
    CHECK(refetched->getFirstName() == "OriginalFirst"); // NOT NewFirstName
    CHECK(refetched->getLastName() == "OriginalLast");
    CHECK(refetched->getAddress() == "Original Address");
    CHECK(refetched->getContactNumber() == "09181112222");
    CHECK(refetched->getEmail() == "orig@example.com");
    CHECK(refetched->getStatus() == "Active");
}

// -----------------------------------------------------------------------------
// Category 3: Read-Only Request Safety
// Proves that repeated GET requests across all three modules (Dashboard,
// Residents, Service Requests) never mutate database state or change counts.
// -----------------------------------------------------------------------------
DROGON_TEST(HardeningCategory3_ReadOnlyRequestsDoNotMutatePersistence)
{
    HardeningEnv env("t15_hardening_cat3.db");

    // Seed database with sample data
    auto resident = env.residentRepo->save(csms::Resident("Jose", "Rizal", "Calamba", "09171234567", "jose@example.com"));
    env.requestRepo->save(csms::ServiceRequest(*resident.getId(), "Clearance", "Barangay clearance", "2026-11-01", "Pending"));

    const int initialResidentCount = env.residentRepo->countAll();
    const int initialRequestCount = env.requestRepo->countAll();

    // Issue repeated GET requests to every read endpoint
    for (int i = 0; i < 3; ++i)
    {
        auto rDash = dispatch([&](auto&& cb) { env.dashCtrl.index(makeReq(drogon::Get, "/dashboard"), std::move(cb)); });
        CHECK(rDash->getStatusCode() == drogon::k200OK);

        auto rResIndex = dispatch([&](auto&& cb) { env.residentCtrl.index(makeReq(drogon::Get, "/residents"), std::move(cb)); });
        CHECK(rResIndex->getStatusCode() == drogon::k200OK);

        auto rReqIndex = dispatch([&](auto&& cb) { env.requestCtrl.index(makeReq(drogon::Get, "/service-requests"), std::move(cb)); });
        CHECK(rReqIndex->getStatusCode() == drogon::k200OK);

        auto rReqDetail = dispatch([&](auto&& cb) { env.requestCtrl.show(makeReq(drogon::Get, "/service-requests/1"), std::move(cb), "1"); });
        CHECK(rReqDetail->getStatusCode() == drogon::k200OK);
    }

    // Assert zero database modifications
    CHECK(env.residentRepo->countAll() == initialResidentCount);
    CHECK(env.requestRepo->countAll() == initialRequestCount);
    auto checkReq = env.requestRepo->findById(1);
    CHECK(checkReq.has_value());
    CHECK(checkReq->getStatus() == "Pending");
}

// -----------------------------------------------------------------------------
// Category 4: Cross-Feature Regression
// Proves that soft-deactivating a resident immediately:
//   1. Rejects new Service Request submissions with residentInactive = true (T09)
//   2. Updates the Dashboard Active/Inactive counts (T14)
//   3. Leaves existing pending service requests intact and queryable (T10/T11)
// -----------------------------------------------------------------------------
DROGON_TEST(HardeningCategory4_CrossFeatureDeactivationIntegrity)
{
    HardeningEnv env("t15_hardening_cat4.db");

    // 1. Create active resident and a pending service request
    auto resident = env.residentRepo->save(csms::Resident("Andres", "Bonifacio", "Tondo", "09171234567", "andres@example.com"));
    const int residentId = *resident.getId();
    auto request = env.requestRepo->save(csms::ServiceRequest(residentId, "Assistance", "Emergency food assistance", "2026-11-02", "Pending"));
    const int requestId = *request.getId();

    // Verify baseline dashboard counts
    csms::DashboardService dashService(*env.residentRepo, *env.requestRepo);
    auto summaryBefore = dashService.getSummary();
    CHECK(summaryBefore.activeResidents == 1);
    CHECK(summaryBefore.inactiveResidents == 0);
    CHECK(summaryBefore.openServiceRequests == 1);

    // 2. Deactivate the resident
    bool deactivated = env.residentRepo->deactivateById(residentId);
    CHECK(deactivated == true);

    // 3. Verify Dashboard immediately reflects the change
    auto summaryAfter = dashService.getSummary();
    CHECK(summaryAfter.totalResidents == 1);
    CHECK(summaryAfter.activeResidents == 0);
    CHECK(summaryAfter.inactiveResidents == 1);
    CHECK(summaryAfter.openServiceRequests == 1); // Existing request remains open

    // 4. Attempt to submit a NEW service request for the now-inactive resident
    csms::ServiceRequestValidator validator;
    csms::ServiceRequestSubmissionService submissionService(validator, *env.residentRepo, *env.requestRepo);
    csms::ServiceRequest newReq(residentId, "Permit", "Business permit", "2026-11-03");
    auto subResult = submissionService.submitServiceRequest(newReq);

    CHECK(subResult.success == false);
    CHECK(subResult.residentInactive == true);
    CHECK(env.requestRepo->countAll() == 1); // Not saved to database

    // 5. Existing service request can still transition to In Progress (T10 rule)
    auto rStart = dispatch([&](auto&& cb) {
        env.requestCtrl.startProcessing(makeReq(drogon::Post, "/service-requests/" + std::to_string(requestId) + "/start"), std::move(cb), std::to_string(requestId));
    });
    CHECK(rStart->getStatusCode() == drogon::k303SeeOther);
    CHECK(env.requestRepo->findById(requestId)->getStatus() == "In Progress");
}

// -----------------------------------------------------------------------------
// Category 5: Repeatability or Boundary Behavior
// Proves exact boundary behavior for the 5-item recent service requests limit:
// when 7 requests exist across various dates, exactly 5 newest are returned
// in deterministic dateRequested DESC, id DESC order.
// -----------------------------------------------------------------------------
DROGON_TEST(HardeningCategory5_RecentRequestsBoundaryAndOrdering)
{
    HardeningEnv env("t15_hardening_cat5.db");

    auto resident = env.residentRepo->save(csms::Resident("Apolinario", "Mabini", "Batangas", "09171234567", "mabini@example.com"));
    const int residentId = *resident.getId();

    // Insert 7 service requests with varying dates and IDs
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 1", "Req 1", "2026-10-01", "Pending"));    // ID 1
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 2", "Req 2", "2026-10-05", "Pending"));    // ID 2
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 3", "Req 3", "2026-10-10", "Pending"));    // ID 3
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 4", "Req 4", "2026-10-15", "Completed"));  // ID 4
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 5", "Req 5", "2026-10-20", "Cancelled"));  // ID 5
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 6", "Req 6", "2026-10-25", "In Progress"));// ID 6
    env.requestRepo->save(csms::ServiceRequest(residentId, "Type 7", "Req 7", "2026-10-25", "Pending"));    // ID 7 (same date as 6, higher ID)

    CHECK(env.requestRepo->countAll() == 7);

    // Call findRecent(5)
    auto recent = env.requestRepo->findRecent(5);
    CHECK(recent.size() == 5);

    // Verify ordering: 2026-10-25 id:7, 2026-10-25 id:6, 2026-10-20 id:5, 2026-10-15 id:4, 2026-10-10 id:3
    CHECK(*recent[0].getId() == 7);
    CHECK(recent[0].getDateRequested() == "2026-10-25");

    CHECK(*recent[1].getId() == 6);
    CHECK(recent[1].getDateRequested() == "2026-10-25");

    CHECK(*recent[2].getId() == 5);
    CHECK(recent[2].getDateRequested() == "2026-10-20");

    CHECK(*recent[3].getId() == 4);
    CHECK(recent[3].getDateRequested() == "2026-10-15");

    CHECK(*recent[4].getId() == 3);
    CHECK(recent[4].getDateRequested() == "2026-10-10");

    // Confirm boundary limit <= 0 safely returns empty vector
    CHECK(env.requestRepo->findRecent(0).empty());
    CHECK(env.requestRepo->findRecent(-1).empty());
}
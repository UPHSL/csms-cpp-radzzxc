// T13 - Service Request Management Interface: presentation/integration tests.
// Each test drives ServiceRequestController against a real, isolated SQLite database,
// so it exercises the controller, the CSP views, and the T09/T10/T11 services together.

#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "controllers/ServiceRequestController.h"
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"

namespace
{

// Temporary database file, removed before and after each test
struct SrUiTestDb
{
    std::filesystem::path path;

    explicit SrUiTestDb(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~SrUiTestDb()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

// Everything one test needs: database, repositories, and the controller under test
struct SrUi
{
    SrUiTestDb db;
    std::shared_ptr<csms::ResidentRepository> residents;
    std::shared_ptr<csms::ServiceRequestRepository> requests;
    csms::ServiceRequestController controller;

    explicit SrUi(const std::string& filename)
        : db(filename)
        , residents(std::make_shared<csms::ResidentRepository>(db.path.string()))
        , requests(std::make_shared<csms::ServiceRequestRepository>(db.path.string()))
        , controller(residents, requests)
    {
    }

    int addResident(const std::string& firstName, bool active = true)
    {
        auto saved = residents->save(csms::Resident(firstName, "Santos", "1 Luna St", "09171234567", "test@example.com"));
        const int id = saved.getId().value();
        if (!active)
        {
            residents->deactivateById(id);
        }
        return id;
    }

    int addRequest(int residentId, const std::string& serviceType, const std::string& description,
                   const std::string& date, const std::string& status = "Pending")
    {
        return requests->save(csms::ServiceRequest(residentId, serviceType, description, date, status)).getId().value();
    }

    std::string statusOf(int id)
    {
        return requests->findById(id)->getStatus();
    }
};

using Params = std::map<std::string, std::string>;

drogon::HttpRequestPtr makeRequest(drogon::HttpMethod method, const std::string& path, const Params& params = {})
{
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(method);
    req->setPath(path);
    for (const auto& [key, value] : params)
    {
        req->setParameter(key, value);
    }
    return req;
}

// Runs a controller action and waits for its response
drogon::HttpResponsePtr dispatch(std::function<void(std::function<void(const drogon::HttpResponsePtr&)>&&)> action)
{
    std::promise<drogon::HttpResponsePtr> promise;
    auto future = promise.get_future();
    action([&promise](const drogon::HttpResponsePtr& resp) { promise.set_value(resp); });
    return future.get();
}

drogon::HttpResponsePtr getIndex(SrUi& ui, const Params& params = {})
{
    auto req = makeRequest(drogon::Get, "/service-requests", params);
    return dispatch([&](auto&& cb) { ui.controller.index(req, std::move(cb)); });
}

drogon::HttpResponsePtr getDetail(SrUi& ui, const std::string& id)
{
    auto req = makeRequest(drogon::Get, "/service-requests/" + id);
    return dispatch([&](auto&& cb) { ui.controller.show(req, std::move(cb), id); });
}

drogon::HttpResponsePtr postSubmit(SrUi& ui, const Params& params)
{
    auto req = makeRequest(drogon::Post, "/service-requests", params);
    return dispatch([&](auto&& cb) { ui.controller.submit(req, std::move(cb)); });
}

drogon::HttpResponsePtr postAction(SrUi& ui, int id, const std::string& action, const Params& params = {})
{
    const std::string idStr = std::to_string(id);
    auto req = makeRequest(drogon::Post, "/service-requests/" + idStr + "/" + action, params);
    return dispatch([&](auto&& cb) {
        if (action == "start") ui.controller.startProcessing(req, std::move(cb), idStr);
        else if (action == "complete") ui.controller.complete(req, std::move(cb), idStr);
        else ui.controller.cancel(req, std::move(cb), idStr);
    });
}

std::string bodyOf(const drogon::HttpResponsePtr& resp)
{
    return std::string(resp->getBody());
}

bool contains(const std::string& body, const std::string& text)
{
    return body.find(text) != std::string::npos;
}

// Form markup for one workflow action button of one request
std::string actionForm(int id, const std::string& action)
{
    return "action=\"/service-requests/" + std::to_string(id) + "/" + action + "\"";
}

} // namespace

// -----------------------------------------------------------------------------
// Test 1: Service Request Management Page Loads
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiManagementPageLoads)
{
    SrUi ui("t13_test1.db");

    auto resp = getIndex(ui);
    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(resp->getContentType() == drogon::CT_TEXT_HTML);

    const std::string body = bodyOf(resp);
    CHECK(contains(body, "<h1 class=\"serif\">Service Request Management</h1>"));
    CHECK(contains(body, "href=\"/service-requests/new\""));
    CHECK(contains(body, "name=\"search\""));
    CHECK(contains(body, "<option value=\"\" selected>All Statuses</option>"));
    // Empty list renders a friendly message
    CHECK(contains(body, "No Service Requests found."));
    // Navigation between the two modules
    CHECK(contains(body, "href=\"/residents\""));
}

// -----------------------------------------------------------------------------
// Test 2: Persisted Service Requests Are Displayed
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiPersistedRequestsAreDisplayed)
{
    SrUi ui("t13_test2.db");
    const int residentId = ui.addResident("Maria");
    const int first = ui.addRequest(residentId, "Barangay Clearance", "Employment requirement", "2026-10-01");
    const int second = ui.addRequest(residentId, "Certificate of Indigency", "Medical assistance", "2026-10-02");

    const std::string body = bodyOf(getIndex(ui));

    CHECK(contains(body, "Barangay Clearance"));
    CHECK(contains(body, "Certificate of Indigency"));
    CHECK(contains(body, "2026-10-01"));
    CHECK(contains(body, "<td class=\"nowrap\" data-label=\"Resident ID\">" + std::to_string(residentId) + "</td>"));
    CHECK(contains(body, ">Pending</span>"));
    CHECK(contains(body, "href=\"/service-requests/" + std::to_string(first) + "\""));
    CHECK(contains(body, "href=\"/service-requests/" + std::to_string(second) + "\""));
}

// -----------------------------------------------------------------------------
// Test 3: Required Ordering Is Preserved (dateRequested DESC, then id DESC)
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiRequiredOrderingIsPreserved)
{
    SrUi ui("t13_test3.db");
    // Saved deliberately out of order
    ui.addRequest(1, "Service A", "Desc A", "2026-09-15");
    ui.addRequest(1, "Service B", "Desc B", "2026-09-18");
    ui.addRequest(1, "Service C", "Desc C", "2026-09-18");
    ui.addRequest(1, "Service D", "Desc D", "2026-09-10");

    const std::string body = bodyOf(getIndex(ui));
    const auto posC = body.find("Service C");
    const auto posB = body.find("Service B");
    const auto posA = body.find("Service A");
    const auto posD = body.find("Service D");

    CHECK(posC != std::string::npos);
    CHECK(posC < posB);  // same date: higher ID first
    CHECK(posB < posA);
    CHECK(posA < posD);
}

// -----------------------------------------------------------------------------
// Test 4: Text Search Uses T11 Behavior (partial, case-insensitive, trimmed)
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiTextSearchUsesT11Behavior)
{
    SrUi ui("t13_test4.db");
    ui.addRequest(1, "Barangay Clearance", "Employment requirement", "2026-10-01");
    ui.addRequest(1, "Certificate of Indigency", "Medical assistance", "2026-10-02");

    std::string body = bodyOf(getIndex(ui, {{"search", "  cLeAr  "}}));
    CHECK(contains(body, "Barangay Clearance"));
    CHECK(!contains(body, "Certificate of Indigency"));
    // The search term stays visible in the box
    CHECK(contains(body, "value=\"  cLeAr  \""));

    // Description is searched too
    body = bodyOf(getIndex(ui, {{"search", "MEDICAL"}}));
    CHECK(contains(body, "Certificate of Indigency"));
    CHECK(!contains(body, "Barangay Clearance"));

    // Blank search lists everything; no-match search renders the empty message
    body = bodyOf(getIndex(ui, {{"search", "   "}}));
    CHECK(contains(body, "Barangay Clearance"));
    CHECK(contains(body, "Certificate of Indigency"));
    body = bodyOf(getIndex(ui, {{"search", "zzz-no-match"}}));
    CHECK(contains(body, "No Service Requests found."));
}

// -----------------------------------------------------------------------------
// Test 5: Status Filter Uses T11 Behavior
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiStatusFilterUsesT11Behavior)
{
    SrUi ui("t13_test5.db");
    ui.addRequest(1, "Request Pending", "Desc", "2026-10-01", "Pending");
    ui.addRequest(1, "Request Working", "Desc", "2026-10-02", "In Progress");
    ui.addRequest(1, "Request Done", "Desc", "2026-10-03", "Completed");
    ui.addRequest(1, "Request Stopped", "Desc", "2026-10-04", "Cancelled");

    const std::map<std::string, std::string> expected = {
        {"Pending", "Request Pending"},
        {"In Progress", "Request Working"},
        {"Completed", "Request Done"},
        {"Cancelled", "Request Stopped"},
    };
    for (const auto& [status, serviceType] : expected)
    {
        const std::string body = bodyOf(getIndex(ui, {{"status", status}}));
        CHECK(contains(body, serviceType));
        for (const auto& [otherStatus, otherType] : expected)
        {
            if (otherStatus != status)
            {
                CHECK(!contains(body, otherType));
            }
        }
        // The chosen status stays selected
        CHECK(contains(body, "<option value=\"" + status + "\" selected>"));
    }

    // "All Statuses" (empty value) shows every status
    std::string body = bodyOf(getIndex(ui, {{"status", ""}}));
    CHECK(contains(body, "Request Pending"));
    CHECK(contains(body, "Request Stopped"));

    // An unknown status from a hand-typed URL is handled, not a server error
    auto resp = getIndex(ui, {{"status", "Approved"}});
    CHECK(resp->getStatusCode() == drogon::k200OK);
    body = bodyOf(resp);
    CHECK(contains(body, "is not a valid status filter"));
    CHECK(contains(body, "Request Pending"));
}

// -----------------------------------------------------------------------------
// Test 6: Combined Search and Status Filter Works (AND, not OR)
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiCombinedSearchAndFilterWorks)
{
    SrUi ui("t13_test6.db");
    ui.addRequest(1, "Barangay Clearance", "Matches both", "2026-10-01", "Pending");
    ui.addRequest(1, "Barangay Clearance", "Text only", "2026-10-02", "Completed");
    ui.addRequest(1, "Community Assistance", "Status only", "2026-10-03", "Pending");
    ui.addRequest(1, "Community Assistance", "Neither", "2026-10-04", "Cancelled");

    const std::string body = bodyOf(getIndex(ui, {{"search", "clearance"}, {"status", "Pending"}}));

    CHECK(contains(body, "Matches both"));
    CHECK(!contains(body, "Text only"));
    CHECK(!contains(body, "Status only"));
    CHECK(!contains(body, "Neither"));
}

// -----------------------------------------------------------------------------
// Test 7: Submission Form Is Accessible
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiSubmissionFormIsAccessible)
{
    SrUi ui("t13_test7.db");
    auto req = makeRequest(drogon::Get, "/service-requests/new");
    auto resp = dispatch([&](auto&& cb) { ui.controller.showSubmitForm(req, std::move(cb)); });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    const std::string body = bodyOf(resp);

    CHECK(contains(body, "<h1 class=\"serif\">Submit Service Request</h1>"));
    CHECK(contains(body, "name=\"residentId\""));
    CHECK(contains(body, "name=\"serviceType\""));
    CHECK(contains(body, "<textarea id=\"description\" name=\"description\""));
    CHECK(contains(body, "type=\"date\" id=\"dateRequested\" name=\"dateRequested\""));
    // No Service Request ID or status inputs
    CHECK(!contains(body, "name=\"id\""));
    CHECK(!contains(body, "name=\"status\""));
    CHECK(contains(body, "<label for=\"residentId\">Resident ID</label>"));
}

// -----------------------------------------------------------------------------
// Test 8: Valid Submission Persists a Pending Request
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiValidSubmissionPersistsPendingRequest)
{
    SrUi ui("t13_test8.db");
    const int residentId = ui.addResident("Maria");

    auto resp = postSubmit(ui, {
        {"residentId", std::to_string(residentId)},
        {"serviceType", "Barangay Clearance"},
        {"description", "Requesting clearance for employment."},
        {"dateRequested", "2026-10-15"},
    });

    // Post/Redirect/Get back to the list with a success message
    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(resp->getHeader("Location") == "/service-requests?msg=submitted");

    const auto all = ui.requests->findAll();
    CHECK(all.size() == 1);
    CHECK(all[0].getId().has_value());
    CHECK(all[0].getResidentId() == residentId);
    CHECK(all[0].getServiceType() == "Barangay Clearance");
    CHECK(all[0].getDescription() == "Requesting clearance for employment.");
    CHECK(all[0].getDateRequested() == "2026-10-15");
    CHECK(all[0].getStatus() == "Pending");

    const std::string body = bodyOf(getIndex(ui, {{"msg", "submitted"}}));
    CHECK(contains(body, "Service Request submitted successfully."));
    CHECK(contains(body, "Barangay Clearance"));
}

// -----------------------------------------------------------------------------
// Test 9: Invalid Submission Shows Validation Feedback
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiInvalidSubmissionShowsValidationFeedback)
{
    SrUi ui("t13_test9.db");
    const int residentId = ui.addResident("Maria");

    auto resp = postSubmit(ui, {
        {"residentId", std::to_string(residentId)},
        {"serviceType", "   "},
        {"description", "Keep this text"},
        {"dateRequested", "not-a-date"},
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    const std::string body = bodyOf(resp);
    CHECK(contains(body, "Please fix the following:"));
    CHECK(contains(body, "Service type is required."));
    CHECK(contains(body, "Date requested must be a valid date (YYYY-MM-DD)."));
    CHECK(!contains(body, "Description is required."));
    // Entered values are kept for correction
    CHECK(contains(body, ">Keep this text</textarea>"));
    CHECK(ui.requests->findAll().empty());

    // A Resident ID that is not a number is reported, not crashed on
    resp = postSubmit(ui, {
        {"residentId", "12abc"},
        {"serviceType", "Barangay Clearance"},
        {"description", "Desc"},
        {"dateRequested", "2026-10-15"},
    });
    CHECK(contains(bodyOf(resp), "Resident ID must be a whole number greater than 0."));
    CHECK(ui.requests->findAll().empty());
}

// -----------------------------------------------------------------------------
// Test 10: Missing Resident Submission Is Rejected
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiMissingResidentSubmissionIsRejected)
{
    SrUi ui("t13_test10.db");

    auto resp = postSubmit(ui, {
        {"residentId", "9999"},
        {"serviceType", "Barangay Clearance"},
        {"description", "Desc"},
        {"dateRequested", "2026-10-15"},
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    const std::string body = bodyOf(resp);
    CHECK(contains(body, "Resident not found."));
    CHECK(contains(body, "value=\"9999\""));
    CHECK(ui.requests->findAll().empty());
}

// -----------------------------------------------------------------------------
// Test 11: Inactive Resident Submission Is Rejected
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiInactiveResidentSubmissionIsRejected)
{
    SrUi ui("t13_test11.db");
    const int inactiveId = ui.addResident("Ana", false);

    auto resp = postSubmit(ui, {
        {"residentId", std::to_string(inactiveId)},
        {"serviceType", "Barangay Clearance"},
        {"description", "Desc"},
        {"dateRequested", "2026-10-15"},
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(contains(bodyOf(resp), "Inactive Residents cannot submit new Service Requests."));
    CHECK(ui.requests->findAll().empty());
    // The Resident is not reactivated
    CHECK(ui.residents->findById(inactiveId)->getStatus() == "Inactive");
}

// -----------------------------------------------------------------------------
// Test 12: Existing Service Request Details Can Be Viewed
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiDetailsCanBeViewed)
{
    SrUi ui("t13_test12.db");
    const std::string longDescription =
        "Requesting barangay clearance for employment requirements at a new job in Quezon City next month.";
    const int id = ui.addRequest(42, "Barangay Clearance", longDescription, "2026-10-25", "In Progress");

    auto resp = getDetail(ui, std::to_string(id));
    CHECK(resp->getStatusCode() == drogon::k200OK);
    const std::string body = bodyOf(resp);

    CHECK(contains(body, "<dt>Service Request ID</dt><dd>" + std::to_string(id) + "</dd>"));
    CHECK(contains(body, "<dt>Resident ID</dt><dd>42</dd>"));
    CHECK(contains(body, "<dt>Service type</dt><dd>Barangay Clearance</dd>"));
    // The full description, not the shortened list version
    CHECK(contains(body, longDescription));
    CHECK(contains(body, "<dt>Date requested</dt><dd>2026-10-25</dd>"));
    CHECK(contains(body, "<span class=\"status progress\">In Progress</span>"));
}

// -----------------------------------------------------------------------------
// Test 13: Missing Service Request View Is Handled Safely
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiMissingRequestViewIsHandledSafely)
{
    SrUi ui("t13_test13.db");

    auto resp = getDetail(ui, "99999");
    CHECK(resp->getStatusCode() == drogon::k404NotFound);
    CHECK(contains(bodyOf(resp), "Service Request not found"));

    resp = getDetail(ui, "abc");
    CHECK(resp->getStatusCode() == drogon::k404NotFound);

    // Processing a missing request is also a controlled 404, and nothing is created
    resp = postAction(ui, 99999, "start");
    CHECK(resp->getStatusCode() == drogon::k404NotFound);
    CHECK(ui.requests->findAll().empty());
}

// -----------------------------------------------------------------------------
// Test 14: Pending Request Can Start Processing
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiPendingRequestCanStartProcessing)
{
    SrUi ui("t13_test14.db");
    const int id = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01");

    // A Pending request offers Start Processing and Cancel, but not Complete
    std::string body = bodyOf(getIndex(ui));
    CHECK(contains(body, actionForm(id, "start")));
    CHECK(contains(body, actionForm(id, "cancel")));
    CHECK(!contains(body, actionForm(id, "complete")));

    auto resp = postAction(ui, id, "start");
    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(resp->getHeader("Location") == "/service-requests/" + std::to_string(id) + "?msg=started");

    const auto updated = ui.requests->findById(id);
    CHECK(updated->getStatus() == "In Progress");
    // Everything else is preserved
    CHECK(updated->getId().value() == id);
    CHECK(updated->getResidentId() == 7);
    CHECK(updated->getServiceType() == "Barangay Clearance");
    CHECK(updated->getDescription() == "Employment");
    CHECK(updated->getDateRequested() == "2026-10-01");

    body = bodyOf(getDetail(ui, std::to_string(id)));
    CHECK(contains(body, "<span class=\"status progress\">In Progress</span>"));
}

// -----------------------------------------------------------------------------
// Test 15: Pending Request Can Be Cancelled
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiPendingRequestCanBeCancelled)
{
    SrUi ui("t13_test15.db");
    const int id = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01");

    auto resp = postAction(ui, id, "cancel");
    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(ui.statusOf(id) == "Cancelled");
}

// -----------------------------------------------------------------------------
// Test 16: In-Progress Request Can Be Completed
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiInProgressRequestCanBeCompleted)
{
    SrUi ui("t13_test16.db");
    const int id = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01");

    // Move to In Progress through the interface, as a user would
    postAction(ui, id, "start");
    CHECK(ui.statusOf(id) == "In Progress");

    // An In Progress request offers Complete and Cancel, but not Start Processing
    std::string body = bodyOf(getIndex(ui));
    CHECK(contains(body, actionForm(id, "complete")));
    CHECK(contains(body, actionForm(id, "cancel")));
    CHECK(!contains(body, actionForm(id, "start")));

    auto resp = postAction(ui, id, "complete");
    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(ui.statusOf(id) == "Completed");

    body = bodyOf(getDetail(ui, std::to_string(id) ));
    CHECK(contains(body, "It is closed and has no further actions."));
}

// -----------------------------------------------------------------------------
// Test 17: In-Progress Request Can Be Cancelled
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiInProgressRequestCanBeCancelled)
{
    SrUi ui("t13_test17.db");
    const int id = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01");
    postAction(ui, id, "start");

    // Started from the list page, so it returns to the list
    auto resp = postAction(ui, id, "cancel", {{"returnTo", "list"}});
    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(resp->getHeader("Location") == "/service-requests?msg=cancelled");
    CHECK(ui.statusOf(id) == "Cancelled");
    CHECK(contains(bodyOf(getIndex(ui, {{"msg", "cancelled"}})), "Service Request cancelled successfully."));
}

// -----------------------------------------------------------------------------
// Test 18: Invalid Direct Transition Is Still Rejected by T10
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiInvalidDirectTransitionIsRejected)
{
    SrUi ui("t13_test18.db");
    const int pendingId = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01");

    // Pending -> Completed directly (no button is shown for this, but a hand-made POST could try)
    auto resp = postAction(ui, pendingId, "complete");
    CHECK(resp->getStatusCode() == drogon::k409Conflict);
    CHECK(contains(bodyOf(resp), "No changes were made."));
    CHECK(ui.statusOf(pendingId) == "Pending");

    // Terminal requests cannot move again
    const int completedId = ui.addRequest(7, "Certificate of Indigency", "Medical", "2026-10-02", "Completed");
    resp = postAction(ui, completedId, "cancel");
    CHECK(resp->getStatusCode() == drogon::k409Conflict);
    CHECK(ui.statusOf(completedId) == "Completed");

    const int cancelledId = ui.addRequest(7, "Business Permit", "Renewal", "2026-10-03", "Cancelled");
    resp = postAction(ui, cancelledId, "start");
    CHECK(resp->getStatusCode() == drogon::k409Conflict);
    CHECK(ui.statusOf(cancelledId) == "Cancelled");
}

// -----------------------------------------------------------------------------
// Test 19: Completed and Cancelled Requests Remain Visible
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiTerminalRequestsRemainVisible)
{
    SrUi ui("t13_test19.db");
    const int completedId = ui.addRequest(7, "Barangay Clearance", "Finished request", "2026-10-01", "Completed");
    const int cancelledId = ui.addRequest(7, "Community Assistance", "Withdrawn request", "2026-10-02", "Cancelled");

    // Listing
    std::string body = bodyOf(getIndex(ui));
    CHECK(contains(body, "<span class=\"status completed\">Completed</span>"));
    CHECK(contains(body, "<span class=\"status cancelled\">Cancelled</span>"));
    // No workflow actions for terminal requests
    for (const std::string action : {"start", "complete", "cancel"})
    {
        CHECK(!contains(body, actionForm(completedId, action)));
        CHECK(!contains(body, actionForm(cancelledId, action)));
    }

    // Searching and filtering
    CHECK(contains(bodyOf(getIndex(ui, {{"search", "finished"}})), "Barangay Clearance"));
    CHECK(contains(bodyOf(getIndex(ui, {{"status", "Cancelled"}})), "Community Assistance"));

    // Viewing
    auto resp = getDetail(ui, std::to_string(cancelledId));
    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(contains(bodyOf(resp), "This request is Cancelled. It is closed and has no further actions."));
}

// -----------------------------------------------------------------------------
// Test 20: GET Requests Do Not Change Service Request State
// -----------------------------------------------------------------------------
DROGON_TEST(SrUiGetRequestsDoNotChangeState)
{
    SrUi ui("t13_test20.db");
    const int pendingId = ui.addRequest(7, "Barangay Clearance", "Employment", "2026-10-01", "Pending");
    const int progressId = ui.addRequest(7, "Certificate of Indigency", "Medical", "2026-10-02", "In Progress");

    getIndex(ui);
    getIndex(ui, {{"search", "clearance"}});
    getIndex(ui, {{"status", "Pending"}});
    auto formReq = makeRequest(drogon::Get, "/service-requests/new");
    dispatch([&](auto&& cb) { ui.controller.showSubmitForm(formReq, std::move(cb)); });
    getDetail(ui, std::to_string(pendingId));
    getDetail(ui, std::to_string(progressId));

    CHECK(ui.requests->findAll().size() == 2);
    CHECK(ui.statusOf(pendingId) == "Pending");
    CHECK(ui.statusOf(progressId) == "In Progress");
}

// T14 - CSMS Dashboard: aggregate-query and presentation tests.
// Each test uses its own isolated SQLite database. It checks the numbers DashboardService builds
// from persistence-level COUNT queries, and the HTML that DashboardController renders from them.

#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "controllers/DashboardController.h"
#include "controllers/ServiceRequestController.h"
#include "models/Resident.h"
#include "models/ServiceRequest.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/DashboardService.h"
#include "services/ResidentDeactivationService.h"
#include "services/ServiceRequestStatusService.h"

namespace
{

// Temporary database file, removed before and after each test
struct DashTestDb
{
    std::filesystem::path path;

    explicit DashTestDb(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~DashTestDb()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

// Everything one test needs: database, repositories, the dashboard service and controller
struct Dash
{
    DashTestDb db;
    std::shared_ptr<csms::ResidentRepository> residents;
    std::shared_ptr<csms::ServiceRequestRepository> requests;
    csms::DashboardService service;
    csms::DashboardController controller;

    explicit Dash(const std::string& filename)
        : db(filename)
        , residents(std::make_shared<csms::ResidentRepository>(db.path.string()))
        , requests(std::make_shared<csms::ServiceRequestRepository>(db.path.string()))
        , service(*residents, *requests)
        , controller(residents, requests)
    {
    }

    int addResident(const std::string& firstName, bool active = true)
    {
        auto saved = residents->save(csms::Resident(firstName, "Reyes", "5 Mabini St", "09171234567", "dash@example.com"));
        const int id = saved.getId().value();
        if (!active)
        {
            residents->deactivateById(id);
        }
        return id;
    }

    int addRequest(int residentId, const std::string& date, const std::string& status = "Pending",
                   const std::string& serviceType = "Barangay Clearance")
    {
        return requests->save(csms::ServiceRequest(residentId, serviceType, "Needed for employment", date, status))
            .getId()
            .value();
    }
};

// Runs GET /dashboard through the controller and waits for the response
drogon::HttpResponsePtr getDashboard(Dash& dash)
{
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/dashboard");

    std::promise<drogon::HttpResponsePtr> promise;
    auto future = promise.get_future();
    dash.controller.index(req, [&promise](const drogon::HttpResponsePtr& resp) { promise.set_value(resp); });
    return future.get();
}

std::string bodyOf(const drogon::HttpResponsePtr& resp)
{
    return std::string(resp->getBody());
}

bool contains(const std::string& body, const std::string& text)
{
    return body.find(text) != std::string::npos;
}

// Markup of one dashboard number, e.g. metric("residents-total", 3) -> id="residents-total">3</
std::string metric(const std::string& id, int value)
{
    return "id=\"" + id + "\">" + std::to_string(value) + "</";
}

std::size_t countOccurrences(const std::string& body, const std::string& text)
{
    std::size_t count = 0;
    for (std::size_t pos = body.find(text); pos != std::string::npos; pos = body.find(text, pos + text.size()))
    {
        ++count;
    }
    return count;
}

// "View" link of one recent request
std::string viewLink(int id)
{
    return "class=\"btn btn-sm btn-view\" href=\"/service-requests/" + std::to_string(id) + "\"";
}

std::vector<int> idsOf(const std::vector<csms::ServiceRequest>& list)
{
    std::vector<int> ids;
    for (const auto& r : list)
    {
        ids.push_back(r.getId().value());
    }
    return ids;
}

// Full text of every persisted record, used to prove viewing the dashboard changes nothing
std::string snapshot(Dash& dash)
{
    std::string out;
    for (const auto& r : dash.residents->findAll())
    {
        out += "R|" + std::to_string(r.getId().value()) + "|" + r.getFirstName() + "|" + r.getLastName() + "|" +
               r.getAddress() + "|" + r.getContactNumber() + "|" + r.getEmail() + "|" + r.getStatus() + "\n";
    }
    for (const auto& s : dash.requests->findAll())
    {
        out += "S|" + std::to_string(s.getId().value()) + "|" + std::to_string(s.getResidentId()) + "|" +
               s.getServiceType() + "|" + s.getDescription() + "|" + s.getDateRequested() + "|" + s.getStatus() + "\n";
    }
    return out;
}

} // namespace

// -----------------------------------------------------------------------------
// Test 1: Dashboard Loads Successfully
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardLoadsSuccessfully)
{
    Dash dash("t14_test1.db");

    auto resp = getDashboard(dash);
    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(resp->getContentType() == drogon::CT_TEXT_HTML);

    const std::string body = bodyOf(resp);
    CHECK(contains(body, "<h1 class=\"serif\">CSMS Dashboard</h1>"));
    CHECK(contains(body, ">Residents</h2>"));
    CHECK(contains(body, ">Service Requests</h2>"));
    CHECK(contains(body, ">Open Service Requests</h2>"));
    CHECK(contains(body, ">Recent Service Requests</h2>"));
    // Shared navigation: Dashboard (current page), Residents, Service Requests
    CHECK(contains(body, "<a href=\"/dashboard\" class=\"on\" aria-current=\"page\">Dashboard</a>"));
    CHECK(contains(body, "<a href=\"/residents\">Residents</a>"));
    CHECK(contains(body, "<a href=\"/service-requests\">Service Requests</a>"));
    // Links into both management modules
    CHECK(contains(body, "href=\"/residents\">Open Resident Management</a>"));
    CHECK(contains(body, "href=\"/service-requests\">Open Service Request Management</a>"));
}

// -----------------------------------------------------------------------------
// Test 2: Empty Dashboard Displays Zero Counts
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardEmptyShowsZeroCounts)
{
    Dash dash("t14_test2.db");

    const auto s = dash.service.getSummary();
    CHECK(s.totalResidents == 0);
    CHECK(s.activeResidents == 0);
    CHECK(s.inactiveResidents == 0);
    CHECK(s.totalServiceRequests == 0);
    CHECK(s.pendingServiceRequests == 0);
    CHECK(s.inProgressServiceRequests == 0);
    CHECK(s.completedServiceRequests == 0);
    CHECK(s.cancelledServiceRequests == 0);
    CHECK(s.openServiceRequests == 0);
    CHECK(s.recentServiceRequests.empty());

    auto resp = getDashboard(dash);
    CHECK(resp->getStatusCode() == drogon::k200OK);
    const std::string body = bodyOf(resp);
    for (const std::string id : {"residents-total", "residents-active", "residents-inactive", "requests-total",
                                 "requests-pending", "requests-in-progress", "requests-completed",
                                 "requests-cancelled", "requests-open"})
    {
        CHECK(contains(body, metric(id, 0)));
    }
    // Safe empty state: a readable message, no table, no "null", no fake rows
    CHECK(contains(body, "No Service Requests available."));
    CHECK(!contains(body, "<table>"));
    CHECK(!contains(body, "null"));
    CHECK(dash.requests->countAll() == 0);
}

// -----------------------------------------------------------------------------
// Test 3: Total Resident Count Is Correct
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardTotalResidentCount)
{
    Dash dash("t14_test3.db");
    dash.addResident("Ana");
    dash.addResident("Ben");
    dash.addResident("Carla");
    dash.addResident("Dino", false);

    CHECK(dash.service.getSummary().totalResidents == 4);
    CHECK(contains(bodyOf(getDashboard(dash)), metric("residents-total", 4)));
}

// -----------------------------------------------------------------------------
// Test 4: Resident Status Counts Are Correct (Total = Active + Inactive)
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardResidentStatusCounts)
{
    Dash dash("t14_test4.db");
    dash.addResident("Ana");
    dash.addResident("Ben");
    dash.addResident("Carla");
    dash.addResident("Dino", false);
    dash.addResident("Ely", false);

    const auto s = dash.service.getSummary();
    CHECK(s.activeResidents == 3);
    CHECK(s.inactiveResidents == 2);
    CHECK(s.totalResidents == 5);
    CHECK(s.totalResidents == s.activeResidents + s.inactiveResidents);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("residents-total", 5)));
    CHECK(contains(body, metric("residents-active", 3)));
    CHECK(contains(body, metric("residents-inactive", 2)));
}

// -----------------------------------------------------------------------------
// Test 5: Total Service Request Count Is Correct
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardTotalServiceRequestCount)
{
    Dash dash("t14_test5.db");
    const int resident = dash.addResident("Ana");
    dash.addRequest(resident, "2026-01-05");
    dash.addRequest(resident, "2026-01-06", "In Progress");
    dash.addRequest(resident, "2026-01-07", "Completed");
    dash.addRequest(resident, "2026-01-08", "Cancelled");
    dash.addRequest(resident, "2026-01-09");
    dash.addRequest(resident, "2026-01-10");

    CHECK(dash.service.getSummary().totalServiceRequests == 6);
    CHECK(contains(bodyOf(getDashboard(dash)), metric("requests-total", 6)));
}

// -----------------------------------------------------------------------------
// Test 6: Service Request Status Counts Are Correct
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardServiceRequestStatusCounts)
{
    Dash dash("t14_test6.db");
    const int resident = dash.addResident("Ana");
    for (int i = 0; i < 3; ++i) dash.addRequest(resident, "2026-02-01", "Pending");
    for (int i = 0; i < 2; ++i) dash.addRequest(resident, "2026-02-02", "In Progress");
    for (int i = 0; i < 1; ++i) dash.addRequest(resident, "2026-02-03", "Completed");
    for (int i = 0; i < 4; ++i) dash.addRequest(resident, "2026-02-04", "Cancelled");

    const auto s = dash.service.getSummary();
    CHECK(s.pendingServiceRequests == 3);
    CHECK(s.inProgressServiceRequests == 2);
    CHECK(s.completedServiceRequests == 1);
    CHECK(s.cancelledServiceRequests == 4);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("requests-pending", 3)));
    CHECK(contains(body, metric("requests-in-progress", 2)));
    CHECK(contains(body, metric("requests-completed", 1)));
    CHECK(contains(body, metric("requests-cancelled", 4)));
    // Every metric is labelled with words, not only a color
    CHECK(contains(body, ">Pending</span></dt>"));
    CHECK(contains(body, ">In Progress</span></dt>"));
    CHECK(contains(body, ">Completed</span></dt>"));
    CHECK(contains(body, ">Cancelled</span></dt>"));
}

// -----------------------------------------------------------------------------
// Test 7: Service Request Counts Reconcile
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardServiceRequestCountsReconcile)
{
    Dash dash("t14_test7.db");
    const int resident = dash.addResident("Ana");
    const std::vector<std::string> statuses = {"Pending", "Completed", "In Progress", "Cancelled", "Pending",
                                               "Completed", "Completed", "In Progress", "Pending"};
    for (const auto& status : statuses)
    {
        dash.addRequest(resident, "2026-03-01", status);
    }

    const auto s = dash.service.getSummary();
    CHECK(s.totalServiceRequests == static_cast<int>(statuses.size()));
    CHECK(s.totalServiceRequests == s.pendingServiceRequests + s.inProgressServiceRequests +
                                        s.completedServiceRequests + s.cancelledServiceRequests);
}

// -----------------------------------------------------------------------------
// Test 8: Open Request Count Is Correct (Open = Pending + In Progress)
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardOpenRequestCount)
{
    Dash dash("t14_test8.db");
    const int resident = dash.addResident("Ana");
    dash.addRequest(resident, "2026-04-01", "Pending");
    dash.addRequest(resident, "2026-04-02", "Pending");
    dash.addRequest(resident, "2026-04-03", "In Progress");
    dash.addRequest(resident, "2026-04-04", "Completed");
    dash.addRequest(resident, "2026-04-05", "Completed");
    dash.addRequest(resident, "2026-04-06", "Cancelled");

    const auto s = dash.service.getSummary();
    CHECK(s.openServiceRequests == 3);
    CHECK(s.openServiceRequests == s.pendingServiceRequests + s.inProgressServiceRequests);
    // Completed and Cancelled are not open
    CHECK(s.openServiceRequests == s.totalServiceRequests - s.completedServiceRequests - s.cancelledServiceRequests);
    // "Open" is derived, never stored as a status
    CHECK(dash.requests->countByStatus("Open") == 0);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("requests-open", 3)));
    CHECK(contains(body, "Pending (2) + In Progress (1)"));
}

// -----------------------------------------------------------------------------
// Test 9: Resident Deactivation Is Reflected
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardReflectsResidentDeactivation)
{
    Dash dash("t14_test9.db");
    const int target = dash.addResident("Ana");
    dash.addResident("Ben");
    dash.addResident("Carla", false);

    const auto before = dash.service.getSummary();
    CHECK(before.activeResidents == 2);
    CHECK(before.inactiveResidents == 1);

    // Existing T07 business capability does the deactivation; the dashboard only observes it
    csms::ResidentDeactivationService deactivation(*dash.residents);
    REQUIRE(deactivation.deactivateResident(target).success);

    const auto after = dash.service.getSummary();
    CHECK(after.totalResidents == before.totalResidents);
    CHECK(after.activeResidents == before.activeResidents - 1);
    CHECK(after.inactiveResidents == before.inactiveResidents + 1);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("residents-total", 3)));
    CHECK(contains(body, metric("residents-active", 1)));
    CHECK(contains(body, metric("residents-inactive", 2)));
}

// -----------------------------------------------------------------------------
// Test 10: Service Request Transition Is Reflected (Pending -> In Progress)
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardReflectsPendingToInProgress)
{
    Dash dash("t14_test10.db");
    const int resident = dash.addResident("Ana");
    const int target = dash.addRequest(resident, "2026-05-01", "Pending");
    dash.addRequest(resident, "2026-05-02", "Pending");
    dash.addRequest(resident, "2026-05-03", "Completed");

    const auto before = dash.service.getSummary();

    // Existing T10 status workflow performs the legitimate transition
    csms::ServiceRequestStatusService statusService(*dash.requests);
    REQUIRE(statusService.updateStatus(target, "In Progress").success);

    const auto after = dash.service.getSummary();
    CHECK(after.pendingServiceRequests == before.pendingServiceRequests - 1);
    CHECK(after.inProgressServiceRequests == before.inProgressServiceRequests + 1);
    CHECK(after.completedServiceRequests == before.completedServiceRequests);
    CHECK(after.cancelledServiceRequests == before.cancelledServiceRequests);
    CHECK(after.totalServiceRequests == before.totalServiceRequests);
    // Still open, just in a different stage
    CHECK(after.openServiceRequests == before.openServiceRequests);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("requests-pending", 1)));
    CHECK(contains(body, metric("requests-in-progress", 1)));
    CHECK(contains(body, metric("requests-total", 3)));
}

// -----------------------------------------------------------------------------
// Test 11: Terminal Transition Updates Open Count
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardTerminalTransitionLowersOpenCount)
{
    Dash dash("t14_test11.db");
    const int resident = dash.addResident("Ana");
    const int toComplete = dash.addRequest(resident, "2026-06-01", "In Progress");
    const int toCancel = dash.addRequest(resident, "2026-06-02", "In Progress");
    dash.addRequest(resident, "2026-06-03", "Pending");

    csms::ServiceRequestStatusService statusService(*dash.requests);
    const auto start = dash.service.getSummary();
    CHECK(start.openServiceRequests == 3);

    // In Progress -> Completed
    REQUIRE(statusService.updateStatus(toComplete, "Completed").success);
    const auto afterComplete = dash.service.getSummary();
    CHECK(afterComplete.openServiceRequests == start.openServiceRequests - 1);
    CHECK(afterComplete.inProgressServiceRequests == start.inProgressServiceRequests - 1);
    CHECK(afterComplete.completedServiceRequests == start.completedServiceRequests + 1);
    CHECK(afterComplete.totalServiceRequests == start.totalServiceRequests);

    // In Progress -> Cancelled
    REQUIRE(statusService.updateStatus(toCancel, "Cancelled").success);
    const auto afterCancel = dash.service.getSummary();
    CHECK(afterCancel.openServiceRequests == afterComplete.openServiceRequests - 1);
    CHECK(afterCancel.cancelledServiceRequests == afterComplete.cancelledServiceRequests + 1);
    CHECK(afterCancel.totalServiceRequests == start.totalServiceRequests);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, metric("requests-open", 1)));
    CHECK(contains(body, metric("requests-total", 3)));
}

// -----------------------------------------------------------------------------
// Test 12: Recent Requests Return No More Than Five
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardRecentRequestsLimitedToFive)
{
    Dash dash("t14_test12.db");
    const int resident = dash.addResident("Ana");
    for (int day = 1; day <= 8; ++day)
    {
        dash.addRequest(resident, "2026-07-0" + std::to_string(day));
    }

    const auto s = dash.service.getSummary();
    CHECK(s.recentServiceRequests.size() == 5);
    CHECK(s.totalServiceRequests == 8);

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(countOccurrences(body, "class=\"btn btn-sm btn-view\"") == 5);
    CHECK(contains(body, "Showing 5 of 8 service requests"));

    // Fewer than five: all of them are shown
    Dash small("t14_test12b.db");
    const int other = small.addResident("Ben");
    small.addRequest(other, "2026-07-01");
    small.addRequest(other, "2026-07-02");
    CHECK(small.service.getSummary().recentServiceRequests.size() == 2);
    const std::string smallBody = bodyOf(getDashboard(small));
    CHECK(countOccurrences(smallBody, "class=\"btn btn-sm btn-view\"") == 2);
    CHECK(contains(smallBody, "Showing 2 service requests"));
}

// -----------------------------------------------------------------------------
// Test 13: Recent Requests Use Correct Ordering (dateRequested DESC, then id DESC)
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardRecentRequestsOrdering)
{
    Dash dash("t14_test13.db");
    const int resident = dash.addResident("Ana");
    const int a = dash.addRequest(resident, "2026-01-10");
    const int b = dash.addRequest(resident, "2026-03-05");
    const int c = dash.addRequest(resident, "2026-03-05");
    const int d = dash.addRequest(resident, "2026-02-01");
    const int e = dash.addRequest(resident, "2026-03-05");
    const int f = dash.addRequest(resident, "2025-12-31");
    const int g = dash.addRequest(resident, "2026-01-10");

    // Same date -> higher id first; the oldest two (a, f) fall outside the five
    const std::vector<int> expected = {e, c, b, d, g};
    CHECK(idsOf(dash.service.getSummary().recentServiceRequests) == expected);

    // The page lists them in the same order
    const std::string body = bodyOf(getDashboard(dash));
    std::size_t previous = 0;
    for (int id : expected)
    {
        const std::size_t pos = body.find(viewLink(id));
        REQUIRE(pos != std::string::npos);
        CHECK(pos > previous);
        previous = pos;
    }
    CHECK(!contains(body, viewLink(a)));
    CHECK(!contains(body, viewLink(f)));
}

// -----------------------------------------------------------------------------
// Test 14: Terminal Requests May Appear in Recent Results
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardRecentIncludesTerminalRequests)
{
    Dash dash("t14_test14.db");
    const int resident = dash.addResident("Ana");
    dash.addRequest(resident, "2026-08-01", "Pending");
    dash.addRequest(resident, "2026-08-02", "Pending");
    dash.addRequest(resident, "2026-08-03", "In Progress");
    dash.addRequest(resident, "2026-08-04", "Pending");
    const int completed = dash.addRequest(resident, "2026-08-10", "Completed");
    const int cancelled = dash.addRequest(resident, "2026-08-09", "Cancelled");

    const auto recent = dash.service.getSummary().recentServiceRequests;
    REQUIRE(recent.size() == 5);
    CHECK(recent[0].getId().value() == completed);
    CHECK(recent[0].getStatus() == "Completed");
    CHECK(recent[1].getId().value() == cancelled);
    CHECK(recent[1].getStatus() == "Cancelled");

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(contains(body, viewLink(completed)));
    CHECK(contains(body, viewLink(cancelled)));
    CHECK(contains(body, "<span class=\"status completed\">Completed</span></td>"));
    CHECK(contains(body, "<span class=\"status cancelled\">Cancelled</span></td>"));
}

// -----------------------------------------------------------------------------
// Test 15: Dashboard GET Does Not Modify Persistence
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardGetDoesNotModifyPersistence)
{
    Dash dash("t14_test15.db");
    const int ana = dash.addResident("Ana");
    dash.addResident("Ben", false);
    dash.addRequest(ana, "2026-09-01", "Pending");
    dash.addRequest(ana, "2026-09-02", "In Progress");
    dash.addRequest(ana, "2026-09-03", "Completed");
    dash.addRequest(ana, "2026-09-04", "Cancelled");

    const std::string before = snapshot(dash);
    const auto summaryBefore = dash.service.getSummary();

    // Refresh the dashboard several times
    for (int i = 0; i < 3; ++i)
    {
        CHECK(getDashboard(dash)->getStatusCode() == drogon::k200OK);
    }

    CHECK(snapshot(dash) == before);
    const auto summaryAfter = dash.service.getSummary();
    CHECK(summaryAfter.totalResidents == summaryBefore.totalResidents);
    CHECK(summaryAfter.activeResidents == summaryBefore.activeResidents);
    CHECK(summaryAfter.totalServiceRequests == summaryBefore.totalServiceRequests);
    CHECK(summaryAfter.pendingServiceRequests == summaryBefore.pendingServiceRequests);
    CHECK(summaryAfter.openServiceRequests == summaryBefore.openServiceRequests);
    // The page offers no forms that could change data
    CHECK(!contains(bodyOf(getDashboard(dash)), "<form"));
}

// -----------------------------------------------------------------------------
// Extra: Recent "View" opens the existing T13 details page
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardRecentViewUsesExistingDetailsPage)
{
    Dash dash("t14_test16.db");
    const int resident = dash.addResident("Ana");
    const int id = dash.addRequest(resident, "2026-10-01", "Pending", "Certificate of Residency");

    CHECK(contains(bodyOf(getDashboard(dash)), viewLink(id)));

    // The link target is served by the T13 ServiceRequestController::show
    csms::ServiceRequestController srController(dash.residents, dash.requests);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/service-requests/" + std::to_string(id));
    std::promise<drogon::HttpResponsePtr> promise;
    auto future = promise.get_future();
    srController.show(req, [&promise](const drogon::HttpResponsePtr& resp) { promise.set_value(resp); },
                      std::to_string(id));
    auto resp = future.get();
    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(contains(bodyOf(resp), "Certificate of Residency"));
}

// -----------------------------------------------------------------------------
// Extra: Repository aggregate queries and recent-request limit work on their own
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardRepositoryAggregateQueries)
{
    Dash dash("t14_test17.db");
    dash.addResident("Ana");
    dash.addResident("Ben", false);
    const int resident = dash.addResident("Carla");
    dash.addRequest(resident, "2026-11-01", "Pending");
    dash.addRequest(resident, "2026-11-02", "Completed");
    dash.addRequest(resident, "2026-11-03", "Completed");

    CHECK(dash.residents->countAll() == 3);
    CHECK(dash.residents->countByStatus("Active") == 2);
    CHECK(dash.residents->countByStatus("Inactive") == 1);
    CHECK(dash.residents->countByStatus("Unknown") == 0);

    CHECK(dash.requests->countAll() == 3);
    CHECK(dash.requests->countByStatus("Pending") == 1);
    CHECK(dash.requests->countByStatus("Completed") == 2);
    CHECK(dash.requests->countByStatus("In Progress") == 0);

    CHECK(dash.requests->findRecent(2).size() == 2);
    CHECK(dash.requests->findRecent(10).size() == 3);
    CHECK(dash.requests->findRecent(0).empty());
    // Same ordering as the T11 list
    CHECK(idsOf(dash.requests->findRecent(3)) == idsOf(dash.requests->findAll()));
}

// -----------------------------------------------------------------------------
// Extra: Stored text is escaped on the dashboard
// -----------------------------------------------------------------------------
DROGON_TEST(DashboardEscapesStoredText)
{
    Dash dash("t14_test18.db");
    const int resident = dash.addResident("Ana");
    dash.addRequest(resident, "2026-11-04", "Pending", "<script>alert(1)</script>");

    const std::string body = bodyOf(getDashboard(dash));
    CHECK(!contains(body, "<script>alert(1)</script>"));
    CHECK(contains(body, "&lt;script&gt;alert(1)&lt;/script&gt;"));
}

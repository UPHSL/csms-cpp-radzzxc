#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "controllers/ResidentController.h"
#include "repositories/ResidentRepository.h"
#include "models/Resident.h"

// Helper cleaner to ensure each test runs against an isolated, clean temporary SQLite database
struct InterfaceTestDbCleaner
{
    std::filesystem::path path;

    explicit InterfaceTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~InterfaceTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

// Dispatches an asynchronous controller action synchronously via std::promise/future
static drogon::HttpResponsePtr dispatchSync(
    std::function<void(std::function<void(const drogon::HttpResponsePtr&)>&&)> action
)
{
    std::promise<drogon::HttpResponsePtr> prom;
    auto fut = prom.get_future();
    action([&prom](const drogon::HttpResponsePtr& resp) {
        prom.set_value(resp);
    });
    return fut.get();
}

// -----------------------------------------------------------------------------
// Test 1: Resident Management Page Loads Successfully
// -----------------------------------------------------------------------------
DROGON_TEST(ResidentManagementPageLoadsSuccessfully)
{
    InterfaceTestDbCleaner cleaner("t12_test1.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());
    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.index(req, std::move(cb));
    });

    CHECK(resp != nullptr);
    CHECK(resp->getStatusCode() == drogon::k200OK);
    CHECK(resp->getContentType() == drogon::CT_TEXT_HTML);

    std::string body(resp->getBody());
    CHECK(body.find("Resident Management Registry") != std::string::npos);
    CHECK(body.find("Register New Resident") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 2: Resident List Displays Persisted Residents
// -----------------------------------------------------------------------------
DROGON_TEST(ResidentListDisplaysPersistedResidents)
{
    InterfaceTestDbCleaner cleaner("t12_test2.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    repo->save(csms::Resident("Maria", "Santos", "456 Luna St", "09187654321", "maria@example.com"));

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.index(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("Juan Dela Cruz") != std::string::npos);
    CHECK(body.find("123 Rizal St") != std::string::npos);
    CHECK(body.find("09171234567") != std::string::npos);
    CHECK(body.find("juan@example.com") != std::string::npos);

    CHECK(body.find("Maria Santos") != std::string::npos);
    CHECK(body.find("456 Luna St") != std::string::npos);
    CHECK(body.find("09187654321") != std::string::npos);
    CHECK(body.find("maria@example.com") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 3: Resident Search Uses Existing Search Behavior
// -----------------------------------------------------------------------------
DROGON_TEST(ResidentSearchUsesExistingSearchBehavior)
{
    InterfaceTestDbCleaner cleaner("t12_test3.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    repo->save(csms::Resident("Maria", "Santos", "456 Luna St", "09187654321", "maria@example.com"));

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents");
    req->setMethod(drogon::Get);
    req->setParameter("search", "san");

    auto resp = dispatchSync([&](auto&& cb) {
        controller.index(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("Maria Santos") != std::string::npos);
    CHECK(body.find("Juan Dela Cruz") == std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 4: Blank Search Displays All Residents
// -----------------------------------------------------------------------------
DROGON_TEST(BlankSearchDisplaysAllResidents)
{
    InterfaceTestDbCleaner cleaner("t12_test4.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    repo->save(csms::Resident("Maria", "Santos", "456 Luna St", "09187654321", "maria@example.com"));

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents");
    req->setMethod(drogon::Get);
    req->setParameter("search", "   ");

    auto resp = dispatchSync([&](auto&& cb) {
        controller.index(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("Juan Dela Cruz") != std::string::npos);
    CHECK(body.find("Maria Santos") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 5: Registration Form Is Accessible
// -----------------------------------------------------------------------------
DROGON_TEST(RegistrationFormIsAccessible)
{
    InterfaceTestDbCleaner cleaner("t12_test5.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());
    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/register");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.showRegisterForm(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("Register New Resident") != std::string::npos);
    CHECK(body.find("name=\"firstName\"") != std::string::npos);
    CHECK(body.find("name=\"lastName\"") != std::string::npos);
    CHECK(body.find("name=\"address\"") != std::string::npos);
    CHECK(body.find("name=\"contactNumber\"") != std::string::npos);
    CHECK(body.find("name=\"email\"") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 6: Valid Registration Persists Resident
// -----------------------------------------------------------------------------
DROGON_TEST(ValidRegistrationPersistsResident)
{
    InterfaceTestDbCleaner cleaner("t12_test6.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());
    csms::ResidentController controller(repo);

    auto postReq = drogon::HttpRequest::newHttpRequest();
    postReq->setPath("/residents/register");
    postReq->setMethod(drogon::Post);
    postReq->setParameter("firstName", "Clara");
    postReq->setParameter("lastName", "Reyes");
    postReq->setParameter("address", "101 Katipunan Ave");
    postReq->setParameter("contactNumber", "09171234567");
    postReq->setParameter("email", "clara.reyes@example.com");

    auto postResp = dispatchSync([&](auto&& cb) {
        controller.registerResident(postReq, std::move(cb));
    });

    // PRG pattern: Redirects with HTTP 303 See Other
    CHECK(postResp->getStatusCode() == drogon::k303SeeOther);
    CHECK(postResp->getHeader("Location") == "/residents?msg=registered");

    // Verify record in persistence
    auto all = repo->findAll();
    CHECK(all.size() == 1);
    CHECK(all[0].getFirstName() == "Clara");
    CHECK(all[0].getLastName() == "Reyes");
    CHECK(all[0].getId().has_value());
    CHECK(all[0].getStatus() == "Active");

    // Verify subsequent presentation in directory
    auto getReq = drogon::HttpRequest::newHttpRequest();
    getReq->setPath("/residents");
    getReq->setMethod(drogon::Get);

    auto getResp = dispatchSync([&](auto&& cb) {
        controller.index(getReq, std::move(cb));
    });

    std::string body(getResp->getBody());
    CHECK(body.find("Clara Reyes") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 7: Invalid Registration Shows Validation Feedback and Does Not Persist
// -----------------------------------------------------------------------------
DROGON_TEST(InvalidRegistrationShowsValidationFeedbackAndDoesNotPersist)
{
    InterfaceTestDbCleaner cleaner("t12_test7.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());
    csms::ResidentController controller(repo);

    auto postReq = drogon::HttpRequest::newHttpRequest();
    postReq->setPath("/residents/register");
    postReq->setMethod(drogon::Post);
    postReq->setParameter("firstName", "Clara");
    postReq->setParameter("lastName", "Reyes");
    postReq->setParameter("address", "101 Katipunan Ave");
    postReq->setParameter("contactNumber", "invalid_phone"); // Invalid
    postReq->setParameter("email", "clara.reyes@example.com");

    auto postResp = dispatchSync([&](auto&& cb) {
        controller.registerResident(postReq, std::move(cb));
    });

    CHECK(postResp->getStatusCode() == drogon::k200OK);
    std::string body(postResp->getBody());

    CHECK(body.find("There is a problem with the submitted form") != std::string::npos);
    CHECK(body.find("contactNumber") != std::string::npos);

    // Verify non-sensitive inputs were preserved in form
    CHECK(body.find("value=\"Clara\"") != std::string::npos);
    CHECK(body.find("value=\"Reyes\"") != std::string::npos);

    // Verify no record persisted
    auto all = repo->findAll();
    CHECK(all.empty());
}

// -----------------------------------------------------------------------------
// Test 8: Edit Form Displays Existing Resident Information
// -----------------------------------------------------------------------------
DROGON_TEST(EditFormDisplaysExistingResidentInformation)
{
    InterfaceTestDbCleaner cleaner("t12_test8.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/" + std::to_string(id) + "/edit");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.showEditForm(req, std::move(cb), std::to_string(id));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("value=\"Juan\"") != std::string::npos);
    CHECK(body.find("value=\"Dela Cruz\"") != std::string::npos);
    CHECK(body.find("value=\"123 Rizal St\"") != std::string::npos);
    CHECK(body.find("value=\"09171234567\"") != std::string::npos);
    CHECK(body.find("value=\"juan@example.com\"") != std::string::npos);

    // ID and status must not be editable inputs
    CHECK(body.find("name=\"id\"") == std::string::npos);
    CHECK(body.find("name=\"status\"") == std::string::npos);
    CHECK(body.find("Read-only") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 9: Valid Update Persists Changes
// -----------------------------------------------------------------------------
DROGON_TEST(ValidUpdatePersistsChanges)
{
    InterfaceTestDbCleaner cleaner("t12_test9.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/" + std::to_string(id) + "/edit");
    req->setMethod(drogon::Post);
    req->setParameter("firstName", "Juanito");
    req->setParameter("lastName", "Dela Cruz");
    req->setParameter("address", "999 Magsaysay Ave");
    req->setParameter("contactNumber", "09179998888");
    req->setParameter("email", "juanito@example.com");

    auto resp = dispatchSync([&](auto&& cb) {
        controller.updateResident(req, std::move(cb), std::to_string(id));
    });

    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(resp->getHeader("Location") == "/residents?msg=updated");

    auto updatedOpt = repo->findById(id);
    CHECK(updatedOpt.has_value());
    CHECK(updatedOpt->getFirstName() == "Juanito");
    CHECK(updatedOpt->getAddress() == "999 Magsaysay Ave");
    CHECK(updatedOpt->getContactNumber() == "09179998888");
    CHECK(updatedOpt->getEmail() == "juanito@example.com");

    // ID and Status preserved
    CHECK(updatedOpt->getId().value() == id);
    CHECK(updatedOpt->getStatus() == "Active");
}

// -----------------------------------------------------------------------------
// Test 10: Invalid Update Does Not Modify Resident
// -----------------------------------------------------------------------------
DROGON_TEST(InvalidUpdateDoesNotModifyResident)
{
    InterfaceTestDbCleaner cleaner("t12_test10.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/" + std::to_string(id) + "/edit");
    req->setMethod(drogon::Post);
    req->setParameter("firstName", "Juan");
    req->setParameter("lastName", "Dela Cruz");
    req->setParameter("address", "123 Rizal St");
    req->setParameter("contactNumber", "invalid_number"); // Invalid
    req->setParameter("email", "juan@example.com");

    auto resp = dispatchSync([&](auto&& cb) {
        controller.updateResident(req, std::move(cb), std::to_string(id));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());
    CHECK(body.find("There is a problem with the submitted form") != std::string::npos);

    // Verify persistence unchanged
    auto unchangedOpt = repo->findById(id);
    CHECK(unchangedOpt.has_value());
    CHECK(unchangedOpt->getContactNumber() == "09171234567");
}

// -----------------------------------------------------------------------------
// Test 11: Missing Resident Edit Is Handled Safely
// -----------------------------------------------------------------------------
DROGON_TEST(MissingResidentEditIsHandledSafely)
{
    InterfaceTestDbCleaner cleaner("t12_test11.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());
    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/99999/edit");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.showEditForm(req, std::move(cb), "99999");
    });

    CHECK(resp->getStatusCode() == drogon::k404NotFound);
    std::string body(resp->getBody());
    CHECK(body.find("404 - Resident Record Not Found") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 12: Active Resident Can Be Deactivated Through the Interface
// -----------------------------------------------------------------------------
DROGON_TEST(ActiveResidentCanBeDeactivatedThroughInterface)
{
    InterfaceTestDbCleaner cleaner("t12_test12.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents/" + std::to_string(id) + "/deactivate");
    req->setMethod(drogon::Post);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.deactivateResident(req, std::move(cb), std::to_string(id));
    });

    CHECK(resp->getStatusCode() == drogon::k303SeeOther);
    CHECK(resp->getHeader("Location") == "/residents?msg=deactivated");

    // Verify soft deactivation
    auto deactivatedOpt = repo->findById(id);
    CHECK(deactivatedOpt.has_value());
    CHECK(deactivatedOpt->getStatus() == "Inactive");
    CHECK(deactivatedOpt->getFirstName() == "Juan");
}

// -----------------------------------------------------------------------------
// Test 13: Inactive Resident Remains Visible
// -----------------------------------------------------------------------------
DROGON_TEST(InactiveResidentRemainsVisible)
{
    InterfaceTestDbCleaner cleaner("t12_test13.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();
    repo->deactivateById(id);

    csms::ResidentController controller(repo);

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/residents");
    req->setMethod(drogon::Get);

    auto resp = dispatchSync([&](auto&& cb) {
        controller.index(req, std::move(cb));
    });

    CHECK(resp->getStatusCode() == drogon::k200OK);
    std::string body(resp->getBody());

    CHECK(body.find("Juan Dela Cruz") != std::string::npos);
    CHECK(body.find("Inactive") != std::string::npos);
    CHECK(body.find("Deactivated") != std::string::npos);
}

// -----------------------------------------------------------------------------
// Test 14: GET Requests Do Not Deactivate Residents
// -----------------------------------------------------------------------------
DROGON_TEST(GetRequestsDoNotDeactivateResidents)
{
    InterfaceTestDbCleaner cleaner("t12_test14.db");
    auto repo = std::make_shared<csms::ResidentRepository>(cleaner.str());

    auto saved = repo->save(csms::Resident("Juan", "Dela Cruz", "123 Rizal St", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    csms::ResidentController controller(repo);

    // 1. GET /residents
    auto listReq = drogon::HttpRequest::newHttpRequest();
    listReq->setPath("/residents");
    listReq->setMethod(drogon::Get);
    dispatchSync([&](auto&& cb) {
        controller.index(listReq, std::move(cb));
    });

    // 2. GET /residents?search=Juan
    auto searchReq = drogon::HttpRequest::newHttpRequest();
    searchReq->setPath("/residents");
    searchReq->setMethod(drogon::Get);
    searchReq->setParameter("search", "Juan");
    dispatchSync([&](auto&& cb) {
        controller.index(searchReq, std::move(cb));
    });

    // 3. GET /residents/register
    auto registerReq = drogon::HttpRequest::newHttpRequest();
    registerReq->setPath("/residents/register");
    registerReq->setMethod(drogon::Get);
    dispatchSync([&](auto&& cb) {
        controller.showRegisterForm(registerReq, std::move(cb));
    });

    // 4. GET /residents/{id}/edit
    auto editReq = drogon::HttpRequest::newHttpRequest();
    editReq->setPath("/residents/" + std::to_string(id) + "/edit");
    editReq->setMethod(drogon::Get);
    dispatchSync([&](auto&& cb) {
        controller.showEditForm(editReq, std::move(cb), std::to_string(id));
    });

    // Verify status remains Active
    auto checkOpt = repo->findById(id);
    CHECK(checkOpt.has_value());
    CHECK(checkOpt->getStatus() == "Active");
}
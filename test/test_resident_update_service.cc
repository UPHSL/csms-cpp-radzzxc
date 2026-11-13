#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "models/ResidentValidator.h"
#include "repositories/ResidentRepository.h"
#include "services/ResidentUpdateService.h"
#include "services/ResidentQueryService.h"
#include <filesystem>
#include <string>
#include <optional>
#include <algorithm>
#include <sqlite3.h>

// Helper to construct and cleanup an isolated temporary SQLite database in the OS temp directory
struct UpdateTestDbCleaner
{
    std::filesystem::path path;

    explicit UpdateTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~UpdateTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

// Helper: Factory function returning an initial valid Resident
static csms::Resident makeInitialResident()
{
    return csms::Resident(
        "Juan",
        "Cruz",
        "123 Rizal Street",
        "09171234567",
        "juan.cruz@example.com"
    );
}

// Test 1: Valid Resident Update Succeeds
// Verifies that updating a resident with valid information returns a successful result.
DROGON_TEST(ValidResidentUpdateSucceeds)
{
    UpdateTestDbCleaner cleaner("test_update_valid.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    csms::ResidentUpdateResult result = service.updateResident(
        id,
        "Juan Miguel",
        "Dela Cruz",
        "456 Bonifacio Avenue",
        "09181234567",
        "juan.miguel@example.com"
    );

    CHECK(result.success == true);
    CHECK(result.notFound == false);
    CHECK(result.resident.has_value());
    CHECK(result.errors.empty());
}

// Test 2: Resident ID Is Preserved
// Verifies that the updated resident retains its original primary key identity.
DROGON_TEST(ResidentIdIsPreserved)
{
    UpdateTestDbCleaner cleaner("test_update_preserve_id.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int originalId = saved.getId().value();

    csms::ResidentUpdateResult result = service.updateResident(
        originalId,
        "Juan Miguel",
        "Dela Cruz",
        "456 Bonifacio Avenue",
        "09181234567",
        "juan.miguel@example.com"
    );

    CHECK(result.success == true);
    CHECK(result.resident.has_value());
    CHECK(result.resident->getId().has_value() == true);
    CHECK(result.resident->getId().value() == originalId);
}

// Test 3: Permitted Resident Information Is Persisted
// Verifies that all 5 editable fields are updated in the SQLite database.
DROGON_TEST(PermittedResidentInformationIsPersisted)
{
    UpdateTestDbCleaner cleaner("test_update_persisted.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    service.updateResident(
        id,
        "Pedro",
        "Santos",
        "789 Magsaysay Blvd",
        "09191234567",
        "pedro.santos@example.com"
    );

    std::optional<csms::Resident> stored = repo.findById(id);
    CHECK(stored.has_value() == true);
    CHECK(stored->getFirstName() == "Pedro");
    CHECK(stored->getLastName() == "Santos");
    CHECK(stored->getAddress() == "789 Magsaysay Blvd");
    CHECK(stored->getContactNumber() == "09191234567");
    CHECK(stored->getEmail() == "pedro.santos@example.com");
}

// Test 4: Resident Status Is Preserved
// Verifies that an update operation does not alter Active or Inactive status.
DROGON_TEST(ResidentStatusIsPreservedAcrossUpdate)
{
    UpdateTestDbCleaner cleaner("test_update_status.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    // Case A: Active resident preserves Active
    csms::Resident activeResident = makeInitialResident();
    csms::Resident savedActive = repo.save(activeResident);
    int activeId = savedActive.getId().value();

    service.updateResident(activeId, "Juan", "Updated", "Address", "09171234567", "juan@example.com");
    std::optional<csms::Resident> storedActive = repo.findById(activeId);
    CHECK(storedActive.has_value() == true);
    CHECK(storedActive->getStatus() == "Active");

    // Case B: Inactive resident preserves Inactive
    csms::Resident inactiveResident("Maria", "Clara", "Address", "09181234567", "maria@example.com", "Inactive");
    csms::Resident savedInactive = repo.save(inactiveResident);
    int inactiveId = savedInactive.getId().value();

    service.updateResident(inactiveId, "Maria", "Updated", "Address", "09181234567", "maria@example.com");
    std::optional<csms::Resident> storedInactive = repo.findById(inactiveId);
    CHECK(storedInactive.has_value() == true);
    CHECK(storedInactive->getStatus() == "Inactive");
}

// Test 5: Invalid Update Fails
// Verifies that proposing invalid data (e.g. blank firstName) returns validation failure.
DROGON_TEST(InvalidUpdateFails)
{
    UpdateTestDbCleaner cleaner("test_update_invalid.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    // Blank firstName violates validation rules
    csms::ResidentUpdateResult result = service.updateResident(
        id,
        "",
        "Dela Cruz",
        "456 Bonifacio Avenue",
        "09181234567",
        "juan@example.com"
    );

    CHECK(result.success == false);
    CHECK(result.notFound == false);
    CHECK(result.resident.has_value() == false);
    CHECK(result.errors.empty() == false);

    // Verify "firstName" is identified as an invalid field
    auto it = std::find(result.errors.begin(), result.errors.end(), "firstName");
    CHECK(it != result.errors.end());
}

// Test 6: Invalid Update Does Not Modify Persisted Information
// Verifies that after a failed update, stored data remains strictly unchanged.
DROGON_TEST(InvalidUpdateDoesNotModifyPersistedInformation)
{
    UpdateTestDbCleaner cleaner("test_update_no_partial.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    // Attempt invalid update with invalid contact number
    csms::ResidentUpdateResult result = service.updateResident(
        id,
        "NewName",
        "NewLastName",
        "NewAddress",
        "INVALID_CONTACT",
        "valid@example.com"
    );

    CHECK(result.success == false);

    // Verify database row still contains the original values
    std::optional<csms::Resident> stored = repo.findById(id);
    CHECK(stored.has_value() == true);
    CHECK(stored->getFirstName() == "Juan");
    CHECK(stored->getLastName() == "Cruz");
    CHECK(stored->getContactNumber() == "09171234567");
}

// Test 7: Updating a Nonexistent Resident Is Handled Safely
// Verifies updating an ID that does not exist returns notFound = true without crashing.
DROGON_TEST(UpdatingNonexistentResidentHandledSafely)
{
    UpdateTestDbCleaner cleaner("test_update_not_found.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    int nonExistentId = 999999;
    csms::ResidentUpdateResult result = service.updateResident(
        nonExistentId,
        "Ghost",
        "Resident",
        "Nowhere",
        "09171234567",
        "ghost@example.com"
    );

    CHECK(result.success == false);
    CHECK(result.notFound == true);
    CHECK(result.resident.has_value() == false);
    CHECK(result.errors.empty() == true);
}

// Test 8: Nonexistent Update Does Not Create a Resident
// Verifies that attempting to update an unknown ID does not perform an upsert/insert.
DROGON_TEST(NonexistentUpdateDoesNotCreateResident)
{
    UpdateTestDbCleaner cleaner("test_update_no_upsert.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    // Database is empty
    CHECK(repo.findAll().empty() == true);

    int nonExistentId = 12345;
    service.updateResident(
        nonExistentId,
        "Nobody",
        "Here",
        "None",
        "09171234567",
        "nobody@example.com"
    );

    // Database must still have 0 records
    CHECK(repo.findAll().empty() == true);
    CHECK(repo.findById(nonExistentId).has_value() == false);
}

// Test 9: Updated Resident Is Visible Through T05 Querying
// Verifies that T05 search and listing immediately reflect the updated information.
DROGON_TEST(UpdatedResidentIsVisibleThroughT05Querying)
{
    UpdateTestDbCleaner cleaner("test_update_t05_integration.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService updateService(validator, repo);
    csms::ResidentQueryService queryService(repo);

    csms::Resident saved = repo.save(csms::Resident("Juan", "Cruz", "Address", "09171234567", "juan@example.com"));
    int id = saved.getId().value();

    // Update name to "Miguel Santos"
    updateService.updateResident(id, "Miguel", "Santos", "Address", "09171234567", "miguel@example.com");

    // Search for new name
    std::vector<csms::Resident> foundNew = queryService.searchResidents("Miguel");
    CHECK(foundNew.size() == 1);
    CHECK(foundNew.at(0).getId().value() == id);
    CHECK(foundNew.at(0).getLastName() == "Santos");

    // Old name should no longer match
    std::vector<csms::Resident> foundOld = queryService.searchResidents("Cruz");
    CHECK(foundOld.empty() == true);
}

// Test 10: Updated Information and Contact Number Are Preserved
// Verifies all updated fields, especially contact number leading zero, persist accurately.
DROGON_TEST(UpdatedInformationAndContactNumberArePreserved)
{
    UpdateTestDbCleaner cleaner("test_update_preserve_contact.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentUpdateService service(validator, repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    csms::ResidentUpdateResult result = service.updateResident(
        id,
        "Maria",
        "Dela Rosa",
        "777 Sunset Road",
        "09181234567",
        "maria.delarosa@example.com"
    );

    CHECK(result.success == true);

    std::optional<csms::Resident> stored = repo.findById(id);
    CHECK(stored.has_value() == true);
    CHECK(stored->getId().value() == id);
    CHECK(stored->getFirstName() == "Maria");
    CHECK(stored->getLastName() == "Dela Rosa");
    CHECK(stored->getAddress() == "777 Sunset Road");
    CHECK(stored->getContactNumber() == "09181234567"); // Leading zero preserved
    CHECK(stored->getEmail() == "maria.delarosa@example.com");
    CHECK(stored->getStatus() == "Active");
}

#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "repositories/ResidentRepository.h"
#include "services/ResidentDeactivationService.h"
#include "services/ResidentQueryService.h"
#include <filesystem>
#include <string>
#include <optional>
#include <algorithm>
#include <sqlite3.h>

// Helper to construct and cleanup an isolated temporary SQLite database in the OS temp directory
struct DeactivateTestDbCleaner
{
    std::filesystem::path path;

    explicit DeactivateTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~DeactivateTestDbCleaner()
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

// Test 1: Active Resident Can Be Deactivated
// Verifies that deactivating an Active resident succeeds and returns an Inactive result.
DROGON_TEST(ActiveResidentCanBeDeactivated)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_active.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();
    CHECK(saved.getStatus() == "Active");

    csms::ResidentDeactivationResult result = service.deactivateResident(id);

    CHECK(result.success == true);
    CHECK(result.alreadyInactive == false);
    CHECK(result.notFound == false);
    CHECK(result.resident.has_value() == true);
    CHECK(result.resident->getStatus() == "Inactive");
}

// Test 2: Resident Status Becomes Inactive in Persistence
// Verifies that the status change is written to SQLite and retrievable via repository.
DROGON_TEST(ResidentStatusBecomesInactiveInPersistence)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_persisted.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    service.deactivateResident(id);

    auto persisted = repo.findById(id);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Inactive");
}

// Test 3: Resident ID Is Preserved
// Verifies that deactivation does not generate a new identifier.
DROGON_TEST(ResidentIdPreservedAcrossDeactivation)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_id_preserved.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int idBefore = saved.getId().value();

    csms::ResidentDeactivationResult result = service.deactivateResident(idBefore);

    CHECK(result.success == true);
    CHECK(result.resident.has_value() == true);
    CHECK(result.resident->getId().value() == idBefore);

    auto persisted = repo.findById(idBefore);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getId().value() == idBefore);
}

// Test 4: Resident Information Is Preserved
// Verifies that all personal and contact details remain unchanged across deactivation.
DROGON_TEST(DeactivatedResidentInformationIsPreserved)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_info_preserved.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident original(
        "Maria",
        "Santos",
        "456 Bonifacio Avenue",
        "09187654321",
        "maria.santos@example.com"
    );
    csms::Resident saved = repo.save(original);
    int id = saved.getId().value();

    service.deactivateResident(id);

    auto persisted = repo.findById(id);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getFirstName() == "Maria");
    CHECK(persisted->getLastName() == "Santos");
    CHECK(persisted->getAddress() == "456 Bonifacio Avenue");
    CHECK(persisted->getContactNumber() == "09187654321");
    CHECK(persisted->getEmail() == "maria.santos@example.com");
    CHECK(persisted->getStatus() == "Inactive");
}

// Test 5: Deactivated Resident Remains Persisted and Retrievable
// Verifies that deactivation is a soft operation, not a physical delete.
DROGON_TEST(DeactivatedResidentRemainsPersistedAndRetrievable)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_soft_delete.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    service.deactivateResident(id);

    // Verify record still exists in persistence via findById
    auto retrieved = repo.findById(id);
    CHECK(retrieved.has_value() == true);
    CHECK(retrieved->getId().value() == id);
    CHECK(retrieved->getStatus() == "Inactive");
}

// Test 6: Deactivated Resident Remains Available Through T05
// Verifies that deactivated residents remain included in T05 search and listing.
DROGON_TEST(DeactivatedResidentRemainsAvailableThroughT05)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_t05_integration.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService deactivationService(repo);
    csms::ResidentQueryService queryService(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    deactivationService.deactivateResident(id);

    // 1. Check T05 listing
    std::vector<csms::Resident> all = queryService.listResidents();
    CHECK(all.size() == 1);
    CHECK(all[0].getId().value() == id);
    CHECK(all[0].getStatus() == "Inactive");

    // 2. Check T05 name search
    std::vector<csms::Resident> searchResults = queryService.searchResidents("Cruz");
    CHECK(searchResults.size() == 1);
    CHECK(searchResults[0].getId().value() == id);
    CHECK(searchResults[0].getStatus() == "Inactive");
}

// Test 7: Already-Inactive Resident Is Handled Safely
// Verifies that repeated deactivation is idempotent and reports alreadyInactive safely.
DROGON_TEST(AlreadyInactiveResidentIsHandledSafely)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_idempotent.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident saved = repo.save(makeInitialResident());
    int id = saved.getId().value();

    // First deactivation
    csms::ResidentDeactivationResult firstResult = service.deactivateResident(id);
    CHECK(firstResult.success == true);
    CHECK(firstResult.alreadyInactive == false);
    CHECK(firstResult.notFound == false);

    // Second deactivation (already Inactive)
    csms::ResidentDeactivationResult secondResult = service.deactivateResident(id);
    CHECK(secondResult.success == true);
    CHECK(secondResult.alreadyInactive == true);
    CHECK(secondResult.notFound == false);
    CHECK(secondResult.resident.has_value() == true);
    CHECK(secondResult.resident->getId().value() == id);
    CHECK(secondResult.resident->getStatus() == "Inactive");

    // Verify persisted state remains Inactive and preserved
    auto persisted = repo.findById(id);
    CHECK(persisted.has_value() == true);
    CHECK(persisted->getStatus() == "Inactive");
    CHECK(persisted->getFirstName() == "Juan");
}

// Test 8: Nonexistent Resident Is Handled Safely
// Verifies that attempting to deactivate an unknown ID returns notFound safely without crashing.
DROGON_TEST(NonexistentResidentIsHandledSafely)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_not_found.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    int nonExistentId = 999999;
    csms::ResidentDeactivationResult result = service.deactivateResident(nonExistentId);

    CHECK(result.success == false);
    CHECK(result.alreadyInactive == false);
    CHECK(result.notFound == true);
    CHECK(result.resident.has_value() == false);
}

// Test 9: Nonexistent Deactivation Does Not Create or Delete Records
// Verifies that attempting to deactivate a missing ID does not insert or delete any rows.
DROGON_TEST(NonexistentDeactivationDoesNotCreateOrDeleteRecords)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_no_side_effects.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    // Persist one initial resident
    csms::Resident saved = repo.save(makeInitialResident());
    int originalId = saved.getId().value();

    size_t countBefore = repo.findAll().size();
    CHECK(countBefore == 1);

    int nonExistentId = 888888;
    service.deactivateResident(nonExistentId);

    // Check count and records after
    std::vector<csms::Resident> after = repo.findAll();
    CHECK(after.size() == countBefore);
    CHECK(after[0].getId().value() == originalId);
    CHECK(after[0].getStatus() == "Active");
}

// Test 10: Deactivating One Resident Does Not Affect Another
// Verifies that deactivating a specific resident leaves other residents untouched.
DROGON_TEST(DeactivatingOneResidentDoesNotAffectAnother)
{
    DeactivateTestDbCleaner cleaner("test_deactivate_isolation.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentDeactivationService service(repo);

    csms::Resident res1 = repo.save(csms::Resident("Juan", "Cruz", "123 Street", "09171111111", "juan@example.com"));
    csms::Resident res2 = repo.save(csms::Resident("Maria", "Santos", "456 Avenue", "09182222222", "maria@example.com"));

    int id1 = res1.getId().value();
    int id2 = res2.getId().value();

    // Deactivate only res1
    csms::ResidentDeactivationResult result = service.deactivateResident(id1);
    CHECK(result.success == true);

    auto persisted1 = repo.findById(id1);
    auto persisted2 = repo.findById(id2);

    CHECK(persisted1.has_value() == true);
    CHECK(persisted1->getStatus() == "Inactive");

    // res2 must remain Active and completely unchanged
    CHECK(persisted2.has_value() == true);
    CHECK(persisted2->getStatus() == "Active");
    CHECK(persisted2->getFirstName() == "Maria");
    CHECK(persisted2->getLastName() == "Santos");
    CHECK(persisted2->getAddress() == "456 Avenue");
    CHECK(persisted2->getContactNumber() == "09182222222");
    CHECK(persisted2->getEmail() == "maria@example.com");
}

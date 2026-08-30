#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "repositories/ResidentRepository.h"
#include <filesystem>
#include <string>

// Helper to construct and cleanup an isolated temporary DB path in the OS temp directory
struct TestDbCleaner
{
    std::filesystem::path path;

    explicit TestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~TestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

// Test 1: Persist a Resident
DROGON_TEST(PersistResidentSuccess)
{
    TestDbCleaner cleaner("test_persist_resident.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident resident("Juan", "Dela Cruz", "123 Main St", "09171234567", "juan@example.com", "Active");

    csms::Resident saved = repo.save(resident);
    CHECK(saved.getFirstName() == "Juan");
    CHECK(saved.getLastName() == "Dela Cruz");
}

// Test 2: Resident Receives an Identifier
DROGON_TEST(ResidentReceivesIdentifierAfterSave)
{
    TestDbCleaner cleaner("test_resident_id.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident resident("Maria", "Santos", "456 Elm St", "09181234567", "maria@example.com");

    CHECK(resident.getId().has_value() == false);

    csms::Resident saved = repo.save(resident);
    CHECK(saved.getId().has_value() == true);
    CHECK(saved.getId().value() > 0);
}

// Test 3: Retrieve Resident by Identifier
DROGON_TEST(RetrieveResidentByIdentifier)
{
    TestDbCleaner cleaner("test_retrieve_resident.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident resident("Pedro", "Penduko", "789 Pine St", "09191234567", "pedro@example.com", "Active");

    csms::Resident saved = repo.save(resident);
    int assignedId = saved.getId().value();

    auto foundOpt = repo.findById(assignedId);
    CHECK(foundOpt.has_value() == true);
    CHECK(foundOpt->getId().value() == assignedId);
    CHECK(foundOpt->getFirstName() == "Pedro");
}

// Test 4: Resident Information Is Preserved
DROGON_TEST(ResidentInformationIsPreserved)
{
    TestDbCleaner cleaner("test_info_preserved.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident resident("Ana", "Reyes", "321 Oak Ave", "09170001234", "ana.reyes@example.com", "Active");

    csms::Resident saved = repo.save(resident);
    auto foundOpt = repo.findById(saved.getId().value());

    CHECK(foundOpt.has_value() == true);
    CHECK(foundOpt->getFirstName() == "Ana");
    CHECK(foundOpt->getLastName() == "Reyes");
    CHECK(foundOpt->getAddress() == "321 Oak Ave");
    CHECK(foundOpt->getContactNumber() == "09170001234"); // Leading zero preserved
    CHECK(foundOpt->getEmail() == "ana.reyes@example.com");
    CHECK(foundOpt->getStatus() == "Active");
}

// Test 5: Active Status Is Preserved
DROGON_TEST(ActiveStatusIsPreserved)
{
    TestDbCleaner cleaner("test_active_status.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident resident("Carlos", "Garcia", "654 Birch Rd", "09171112233", "carlos@example.com");

    csms::Resident saved = repo.save(resident);
    auto foundOpt = repo.findById(saved.getId().value());

    CHECK(foundOpt.has_value() == true);
    CHECK(foundOpt->getStatus() == "Active");
}

// Test 6: Missing Resident Is Handled
DROGON_TEST(MissingResidentReturnsNullopt)
{
    TestDbCleaner cleaner("test_missing_resident.db");

    csms::ResidentRepository repo(cleaner.str());
    auto foundOpt = repo.findById(999999);

    CHECK(foundOpt.has_value() == false);
}

// Test 7: Persistence Is Not Limited to One Repository Object
DROGON_TEST(PersistenceAcrossDistinctRepositoryInstances)
{
    TestDbCleaner cleaner("test_cross_instance.db");

    int savedId = 0;
    {
        csms::ResidentRepository repoA(cleaner.str());
        csms::Resident resident("Elena", "Torres", "987 Cedar Blvd", "09175556677", "elena@example.com", "Active");
        csms::Resident saved = repoA.save(resident);
        savedId = saved.getId().value();
    } // repoA is destroyed and its SQLite connection closed here

    {
        csms::ResidentRepository repoB(cleaner.str());
        auto foundOpt = repoB.findById(savedId);

        CHECK(foundOpt.has_value() == true);
        CHECK(foundOpt->getId().value() == savedId);
        CHECK(foundOpt->getFirstName() == "Elena");
        CHECK(foundOpt->getLastName() == "Torres");
    }
}

// Test 8 (Student-Designed Test): Multiple Residents Persist With Distinct Auto-Increment IDs
DROGON_TEST(MultipleResidentsPersistWithDistinctAutoIncrementIds)
{
    TestDbCleaner cleaner("test_multiple_residents.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::Resident res1("ResidentOne", "LastNameA", "Address 1", "09171111111", "res1@example.com", "Active");
    csms::Resident res2("ResidentTwo", "LastNameB", "Address 2", "09172222222", "res2@example.com", "Inactive");

    csms::Resident saved1 = repo.save(res1);
    csms::Resident saved2 = repo.save(res2);

    CHECK(saved1.getId().has_value() == true);
    CHECK(saved2.getId().has_value() == true);
    CHECK(saved1.getId().value() != saved2.getId().value());

    auto found1 = repo.findById(saved1.getId().value());
    auto found2 = repo.findById(saved2.getId().value());

    CHECK(found1.has_value() == true);
    CHECK(found2.has_value() == true);
    CHECK(found1->getFirstName() == "ResidentOne");
    CHECK(found1->getStatus() == "Active");
    CHECK(found2->getFirstName() == "ResidentTwo");
    CHECK(found2->getStatus() == "Inactive");
}

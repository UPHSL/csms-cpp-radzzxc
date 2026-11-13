#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "models/ResidentValidator.h"
#include "repositories/ResidentRepository.h"
#include "services/ResidentRegistrationService.h"
#include <filesystem>
#include <string>
#include <algorithm>
#include <sqlite3.h>

// Helper to construct and cleanup an isolated temporary SQLite database in the OS temp directory
struct RegistrationTestDbCleaner
{
    std::filesystem::path path;

    explicit RegistrationTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~RegistrationTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

// Helper: Count rows directly in SQLite table to verify database persistence state
static int countResidentsInDb(const std::string& dbPath)
{
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        if (db) sqlite3_close(db);
        return -1;
    }

    const char* sql = "SELECT COUNT(*) FROM residents;";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        return -1;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

// Helper: Factory function returning a valid Resident instance for registration tests
static csms::Resident makeValidResident()
{
    return csms::Resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
}

// Test 1: Register a Valid Resident
// Verifies that a valid resident passes validation and persists successfully.
DROGON_TEST(RegisterValidResidentSuccess)
{
    RegistrationTestDbCleaner cleaner("test_reg_valid.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident resident = makeValidResident();
    csms::ResidentRegistrationResult result = service.registerResident(resident);

    CHECK(result.success == true);
    CHECK(result.resident.has_value());
    CHECK(result.errors.empty());
}

// Test 2: Registered Resident Receives an Identifier
// Verifies that the resident starts with no ID and receives an auto-generated ID from SQLite.
DROGON_TEST(RegisteredResidentReceivesIdentifier)
{
    RegistrationTestDbCleaner cleaner("test_reg_id.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident resident = makeValidResident();
    // Before registration: no ID assigned yet
    CHECK(resident.getId().has_value() == false);

    csms::ResidentRegistrationResult result = service.registerResident(resident);

    // After registration: generated database identifier is present
    CHECK(result.success == true);
    CHECK(result.resident.has_value());
    CHECK(result.resident->getId().has_value() == true);
    CHECK(result.resident->getId().value() > 0);
}

// Test 3: Registered Resident Is Actually Persisted
// Verifies that the resident can be retrieved directly from the repository using the generated ID.
DROGON_TEST(RegisteredResidentIsPersisted)
{
    RegistrationTestDbCleaner cleaner("test_reg_persisted.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident resident = makeValidResident();
    csms::ResidentRegistrationResult result = service.registerResident(resident);

    CHECK(result.success == true);
    CHECK(result.resident.has_value());
    CHECK(result.resident->getId().has_value() == true);

    int id = result.resident->getId().value();
    std::optional<csms::Resident> stored = repo.findById(id);

    CHECK(stored.has_value() == true);
}

// Test 4: Registered Resident Information Is Preserved
// Verifies that all field values (including the leading zero in contactNumber) remain intact.
DROGON_TEST(RegisteredResidentInformationIsPreserved)
{
    RegistrationTestDbCleaner cleaner("test_reg_preserve_info.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident resident = makeValidResident();
    csms::ResidentRegistrationResult result = service.registerResident(resident);

    CHECK(result.success == true);
    int id = result.resident->getId().value();

    std::optional<csms::Resident> stored = repo.findById(id);
    CHECK(stored.has_value() == true);
    CHECK(stored->getFirstName() == "Juan");
    CHECK(stored->getLastName() == "Dela Cruz");
    CHECK(stored->getAddress() == "Barangay Santo Tomas");
    CHECK(stored->getContactNumber() == "09171234567");
    CHECK(stored->getEmail() == "juan@example.com");
    CHECK(stored->getStatus() == "Active");
}

// Test 5: Default Active Status Is Preserved
// Verifies that when a resident is created with default status, it remains "Active" after registration.
DROGON_TEST(RegisteredResidentPreservesDefaultActiveStatus)
{
    RegistrationTestDbCleaner cleaner("test_reg_default_status.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident resident = makeValidResident();
    CHECK(resident.getStatus() == "Active");

    csms::ResidentRegistrationResult result = service.registerResident(resident);

    CHECK(result.success == true);
    CHECK(result.resident.has_value());
    CHECK(result.resident->getStatus() == "Active");
}

// Test 6: Invalid Resident Registration Fails
// Verifies that registering an invalid resident halts and returns failure.
DROGON_TEST(InvalidResidentRegistrationFails)
{
    RegistrationTestDbCleaner cleaner("test_reg_invalid_fail.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    // Missing first name violates validation rules
    csms::Resident invalidResident(
        "",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );

    csms::ResidentRegistrationResult result = service.registerResident(invalidResident);

    CHECK(result.success == false);
    CHECK(result.resident.has_value() == false);
    CHECK(result.errors.empty() == false);
}

// Test 7: Invalid Resident Is NOT Persisted
// Verifies via direct SQLite COUNT(*) inspection that no record is written when validation fails.
DROGON_TEST(InvalidResidentIsNotPersisted)
{
    RegistrationTestDbCleaner cleaner("test_reg_invalid_no_persist.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    // Check table count before registration attempt
    int countBefore = countResidentsInDb(cleaner.str());

    csms::Resident invalidResident(
        "",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );

    csms::ResidentRegistrationResult result = service.registerResident(invalidResident);

    // Check table count after registration attempt
    int countAfter = countResidentsInDb(cleaner.str());

    CHECK(result.success == false);
    // Table count must remain strictly unchanged
    CHECK(countAfter == countBefore);
}

// Test 8: Validation Failure Can Be Identified
// Verifies that the returned errors vector contains the specific failing field name.
DROGON_TEST(RegistrationReturnsValidationErrors)
{
    RegistrationTestDbCleaner cleaner("test_reg_errors.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentValidator validator;
    csms::ResidentRegistrationService service(validator, repo);

    csms::Resident invalidResident(
        "",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );

    csms::ResidentRegistrationResult result = service.registerResident(invalidResident);

    CHECK(result.success == false);

    // Verify that "firstName" is specifically reported among the errors
    auto it = std::find(result.errors.begin(), result.errors.end(), "firstName");
    CHECK(it != result.errors.end());
}

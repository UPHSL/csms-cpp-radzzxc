#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "repositories/ResidentRepository.h"
#include "services/ResidentQueryService.h"
#include <filesystem>
#include <string>
#include <vector>

// Helper to construct and cleanup an isolated temporary SQLite database in the OS temp directory
struct QueryTestDbCleaner
{
    std::filesystem::path path;

    explicit QueryTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~QueryTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

// Helper: Factory function returning an Active Resident
static csms::Resident makeResidentForQuery(
    const std::string& firstName,
    const std::string& lastName,
    const std::string& contactNumber,
    const std::string& email
)
{
    return csms::Resident(
        firstName,
        lastName,
        "Barangay Santo Tomas",
        contactNumber,
        email
    );
}

// Helper: Factory function returning an Inactive Resident
static csms::Resident makeInactiveResidentForQuery(
    const std::string& firstName,
    const std::string& lastName,
    const std::string& contactNumber,
    const std::string& email
)
{
    csms::Resident resident = makeResidentForQuery(
        firstName,
        lastName,
        contactNumber,
        email
    );
    resident.setStatus("Inactive");
    return resident;
}

// Test 1: List All Persisted Residents
// Verifies that listResidents() returns all records currently in SQLite.
DROGON_TEST(ListAllPersistedResidents)
{
    QueryTestDbCleaner cleaner("test_list_all.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Cruz", "09171234561", "juan@example.com"));
    repo.save(makeResidentForQuery("Maria", "Santos", "09171234562", "maria@example.com"));
    repo.save(makeResidentForQuery("Ana", "Reyes", "09171234563", "ana@example.com"));

    std::vector<csms::Resident> residents = service.listResidents();
    CHECK(residents.size() == 3);
}

// Test 2: Empty Resident Listing
// Verifies that querying an empty database safely returns an empty vector without throwing.
DROGON_TEST(EmptyResidentListing)
{
    QueryTestDbCleaner cleaner("test_list_empty.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    std::vector<csms::Resident> residents = service.listResidents();
    CHECK(residents.empty() == true);
}

// Test 3: Resident Listing Uses Required Ordering
// Verifies deterministic ordering: lastName ASC, then firstName ASC, then id ASC (tie-breaker).
DROGON_TEST(ResidentListingUsesRequiredOrdering)
{
    QueryTestDbCleaner cleaner("test_list_ordering.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    // Insert out of required order
    repo.save(makeResidentForQuery("Ana", "Santos", "09171234561", "ana.santos@example.com"));
    repo.save(makeResidentForQuery("Pedro", "Cruz", "09171234562", "pedro.cruz@example.com"));
    repo.save(makeResidentForQuery("Maria", "Andres", "09171234563", "maria.andres@example.com"));
    csms::Resident firstJuan = repo.save(makeResidentForQuery("Juan", "Cruz", "09171234564", "juan.one@example.com"));
    csms::Resident secondJuan = repo.save(makeResidentForQuery("Juan", "Cruz", "09171234565", "juan.two@example.com"));

    std::vector<csms::Resident> residents = service.listResidents();
    CHECK(residents.size() == 5);

    // Expected order:
    // 0: Andres, Maria
    // 1: Cruz, Juan (firstJuan id)
    // 2: Cruz, Juan (secondJuan id)
    // 3: Cruz, Pedro
    // 4: Santos, Ana
    CHECK(residents.at(0).getLastName() == "Andres");
    CHECK(residents.at(0).getFirstName() == "Maria");

    CHECK(residents.at(1).getLastName() == "Cruz");
    CHECK(residents.at(1).getFirstName() == "Juan");
    CHECK(residents.at(1).getId() == firstJuan.getId());

    CHECK(residents.at(2).getLastName() == "Cruz");
    CHECK(residents.at(2).getFirstName() == "Juan");
    CHECK(residents.at(2).getId() == secondJuan.getId());

    CHECK(residents.at(3).getLastName() == "Cruz");
    CHECK(residents.at(3).getFirstName() == "Pedro");

    CHECK(residents.at(4).getLastName() == "Santos");
    CHECK(residents.at(4).getFirstName() == "Ana");
}

// Test 4: Partial First Name Search Is Case-Insensitive
// Verifies search handles partial letters, mixed casing, and outer whitespace on firstName.
DROGON_TEST(PartialFirstNameSearchIsCaseInsensitive)
{
    QueryTestDbCleaner cleaner("test_search_first_name.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Dela Cruz", "09171234561", "juan@example.com"));
    repo.save(makeResidentForQuery("Maria", "Santos", "09171234562", "maria@example.com"));

    std::vector<csms::Resident> results = service.searchResidents("   jUa   ");
    CHECK(results.size() == 1);
    CHECK(results.at(0).getFirstName() == "Juan");
}

// Test 5: Partial Last Name Search Is Case-Insensitive
// Verifies search handles partial letters and mixed casing on lastName.
DROGON_TEST(PartialLastNameSearchIsCaseInsensitive)
{
    QueryTestDbCleaner cleaner("test_search_last_name.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Dela Cruz", "09171234561", "juan@example.com"));
    repo.save(makeResidentForQuery("Maria", "Santos", "09171234562", "maria@example.com"));

    std::vector<csms::Resident> results = service.searchResidents("cRuZ");
    CHECK(results.size() == 1);
    CHECK(results.at(0).getLastName() == "Dela Cruz");
}

// Test 6: Blank Search Returns All Residents
// Verifies that a whitespace-only search string returns the exact same list as listResidents().
DROGON_TEST(BlankSearchReturnsAllResidents)
{
    QueryTestDbCleaner cleaner("test_search_blank.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Cruz", "09171234561", "juan@example.com"));
    repo.save(makeResidentForQuery("Maria", "Santos", "09171234562", "maria@example.com"));

    std::vector<csms::Resident> listed = service.listResidents();
    std::vector<csms::Resident> searched = service.searchResidents("      ");

    CHECK(listed.size() == searched.size());
    for (std::size_t i = 0; i < listed.size(); ++i)
    {
        CHECK(listed.at(i).getId() == searched.at(i).getId());
    }
}

// Test 7: Search With No Match Returns Empty Collection
// Verifies searching for a nonexistent term returns an empty vector without error.
DROGON_TEST(SearchWithNoMatchReturnsEmptyCollection)
{
    QueryTestDbCleaner cleaner("test_search_no_match.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Cruz", "09171234561", "juan@example.com"));

    std::vector<csms::Resident> results = service.searchResidents("ZzzUnknownResident");
    CHECK(results.empty() == true);
}

// Test 8: Search Results Preserve Resident Information
// Verifies that returned records retain all field values, especially contactNumber leading zero.
DROGON_TEST(SearchResultsPreserveResidentInformation)
{
    QueryTestDbCleaner cleaner("test_search_preserve_info.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    csms::Resident saved = repo.save(
        makeResidentForQuery("Juan", "Dela Cruz", "09171234567", "juan@example.com")
    );

    std::vector<csms::Resident> results = service.searchResidents("Juan");
    CHECK(results.size() == 1);

    const csms::Resident& resident = results.at(0);
    CHECK(resident.getId() == saved.getId());
    CHECK(resident.getFirstName() == "Juan");
    CHECK(resident.getLastName() == "Dela Cruz");
    CHECK(resident.getAddress() == "Barangay Santo Tomas");
    CHECK(resident.getContactNumber() == "09171234567");
    CHECK(resident.getEmail() == "juan@example.com");
    CHECK(resident.getStatus() == "Active");
}

// Test 9: Active and Inactive Residents Are Included
// Verifies that T05 does not apply any status filtering; both Active and Inactive records appear.
DROGON_TEST(ListIncludesActiveAndInactiveResidents)
{
    QueryTestDbCleaner cleaner("test_list_active_inactive.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    repo.save(makeResidentForQuery("Juan", "Cruz", "09171234561", "juan@example.com"));
    repo.save(makeInactiveResidentForQuery("Maria", "Santos", "09171234562", "maria@example.com"));

    std::vector<csms::Resident> residents = service.listResidents();
    CHECK(residents.size() == 2);

    bool hasActive = false;
    bool hasInactive = false;
    for (const auto& r : residents)
    {
        if (r.getStatus() == "Active") hasActive = true;
        if (r.getStatus() == "Inactive") hasInactive = true;
    }

    CHECK(hasActive == true);
    CHECK(hasInactive == true);
}

// Test 10: Matching Resident Is Not Duplicated
// Verifies that when a search term matches both firstName and lastName of the same row,
// it is returned only once.
DROGON_TEST(MatchingResidentIsNotDuplicated)
{
    QueryTestDbCleaner cleaner("test_search_no_duplicates.db");

    csms::ResidentRepository repo(cleaner.str());
    csms::ResidentQueryService service(repo);

    // "Ana" and "Anaya" both contain "ana"
    csms::Resident saved = repo.save(
        makeResidentForQuery("Ana", "Anaya", "09171234561", "ana@example.com")
    );

    std::vector<csms::Resident> results = service.searchResidents("ana");
    CHECK(results.size() == 1);
    CHECK(results.at(0).getId() == saved.getId());
}

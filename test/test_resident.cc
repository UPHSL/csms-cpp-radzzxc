#include <drogon/drogon_test.h>
#include "models/Resident.h"

// Test 1: Resident Can Be Created With Required Information
DROGON_TEST(ResidentCreationTest)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com",
        "Active",
        1
    );

    CHECK(resident.getId().has_value() == true);
    CHECK(resident.getId().value() == 1);
    CHECK(resident.getFirstName() == "Juan");
    CHECK(resident.getLastName() == "Dela Cruz");
    CHECK(resident.getAddress() == "Barangay Santo Tomas");
    CHECK(resident.getContactNumber() == "09171234567");
    CHECK(resident.getEmail() == "juan@example.com");
    CHECK(resident.getStatus() == "Active");
}

// Test 2: Resident Information Can Be Accessed and Updated
DROGON_TEST(ResidentInformationAccessTest)
{
    csms::Resident resident(
        "Maria",
        "Santos",
        "Barangay Santo Tomas",
        "09181234567",
        "maria@example.com"
    );

    resident.setContactNumber("09991234567");
    resident.setEmail("maria.santos@example.com");

    CHECK(resident.getContactNumber() == "09991234567");
    CHECK(resident.getEmail() == "maria.santos@example.com");
}

// Test 3: Resident Defaults to Active Status
DROGON_TEST(ResidentDefaultStatusTest)
{
    csms::Resident resident(
        "Pedro",
        "Penduko",
        "Barangay Matino",
        "09123456789",
        "pedro@example.com"
    );

    CHECK(resident.getStatus() == "Active");
}

// Test 4: Resident Has No ID Before Persistence
DROGON_TEST(ResidentNoIdBeforePersistenceTest)
{
    csms::Resident resident(
        "Pedro",
        "Penduko",
        "Barangay Matino",
        "09123456789",
        "pedro@example.com"
    );

    CHECK(resident.getId().has_value() == false);
}

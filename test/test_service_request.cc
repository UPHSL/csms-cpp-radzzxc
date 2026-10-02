#include <drogon/drogon_test.h>
#include "models/ServiceRequest.h"

// Test 1: Service Request Can Be Created
// Verifies that a ServiceRequest can be constructed with sensible domain information.
DROGON_TEST(ServiceRequestCanBeCreated)
{
    csms::ServiceRequest request(
        25,
        "Barangay Clearance",
        "Request for employment requirement",
        "2026-10-02"
    );

    CHECK(request.getResidentId() == 25);
    CHECK(request.getServiceType() == "Barangay Clearance");
    CHECK(request.getDescription() == "Request for employment requirement");
    CHECK(request.getDateRequested() == "2026-10-02");
    CHECK(request.getStatus() == "Pending");
    CHECK(request.getId().has_value() == false);
}

// Test 2: Service Request Information Is Accessible
// Verifies that all supplied fields are accurately retrievable through accessors.
DROGON_TEST(ServiceRequestInformationIsAccessible)
{
    csms::ServiceRequest request(
        10,
        "Certificate Request",
        "Certificate of Indigency for scholarship",
        "2026-10-01",
        "Pending",
        5
    );

    CHECK(request.getId().has_value() == true);
    CHECK(request.getId().value() == 5);
    CHECK(request.getResidentId() == 10);
    CHECK(request.getServiceType() == "Certificate Request");
    CHECK(request.getDescription() == "Certificate of Indigency for scholarship");
    CHECK(request.getDateRequested() == "2026-10-01");
    CHECK(request.getStatus() == "Pending");
}

// Test 3: Resident ID Is Preserved
// Verifies that the residentId is preserved and not altered or replaced.
DROGON_TEST(ServiceRequestResidentIdIsPreserved)
 {
     int expectedResidentId = 25;
     csms::ServiceRequest request(
         expectedResidentId,
         "Barangay Clearance",
         "Employment requirement",
         "2026-10-02"
     );
     CHECK(request.getResidentId() == expectedResidentId);
 }

// Test 4: New Service Request Has an Unassigned ID
// Verifies that a new ServiceRequest prior to persistence has an unassigned ID (nullopt).
DROGON_TEST(NewServiceRequestHasUnassignedId)
{
    csms::ServiceRequest request(
        15,
        "Community Assistance",
        "Financial aid request",
        "2026-10-02"
    );

    CHECK(request.getId().has_value() == false);
    CHECK(request.getId() == std::nullopt);
}

// Test 5: New Service Request Defaults to Pending
// Verifies that omitting status in constructor defaults to "Pending".
DROGON_TEST(NewServiceRequestDefaultsToPending)
{
    csms::ServiceRequest request(
        30,
        "Permit Request",
        "Noise permit for community celebration",
        "2026-10-02"
    );

    CHECK(request.getStatus() == "Pending");
}

// Test 6: Service Request Information Is Independent Between Objects
// Verifies that multiple ServiceRequest objects maintain independent state.
DROGON_TEST(ServiceRequestInformationIsIndependentBetweenObjects)
{
    csms::ServiceRequest request1(
        1,
        "Barangay Clearance",
        "First request description",
        "2026-10-01"
    );

    csms::ServiceRequest request2(
        2,
        "Permit Request",
        "Second request description",
        "2026-10-02"
    );

    CHECK(request1.getResidentId() == 1);
    CHECK(request1.getServiceType() == "Barangay Clearance");
    CHECK(request1.getDescription() == "First request description");
    CHECK(request1.getDateRequested() == "2026-10-01");

    CHECK(request2.getResidentId() == 2);
    CHECK(request2.getServiceType() == "Permit Request");
    CHECK(request2.getDescription() == "Second request description");
    CHECK(request2.getDateRequested() == "2026-10-02");
}

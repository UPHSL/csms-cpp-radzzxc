#include <drogon/drogon_test.h>
#include "models/ServiceRequest.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/ServiceRequestQueryService.h"
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

// Helper to construct and cleanup an isolated temporary SQLite database in the OS temp directory
struct ServiceRequestQueryTestDbCleaner
{
    std::filesystem::path path;

    explicit ServiceRequestQueryTestDbCleaner(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~ServiceRequestQueryTestDbCleaner()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string str() const
    {
        return path.string();
    }
};

static csms::ServiceRequest makeRequest(
    int residentId,
    const std::string& serviceType,
    const std::string& description,
    const std::string& dateRequested,
    const std::string& status = "Pending"
)
{
    return csms::ServiceRequest(residentId, serviceType, description, dateRequested, status);
}

// Test 1: List All Persisted Service Requests
DROGON_TEST(ListAllPersistedServiceRequests)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_list_all.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Barangay Clearance", "Employment requirement", "2026-10-01"));
    repo.save(makeRequest(1, "Certificate of Indigency", "Medical assistance", "2026-10-02"));
    repo.save(makeRequest(2, "Business Permit Inspection", "Store renewal", "2026-10-03"));

    std::vector<csms::ServiceRequest> results = service.listServiceRequests();

    CHECK(results.size() == 3);
}

// Test 2: Empty Listing Returns an Empty Collection
DROGON_TEST(EmptyListingReturnsEmptyCollection)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_empty_list.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    std::vector<csms::ServiceRequest> results = service.listServiceRequests();

    CHECK(results.empty() == true);
}

// Test 3: Listing Uses Required Ordering (dateRequested DESC, then id DESC)
DROGON_TEST(ListingUsesRequiredOrdering)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_ordering.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    // Save deliberately out of order
    csms::ServiceRequest req1 = repo.save(makeRequest(1, "Service A", "Desc A", "2026-09-15"));
    csms::ServiceRequest req2 = repo.save(makeRequest(1, "Service B", "Desc B", "2026-09-18"));
    csms::ServiceRequest req3 = repo.save(makeRequest(1, "Service C", "Desc C", "2026-09-18"));
    csms::ServiceRequest req4 = repo.save(makeRequest(1, "Service D", "Desc D", "2026-09-10"));

    std::vector<csms::ServiceRequest> results = service.listServiceRequests();

    CHECK(results.size() == 4);
    // 2026-09-18 with higher ID (req3) comes first
    CHECK(results[0].getId().value() == req3.getId().value());
    // 2026-09-18 with lower ID (req2) comes second
    CHECK(results[1].getId().value() == req2.getId().value());
    // 2026-09-15 (req1) comes third
    CHECK(results[2].getId().value() == req1.getId().value());
    // 2026-09-10 (req4) comes fourth
    CHECK(results[3].getId().value() == req4.getId().value());
}

// Test 4: View Existing Service Request by ID
DROGON_TEST(ViewExistingServiceRequestById)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_view_by_id.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    csms::ServiceRequest saved = repo.save(makeRequest(10, "Barangay Clearance", "Job application", "2026-10-15", "Pending"));
    int id = saved.getId().value();

    std::optional<csms::ServiceRequest> result = service.viewServiceRequest(id);

    CHECK(result.has_value() == true);
    CHECK(result->getId().value() == id);
    CHECK(result->getResidentId() == 10);
    CHECK(result->getServiceType() == "Barangay Clearance");
    CHECK(result->getDescription() == "Job application");
    CHECK(result->getDateRequested() == "2026-10-15");
    CHECK(result->getStatus() == "Pending");
}

// Test 5: Viewing an Unknown ID Is Handled Safely
DROGON_TEST(ViewingUnknownIdHandledSafely)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_unknown_id.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    std::optional<csms::ServiceRequest> result = service.viewServiceRequest(9999);

    CHECK(result.has_value() == false);
    CHECK(repo.findAll().empty() == true);
}

// Test 6: Partial Service-Type Search Is Case-Insensitive
DROGON_TEST(PartialServiceTypeSearchIsCaseInsensitive)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_partial_type.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Barangay Clearance", "Job application", "2026-10-01"));
    repo.save(makeRequest(2, "Certificate of Indigency", "Medical assistance", "2026-10-02"));

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("cLeAr");

    CHECK(results.size() == 1);
    CHECK(results[0].getServiceType() == "Barangay Clearance");
}

// Test 7: Partial Description Search Is Case-Insensitive
DROGON_TEST(PartialDescriptionSearchIsCaseInsensitive)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_partial_desc.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Barangay Clearance", "Requesting clearance for eMpLoYmEnT requirements", "2026-10-01"));
    repo.save(makeRequest(2, "Community Assistance", "Financial medical aid", "2026-10-02"));

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("employment");

    CHECK(results.size() == 1);
    CHECK(results[0].getServiceType() == "Barangay Clearance");
}

// Test 8: Blank Search Returns All Service Requests
DROGON_TEST(BlankSearchReturnsAllServiceRequests)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_blank_search.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Service A", "Desc A", "2026-10-01"));
    repo.save(makeRequest(2, "Service B", "Desc B", "2026-10-02"));
    repo.save(makeRequest(3, "Service C", "Desc C", "2026-10-03"));

    std::vector<csms::ServiceRequest> listResults = service.listServiceRequests();
    std::vector<csms::ServiceRequest> whitespaceResults = service.searchServiceRequests("       ");
    std::vector<csms::ServiceRequest> emptyResults = service.searchServiceRequests("");

    CHECK(whitespaceResults.size() == listResults.size());
    CHECK(emptyResults.size() == listResults.size());
    for (size_t i = 0; i < listResults.size(); ++i)
    {
        CHECK(whitespaceResults[i].getId().value() == listResults[i].getId().value());
        CHECK(emptyResults[i].getId().value() == listResults[i].getId().value());
    }
}

// Test 9: No Text Match Returns an Empty Collection
DROGON_TEST(NoTextMatchReturnsEmptyCollection)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_no_match.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Barangay Clearance", "Employment", "2026-10-01"));

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("NonExistentTermXYZ");

    CHECK(results.empty() == true);
}

// Test 10: Status Filter Returns Only Matching Status
DROGON_TEST(StatusFilterReturnsOnlyMatchingStatus)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_status_filter.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Request 1", "Desc 1", "2026-10-01", "Pending"));
    repo.save(makeRequest(2, "Request 2", "Desc 2", "2026-10-02", "In Progress"));
    repo.save(makeRequest(3, "Request 3", "Desc 3", "2026-10-03", "Completed"));
    repo.save(makeRequest(4, "Request 4", "Desc 4", "2026-10-04", "Cancelled"));

    std::vector<csms::ServiceRequest> pendingResults = service.searchServiceRequests("", "Pending");

    // Verify Pending filter
    CHECK(pendingResults.size() == 1);
    CHECK(pendingResults[0].getStatus() == "Pending");

    // Verify In Progress filter
    std::vector<csms::ServiceRequest> inProgressResults = service.searchServiceRequests("", "In Progress");
    CHECK(inProgressResults.size() == 1);
    CHECK(inProgressResults[0].getStatus() == "In Progress");

    // Verify Completed filter
    std::vector<csms::ServiceRequest> completedResults = service.searchServiceRequests("", "Completed");
    CHECK(completedResults.size() == 1);
    CHECK(completedResults[0].getStatus() == "Completed");

    // Verify Cancelled filter
    std::vector<csms::ServiceRequest> cancelledResults = service.searchServiceRequests("", "Cancelled");
    CHECK(cancelledResults.size() == 1);
    CHECK(cancelledResults[0].getStatus() == "Cancelled");

    // Unsupported status filter check
    bool threw = false;
    try
    {
        service.searchServiceRequests("", "Approved");
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }
    CHECK(threw == true);
}

// Test 11: Unfiltered Query Includes All Statuses
DROGON_TEST(UnfilteredQueryIncludesAllStatuses)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_unfiltered.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    repo.save(makeRequest(1, "Request 1", "Desc 1", "2026-10-01", "Pending"));
    repo.save(makeRequest(2, "Request 2", "Desc 2", "2026-10-02", "In Progress"));
    repo.save(makeRequest(3, "Request 3", "Desc 3", "2026-10-03", "Completed"));
    repo.save(makeRequest(4, "Request 4", "Desc 4", "2026-10-04", "Cancelled"));

    std::vector<csms::ServiceRequest> results = service.listServiceRequests();

    CHECK(results.size() == 4);
}

// Test 12: Combined Text Search and Status Filter
DROGON_TEST(CombinedTextSearchAndStatusFilter)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_combined.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    // Request A: matches text AND status
    csms::ServiceRequest reqA = repo.save(makeRequest(1, "Barangay Clearance", "Employment requirement", "2026-10-01", "Pending"));
    // Request B: matches text only
    repo.save(makeRequest(2, "Barangay Clearance", "Scholarship requirement", "2026-10-02", "Completed"));
    // Request C: matches status only
    repo.save(makeRequest(3, "Community Assistance", "Employment support", "2026-10-03", "Pending"));
    // Request D: matches neither
    repo.save(makeRequest(4, "Community Assistance", "Medical support", "2026-10-04", "Cancelled"));

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("clearance", "Pending");

    CHECK(results.size() == 1);
    CHECK(results[0].getId().value() == reqA.getId().value());
    CHECK(results[0].getServiceType() == "Barangay Clearance");
    CHECK(results[0].getStatus() == "Pending");
}

// Test 13: Matching Service Request Is Not Duplicated
DROGON_TEST(MatchingServiceRequestIsNotDuplicated)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_not_duplicated.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    // Both serviceType and description contain "Assistance"
    repo.save(makeRequest(1, "Community Assistance", "Assistance requested for medical support", "2026-10-01", "Pending"));

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("assistance");

    CHECK(results.size() == 1);
}

// Test 14: Query Result Preserves Complete Service Request Information
DROGON_TEST(QueryResultPreservesCompleteServiceRequestInformation)
{
    ServiceRequestQueryTestDbCleaner cleaner("test_sr_query_preserves_info.db");
    csms::ServiceRequestRepository repo(cleaner.str());
    csms::ServiceRequestQueryService service(repo);

    csms::ServiceRequest saved = repo.save(makeRequest(42, "Business Permit Inspection", "Annual commercial safety check", "2026-10-25", "In Progress"));
    int id = saved.getId().value();

    std::vector<csms::ServiceRequest> results = service.searchServiceRequests("Inspection");

    CHECK(results.size() == 1);
    CHECK(results[0].getId().value() == id);
    CHECK(results[0].getResidentId() == 42);
    CHECK(results[0].getServiceType() == "Business Permit Inspection");
    CHECK(results[0].getDescription() == "Annual commercial safety check");
    CHECK(results[0].getDateRequested() == "2026-10-25");
    CHECK(results[0].getStatus() == "In Progress");
}
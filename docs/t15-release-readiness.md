# T15 Release Readiness Report

## 1. Developer Information
- **Name:** Radge Michael A. Emperador
- **Student Number:** 23-1857-644
- **Section:** J4A
- **GitHub Username:** radzzxc
- **Technology Stack:** C++ with Drogon (C++20, CMake, Ninja, SQLite3)
- **T15 Branch:** feature/t15-system-hardening

---

## 2. Release Candidate Baseline
Prior to T15 hardening, baseline components completed in tickets T01 through T14 were audited:
- **Application Startup:** Working cleanly without errors.
- **`/health` Endpoint:** Responding 200 OK.
- **Dashboard (`/dashboard`):** Fully operational with SQL aggregation metrics and recent request listing.
- **Resident Management (`/residents`):** Fully operational with native Drogon CSP templates, search, register, edit, and deactivation.
- **Service Request Management (`/service-requests`):** Fully operational with search, status filtering, submission, details, and lifecycle state transitions.
- **Automated Tests (Starter through T14):** 160/160 automated tests passing cleanly.

---

## 3. Hardening Changes

| Area | Issue or Risk Found | Action Taken | Files Changed | Test/Evidence |
| :--- | :--- | :--- | :--- | :--- |
| **Invalid Route Identifiers** | Malformed route strings (e.g., `/residents/abc/edit` or negative/overflow IDs) could cause uncontrolled parsing exceptions. | Added a strict `parseId` helper in `ResidentController` that rejects empty, non-digit, negative, or overflow values before queries run, safely returning a controlled 404 response. | `csms-cpp-radzzxc/controllers/ResidentController.cc` | `HardeningCategory1_MalformedIdentifiersHandledSafely` |
| **Persistence Integrity on Mutation Failure** | If an edit form submits partially valid data with an invalid field (e.g., invalid phone format), persistence must remain atomic and unchanged. | Validated in `ResidentUpdateService` and `ResidentController` that failed validation rejects the update in memory prior to repository calls, leaving existing persisted rows completely intact. | `csms-cpp-radzzxc/controllers/ResidentController.cc` | `HardeningCategory2_RejectedMutationProtectsPersistence` |
| **HTTP Read-Only GET Safety** | Read endpoints (Dashboard, Residents list, Requests list, Request details) must never mutate persistence or alter counters when visited repeatedly. | Verified that all GET controller actions invoke only read queries (`listResidents`, `searchResidents`, `getSummary`, `findById`, `findRecent`) without side effects. | `csms-cpp-radzzxc/controllers/ResidentController.cc`, `csms-cpp-radzzxc/controllers/DashboardController.cc`, `csms-cpp-radzzxc/controllers/ServiceRequestController.cc` | `HardeningCategory3_ReadOnlyRequestsDoNotMutatePersistence` |
| **Cross-Feature Lifecycle Integration** | Soft deactivation of a resident must immediately update Dashboard counts and prevent new service request submissions while keeping existing requests intact and transitionable. | Verified end-to-end integration across `ResidentDeactivationService`, `DashboardService`, and `ServiceRequestSubmissionService`. | `csms-cpp-radzzxc/services/ResidentDeactivationService.cc`, `csms-cpp-radzzxc/services/ServiceRequestSubmissionService.cc` | `HardeningCategory4_CrossFeatureDeactivationIntegrity` |
| **Recent Requests Boundary and Tie-Breaking** | Boundary limit for the recent service requests widget on the dashboard requires deterministic DESC sorting by `dateRequested` and secondary `id` DESC, safely returning an empty set on zero or negative limits. | Audited SQLite query in `ServiceRequestRepository::findRecent` to ensure `ORDER BY dateRequested DESC, id DESC LIMIT ?` handles limit bounds defensively. | `csms-cpp-radzzxc/repositories/ServiceRequestRepository.cc` | `HardeningCategory5_RecentRequestsBoundaryAndOrdering` |
| **SQL Query Safety** | Potential SQL injection risks in user search inputs or dynamic queries. | Reviewed all repositories (`ResidentRepository`, `ServiceRequestRepository`); verified 100% of queries use SQLite prepared statements with parameter binding (`sqlite3_bind_text`, `sqlite3_bind_int`). | `csms-cpp-radzzxc/repositories/ResidentRepository.cc`, `csms-cpp-radzzxc/repositories/ServiceRequestRepository.cc` | Code audit confirmed zero string-concatenated SQL queries. |
| **Repository Hygiene** | CMake test target required registering the new hardening test file and removing accidental duplicate source entries. | Added `test_system_hardening.cc`, deduplicated test source entries in `test/CMakeLists.txt`, and confirmed build artifacts remain safely ignored by `.gitignore`. | `csms-cpp-radzzxc/test/CMakeLists.txt` | Clean CMake configuration and build. |

---

## 4. Five New T15 Hardening Tests

### Test 1: `HardeningCategory1_MalformedIdentifiersHandledSafely`
- **Category:** Category 1 - Invalid or Malformed Input
- **What It Verifies:** Submitting non-numeric or malformed identifiers (e.g., `/residents/abc/edit`, `/residents/not-a-number/edit`, `/service-requests/xyz123`, `/service-requests/bad-id/start`) returns a controlled 404 response without crashing, throwing unhandled exceptions, or altering database counts.
- **Why It Adds Value Beyond Existing Tests:** Previous tests only validated numeric nonexistent IDs (e.g., `999999`). This test verifies framework routing and defensive parsing for arbitrary non-numeric text strings across both GET and POST endpoints.

### Test 2: `HardeningCategory2_RejectedMutationProtectsPersistence`
- **Category:** Category 2 - Rejected Mutation Protects Persistence
- **What It Verifies:** When a resident update fails validation (e.g., valid first name combined with an invalid contact number format), the update is rejected atomically. Subsequent retrieval proves the original persisted database record remained completely unchanged (no partial updates).
- **Why It Adds Value Beyond Existing Tests:** Proves atomic update behavior by re-querying SQLite directly from disk to ensure partial field writes cannot occur when validation errors are present.

### Test 3: `HardeningCategory3_ReadOnlyRequestsDoNotMutatePersistence`
- **Category:** Category 3 - Read-Only Request Safety
- **What It Verifies:** Repeated GET requests across the Dashboard, Resident index, Service Request index, and Service Request details endpoints do not alter database counts or change existing record status.
- **Why It Adds Value Beyond Existing Tests:** Guarantees strict HTTP read-only semantics across all three modules under repeated request loops.

### Test 4: `HardeningCategory4_CrossFeatureDeactivationIntegrity`
- **Category:** Category 4 - Cross-Feature Regression
- **What It Verifies:** When an active resident is deactivated, Dashboard counts update immediately (Active: 0, Inactive: 1), new service request submissions for that resident are rejected with `residentInactive = true`, and pre-existing pending service requests remain intact and can still transition through their lifecycle.
- **Why It Adds Value Beyond Existing Tests:** Tests the complete multi-module interaction between Resident deactivation (T07), Service Request submission eligibility (T09), Service Request status progression (T10), and Dashboard live aggregation (T14).

### Test 5: `HardeningCategory5_RecentRequestsBoundaryAndOrdering`
- **Category:** Category 5 - Repeatability or Boundary Behavior
- **What It Verifies:** When 7 service requests exist across various dates, `findRecent(5)` returns exactly 5 items ordered by `dateRequested DESC, id DESC`. Boundary conditions with limits `<= 0` safely return an empty vector without error.
- **Why It Adds Value Beyond Existing Tests:** Specifically validates the secondary tie-breaker sort (`id DESC`) for same-date requests and verifies edge-case limit handling (`0` and `-1`).

---

## 5. Full Regression Test Results
- **Test Command Used:** `ctest --test-dir build --output-on-failure`
- **Run 1 Result:** 165/165 tests passed (100% pass rate).
- **Run 2 Result:** 165/165 tests passed (100% pass rate).
- **Total Passing Tests:** 165 tests (160 baseline from Starter-T14 + 5 new T15 hardening tests).

---

## 6. Manual Release Verification

| Verification | Result | Notes |
| :--- | :--- | :--- |
| Application starts | PASS | Starts cleanly on localhost:8888. |
| `/health` works | PASS | Returns 200 OK with health status JSON. |
| Dashboard loads | PASS | Displays summary statistics cards and recent service requests table. |
| Resident listing/search works | PASS | Shows table of residents and filters by name/contact correctly. |
| Valid Resident registration works | PASS | Creates active resident and redirects to `/residents?msg=registered`. |
| Invalid Resident registration is rejected | PASS | Displays field error messages and preserves input values without persisting. |
| Resident update works | PASS | Updates editable fields while preserving ID and status. |
| Resident deactivation works and record remains visible | PASS | Marks resident Inactive; record remains visible and searchable in table. |
| Valid Service Request submission for Active Resident works | PASS | Creates service request in Pending state and redirects to list. |
| Missing/Inactive Resident submission is rejected | PASS | Rejects submission with clear user message; no record created. |
| Service Request search/filter/details work | PASS | Search by keyword and filter by status dropdown work together. |
| Valid Service Request workflow works | PASS | Pending -> In Progress -> Completed workflow succeeds via POST buttons. |
| Invalid/terminal transition is rejected | PASS | Terminal requests (Completed/Cancelled) cannot be transitioned further. |
| Dashboard reflects Resident/Service Request changes | PASS | Metrics and recent requests reflect database state immediately upon refresh. |
| Read-only GET operations do not modify persistence | PASS | Repeated page visits produce zero mutations or side effects. |

---

## 7. Repository Hygiene Review
- **Debug Output:** Verified that no temporary `std::cout` or debug trace prints exist in production code paths.
- **Temporary Files:** Verified that temporary SQLite test databases are cleaned up in test fixture destructors.
- **Generated Build/Runtime Artifacts:** Checked `.gitignore` to ensure `build/`, `*.db`, and compiled objects are excluded from git tracking.
- **Accidental Local Database Files:** Confirmed no local test databases are staged or committed.
- **Commented-Out Abandoned Code:** Audited controllers, services, models, and repositories; no abandoned blocks.
- **Secrets/Credentials:** Verified no API keys, tokens, or passwords exist in the repository.
- **Broken Navigation/Links:** Audited header links, buttons, and cancel actions across all CSP templates; all routes link correctly.
- **Direct SQL Parameter Binding:** Confirmed all repository queries utilize prepared statements with parameter binding.

---

## 8. Known Limitations
- **Authentication & Authorization:** In accordance with the course scope for T01-T15, user login, session management, and role-based access control are not implemented.
- **Pagination:** Resident and service request listing tables display all matching rows without server-side pagination, which is sufficient for the target local barangay scope.
- **External Notifications:** Email and SMS notifications for status transitions are not implemented per specification.

No known release-blocking limitations identified within the defined CSMS scope.

---

## 9. Release Candidate Statement

**READY FOR T16 FINAL EXAM**
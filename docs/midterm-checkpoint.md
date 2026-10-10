# Midterm Checkpoint: CSMS Service Request Status Management (T10)

## Section 1: Developer Information
- **Name:** Radge Michael A. Emperador
- **GitHub Username:** radzzxc
- **Primary Technology Stack:** C++20, Drogon, SQLite, CMake + Ninja
- **T10 Branch:** `feature/t10-service-request-status`

---

## Section 2: My T10 Implementation
For T10, I implemented a controlled status workflow for service requests that already exist in our SQLite database. The central component managing this logic is `ServiceRequestStatusService`, which coordinates business transition rules and database updates through `ServiceRequestRepository`. 

When a status change is requested, the service first queries the database using `repository.findById(id)` to retrieve the current record. If the record does not exist, the operation halts immediately with a clear not-found result. Next, the service inspects the record's current status and verifies that the requested target status is one of our four supported options. It then checks whether the transition is allowed using `isAllowedTransition()`. If the transition is invalid (such as trying to complete a pending request directly or repeating the same status), the method stops immediately without touching the database. Only when the transition is valid does the service call `repository.updateStatus(id, newStatus)` to run a parameterized SQL update. Finally, the service reloads the updated record from SQLite and returns a structured `ServiceRequestStatusResult`.

---

## Section 3: My Transition Rules
My implementation enforces the following lifecycle state transitions:

- **Pending to In Progress:** Allowed. This represents staff picking up a newly submitted request and beginning work on it.
- **Pending to Cancelled:** Allowed. A pending request can be cancelled if the resident withdraws it before work begins.
- **In Progress to Completed:** Allowed. Once the requested service is fulfilled, the request reaches its completed state.
- **In Progress to Cancelled:** Allowed. An ongoing request can be cancelled if it cannot be fulfilled or is halted during processing.

### Why Certain Transitions Are Prohibited:
- **Pending to Completed is rejected:** A service request cannot skip the active work stage. Staff must first mark it as "In Progress" before completing it to ensure real processing took place.
- **In Progress to Pending is rejected:** Workflow states move forward, not backward. Once work has begun, a request cannot be reset back to an untouched pending state.
- **Completed is terminal:** A completed request represents finished work and archival history. Allowing transitions away from completed would risk corrupting historical transaction records.
- **Cancelled is terminal:** A cancelled request represents a terminated workflow. Our system does not support reopening cancelled requests.
- **Same-status requests are rejected:** Requesting a transition to the same status (such as `Pending` to `Pending` or `Completed` to `Completed`) is treated as an invalid transition. This ensures the system explicitly validates meaningful state changes rather than treating status updates as blind assignments.

---

## Section 4: Files I Changed
1. **`repositories/ServiceRequestRepository.h` and `ServiceRequestRepository.cc`**
   - *Purpose:* Added the `updateStatus(int id, const std::string& newStatus)` method, which executes the parameterized SQL statement `UPDATE service_requests SET status = ? WHERE id = ?;` and verifies that a database row was actually modified using `sqlite3_changes()`.
2. **`services/ServiceRequestStatusResult.h`**
   - *Purpose:* Created the structured return type for status operations. It clearly tells the caller whether the operation succeeded, if the request was not found, if the target status was unsupported, or if the transition was invalid.
3. **`services/ServiceRequestStatusService.h` and `ServiceRequestStatusService.cc`**
   - *Purpose:* Implemented the business coordination layer. It verifies request existence, enforces the allowed transition rules, protects against invalid updates, and requests database persistence only when valid.
4. **`test/test_service_request_status.cc`**
   - *Purpose:* Implemented all 14 automated Drogon unit tests covering the 13 required lifecycle scenarios and 1 student-designed scenario using isolated temporary SQLite databases.
5. **`test/CMakeLists.txt`**
   - *Purpose:* Added `test_service_request_status.cc` and `../services/ServiceRequestStatusService.cc` to the test executable build target.

---

## Section 5: Problem I Encountered
- **What happened:** During the initial design of the transition check, I needed to decide how same-status requests (like requesting "Pending" when the status is already "Pending") should be handled.
- **What caused it:** In standard database updates, updating a column to its existing value succeeds silently without error. However, T10 explicitly requires that same-status requests be treated as invalid transitions rather than harmless no-ops.
- **How I investigated it:** I reviewed the T10 specification under the "Same-Status Requests" rule, which states that requesting the current status again must be handled as an invalid transition and leave persistence unchanged.
- **How I resolved it:** In `ServiceRequestStatusService::isAllowedTransition()`, I added an explicit guard at the very start: `if (currentStatus == targetStatus) return false;`. Because of that guard, a same-status request falls into the normal invalid-transition path: `ServiceRequestStatusService::updateStatus()` returns `invalidTransition = true` with the standard message (for example, `Invalid status transition from 'Pending' to 'Pending'.`) and never calls the repository. I verified this with automated test `SameStatusRequestIsRejected`.

---

## Section 6: My Student-Designed Test
- **Test Name:** `ExistingRequestProcessesEvenIfResidentIsInactive`
- **What the Test Verifies:** It verifies that an existing service request can continue through its full lifecycle (moving from `Pending` to `In Progress` and then to `Completed`) even if its associated resident is soft-deactivated (marked `Inactive`) after the request was originally submitted.
- **Why I Added This Test:** In T09, we implemented a business rule preventing inactive residents from submitting new service requests. However, T10 emphasizes an important distinction: resident deactivation should not block staff from processing transactions that were already legitimately submitted while the resident was active. This test explicitly verifies that cross-domain deactivation does not lock out ongoing service request workflows.

---

## Section 7: Tools and References Used
- **C++20 Standard Documentation (cppreference.com):** Referenced for std::optional and clean move semantics.
- **Drogon Framework Test Documentation:** Referenced for writing isolated unit test macros (`DROGON_TEST` and `CHECK`).
- **SQLite C/C++ Interface:** Used for prepared statement binding and checking row change counts via `sqlite3_changes()`.
- **Google Antigravity AI Assistant:** Used as a pair programming assistant to discuss architectural separation of concerns, verify test coverage, and format documentation.
- **Claude Code (AI coding assistant):** Used to review the finished implementation against the T10 ticket, tighten the assertions in the nonexistent-ID and invalid-transition tests, and correct this document.

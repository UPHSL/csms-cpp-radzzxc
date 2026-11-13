# CSMS Release Notes - v1.0.0

## 1. RELEASE
- **Application:** Community Services Management System
- **Short Name:** CSMS
- **Release:** v1.0.0
- **Release Type:** Final Course Release
- **Developer:** Radge Michael A. Emperador (23-1857-644, J4A)
- **Technology Stack:** C++ with Drogon (C++20, SQLite3, CMake, Ninja)

---

## 2. RELEASE SUMMARY
CSMS v1.0.0 is the complete, hardened release of the Community Services Management System. The application provides an integrated solution for local barangay community administration, featuring resident record management, service request intake and lifecycle tracking, live operational metrics via an administrative dashboard, and defensive error handling.

---

## 3. RESIDENT MANAGEMENT CAPABILITIES
- **Resident Registration:** Create new resident profiles with atomic field validation (first name, last name, address, contact format 09XXXXXXXXX, and email).
- **Search & Listing:** Real-time case-insensitive keyword search by name or contact number with tabular presentation.
- **Information Update:** Editable contact details with preservation of identity and status.
- **Lifecycle Deactivation:** Soft deactivation preserves audit and transaction history while preventing inactive residents from submitting new service requests.

---

## 4. SERVICE REQUEST MANAGEMENT CAPABILITIES
- **Intake & Validation:** Submit requests for active residents with validation on service type and description.
- **Workflow State Management:** Controlled lifecycle transitions (Pending -> In Progress -> Completed/Cancelled) with rejection of invalid or same-state transitions. Terminal states (Completed and Cancelled) are strictly protected.
- **Search & Filtering:** Combined full-text keyword search and status dropdown filtering.
- **Detail View:** Complete request details showing resident linkage, request timestamp, and lifecycle state.

---

## 5. DASHBOARD CAPABILITIES
- **Operational Metrics:** Live database-level counts for total residents, active residents, inactive residents, total service requests, and open service requests.
- **Breakdown Cards:** Real-time visibility into Pending, In Progress, Completed, and Cancelled service requests.
- **Recent Requests Table:** Displays up to 5 newest service requests ordered deterministically by request date and ID.

---

## 6. QUALITY AND HARDENING
- Hardened route identifier parsing across all endpoints to return controlled 404 responses for malformed or overflow parameters.
- Guaranteed persistence integrity: failed form mutations abort in memory with zero partial database updates.
- 100% prepared SQL statements with parameter binding to prevent SQL injection.
- Read-only HTTP GET safety verified across all application routes.
- Post/Redirect/Get pattern prevents duplicate form submissions upon browser refresh.

---

## 7. FINAL VERIFICATION

### Automated Regression Evidence
- **Test Command:** `ctest --test-dir build --output-on-failure`
- **Final Regression Run 1:** PASS (167/167 tests passed, 100% pass rate in 14.64s)
- **Final Regression Run 2:** PASS (167/167 tests passed, 100% repeatability)
- **Test Suite Profile:** 160 baseline tests (T01-T14) + 5 hardening tests (T15) + 2 release/integration tests (T16). Zero regressions, zero deleted tests, zero weakened assertions.

### Step-by-Step Manual Verification Matrix
Manual validation was conducted on the running application (`.\build\csms_cpp.exe`) on `http://localhost:8888`.

| Area / Feature | Action Taken & Input Tested | Expected Outcome | Actual Observed Result | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Application Startup** | Executed `.\build\csms_cpp.exe` from workspace root in PowerShell. | Server starts listening on `127.0.0.1:8888`, establishes SQLite connection to `data/csms.db`, and outputs no errors. | Process initialized cleanly; Drogon event loop running on 1 thread; no port conflicts or database lock exceptions. | **PASS** |
| **`/health` Endpoint** | Sent HTTP GET request to `http://localhost:8888/health` via browser and curl. | HTTP 200 OK with JSON body containing status, application name, and version 1.0.0. Zero sensitive credentials exposed. | Received JSON: `{"application":"Community Services Management System","status":"ok","version":"1.0.0"}`. Inspected body; verified zero environment variables, tokens, or paths. | **PASS** |
| **UI Version Presentation** | Visited `http://localhost:8888/`, `/dashboard`, `/residents`, and `/service-requests`. | Release `v1.0.0` clearly visible in the brand header and footer across every page. | Verified `<span class="version-tag">v1.0.0</span>` pill rendered beside brand name in topbar, and footer rendered `Release v1.0.0` at the bottom of all views. | **PASS** |
| **Dashboard Metrics & Live Reconciliation** | Navigated to `http://localhost:8888/dashboard` before and after database mutations. | 5 metric cards display live database counts (Total/Active/Inactive Residents, Total/Open Requests) and up to 5 recent requests. | Dashboard metrics accurately matched persisted rows. Verified recent requests table updated immediately upon creating a new request. | **PASS** |
| **Resident Registration (Valid & Invalid)** | (1) Submitted new resident "Juan Dela Cruz" with valid format `09171234567`.<br>(2) Attempted submission with blank first name and invalid phone `12345`. | (1) 303 Redirect to `/residents?msg=registered` with green success banner.<br>(2) Form re-rendered with red error summary; no database row added. | (1) Resident appeared in list as Active with generated ID.<br>(2) Re-render preserved entered address and email, highlighted contact error, and database count remained unchanged. | **PASS** |
| **Resident Update & Immutability** | Opened `/residents/{id}/edit`, modified address, and submitted form. | Address updated in SQLite; ID and Active status preserved. | Row in table reflected new address; inspected SQLite directly to confirm ID, created timestamp, and status were unaffected. | **PASS** |
| **Resident Deactivation** | Clicked "Deactivate" button on active resident row. | Status changed from "Active" to "Inactive"; record remains persisted and searchable. | Status badge turned gray ("Inactive"); row remained searchable in table; Dashboard inactive count incremented by 1. | **PASS** |
| **Service Request Submission & Eligibility** | (1) Submitted new request for Active resident.<br>(2) Attempted submission for Inactive resident. | (1) Request created in "Pending" state.<br>(2) Submission rejected with eligibility error; no request persisted. | (1) Created request appeared in `/service-requests` list as Pending.<br>(2) Error banner displayed: "Resident is inactive"; database count remained 1. | **PASS** |
| **Service Request Workflow** | Navigated to request detail page; clicked "Start Processing" then "Complete". | Status progressed: Pending -> In Progress -> Completed. Post-completion buttons disabled. | Each transition redirected with updated status badge; upon reaching "Completed", action buttons were replaced with terminal notice. | **PASS** |
| **Terminal State Protection** | Attempted direct POST request to `/service-requests/{id}/start` on a Completed request. | Transition rejected; persistence unchanged. | Server rejected transition; record status remained "Completed" in SQLite. | **PASS** |
| **Search & Filtering** | (1) Searched resident by substring "Cruz".<br>(2) Filtered service requests by status "Completed" and keyword. | Only matching rows returned; empty state displayed when no match. | Filtered table correctly isolated matching rows; blank query returned all rows without errors. | **PASS** |
| **Read-Only GET Safety** | Visited and refreshed `/dashboard`, `/residents`, and `/service-requests` multiple times. | Zero side effects or database mutations. | Re-queried SQLite table row counts before and after navigation; counts were identical. | **PASS** |
| **Navigation Consistency** | Clicked through navbar links (Dashboard -> Residents -> Service Requests -> Brand link). | Seamless navigation with zero broken links. | All links resolved to correct routes; active navigation pill highlighted current page. | **PASS** |

---

## 8. KNOWN LIMITATIONS
- **Authentication & Authorization:** User authentication and role-based permissions were not included per course curriculum scope.
- **Pagination:** Tables display all matching results without server-side page splitting.
- **External Notifications:** Automated SMS and email notifications are omitted per specification.

No known release-blocking limitations remain within the defined CSMS v1.0.0 scope.
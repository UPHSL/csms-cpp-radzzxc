# Final Development Checkpoint: CSMS Release (T16)

## 1. DEVELOPER INFORMATION
- **Name:** Radge Michael A. Emperador
- **Student Number:** 23-1857-644
- **Section:** J4A
- **GitHub Username:** radzzxc
- **Technology Stack:** C++ with Drogon (C++20, SQLite3, CMake, Ninja)
- **T16 Branch:** feature/t16-final-release
- **Final Release Version:** 1.0.0

---

## 2. MY FINAL RELEASE IMPLEMENTATION
To establish a single authoritative source of truth for the release version, I introduced a lightweight centralized header at `Version.h`. This structure defines `kApplication`, `kShortName`, `kVersion`, and `kDisplayVersion` as compile-time constants. The `/health` endpoint in `HealthController` dynamically references these constants to construct its JSON response, eliminating hardcoded version numbers. In the user interface, the shared layout template in `views/Layout.cc` consumes `Version.h` to render the version pill in the top navigation bar and display the release version in the application footer. The landing page in `controllers/HomeController.cc` also references this metadata. This design guarantees that if the application version advances in the future, it only needs to be updated in one place.

---

## 3. FILES I CHANGED
1. **`Version.h`**: Created the single authoritative source of truth for release metadata and version `1.0.0`.
2. **`controllers/HealthController.cc`**: Updated `/health` to dynamically pull application name and version from `Version.h`.
3. **`views/Layout.cc`**: Integrated release version `v1.0.0` into the shared header brand and application footer.
4. **`controllers/HomeController.cc`**: Dynamically interpolated `csms::Version` constants into the landing page.
5. **`test/test_final_release.cc`**: Created automated tests verifying release version reporting and the multi-module Final Exam integration scenario.
6. **`test/CMakeLists.txt`**: Registered `test_final_release.cc` and `../controllers/HealthController.cc` in the test executable.
7. **`README.md`**: Updated project overview and sprint documentation to reflect Sprint 6 and final release `v1.0.0`.

---

## 4. MY FINAL EXAMINATION TEST
- **Test Name:** `FinalExamIntegration_ResidentLifecycleServiceRequestAndDashboardFlow`
- **Modules/Responsibilities Covered:** Resident Registration & Deactivation (T04, T07), Service Request Submission & Eligibility Gate (T09), Status Progression Workflow (T10), Dashboard Live Aggregation (T14), and SQLite Disk Persistence.
- **Starting State:** Clean, isolated temporary SQLite database with zero residents and zero service requests.
- **Operations Performed:** Registered an active resident, submitted a pending service request, transitioned request status to "In Progress" and then "Completed", soft-deactivated the resident, verified live dashboard metrics, attempted a new request submission for the inactive resident, and directly inspected persisted database rows.
- **Expected Final State:** Exactly 1 inactive resident, 1 completed service request, 0 open service requests, rejection of new requests for inactive residents, and complete disk consistency.
- **Why This Test Is Meaningful:** It verifies the end-to-end integration of four separate architectural layers under a real transactional scenario with actual SQLite disk operations.

---

## 5. MY FINAL TECHNICAL DECISION
- **Decision:** Placed application release metadata in a central root header (`Version.h`) using `static constexpr std::string_view` constants.
- **Alternative Considered:** Defining version preprocessor macros in CMake (`add_compile_definitions(CSMS_VERSION="1.0.0")`) or reading a runtime configuration file.
- **Reason for My Decision:** A dedicated C++ header provides type safety, zero runtime overhead, clear discoverability, and seamless inclusion across controllers, views, and test suites without build-system coupling.

---

## 6. PROBLEM OR VERIFICATION ISSUE ENCOUNTERED
- **Problem:** When building `csms_cpp_test`, the linker failed with an undefined reference to `vtable for HealthController`.
- **Cause or Investigation:** `test_final_release.cc` instantiated `HealthController` to verify `/health` responses, but `controllers/HealthController.cc` had not been included in `test/CMakeLists.txt`.
- **Resolution:** Added `../controllers/HealthController.cc` to the `csms_cpp_test` target sources in `test/CMakeLists.txt`, allowing the test binary to link cleanly.

---

## 7. FINAL REGRESSION RESULTS
- **Test Command:** `ctest --test-dir build --output-on-failure`
- **Final Regression Run 1:** 167/167 tests passed (100%)
- **Final Regression Run 2:** 167/167 tests passed (100% repeatability)
- **Total Passing Tests Reported:** 167

---

## 8. TOOLS AND REFERENCES USED
- Drogon Web Framework Documentation (HTTP controllers, views, testing macros)
- C++20 Standard Library Reference (std::string_view, std::filesystem, RAII)
- SQLite3 C API Documentation (prepared statements and parameter binding)
- Git & GitHub Classroom Documentation (branching, pull requests, annotated tagging, releases)
- Antigravity AI Assistant (guided verification and documentation review)

---

## 9. FINAL RELEASE DECLARATION

**CSMS v1.0.0 - READY FOR RELEASE**
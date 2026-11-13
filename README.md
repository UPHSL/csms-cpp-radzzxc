[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-2e0aaae1b6195c2367325f4f02e2d04e9abb55f0b24a779b69b11b9e10269abc.svg)](https://classroom.github.com/online_ide?assignment_repo_id=24293563&assignment_repo_type=AssignmentRepo)

# Community Services Management System (CSMS)

## C++ with Drogon

This repository is the C++ implementation of the Community Services Management System for the Programming Languages Laboratory.

- **Current Sprint:** Sprint 6 - Final Release
- **Technology Stack:** C++20, Drogon Web Framework, SQLite3, CMake, CTest
- **Current Version:** `1.0.0`

---

## Project Structure

```text
csms-cpp-radzzxc/
├── controllers/          # HTTP Controllers (Home, Health)
├── models/               # Domain Models & Validators (Resident, ResidentValidator)
├── database/             # SQLite Connection & Schema Management (DatabaseConnection)
├── repositories/         # Persistence Layer (ResidentRepository)
├── test/                 # Automated Test Suite (Drogon Test / CTest)
├── docs/                 # Documentation & Architecture Guides
└── main.cc               # Application Entry Point
```

---

## Requirements & Environment

Ensure the following tools are installed:
- Git
- C++ Compiler (GCC with MinGW-w64 / MSYS2 UCRT64, Clang, or MSVC supporting C++20)
- CMake (>= 3.10)
- Drogon C++ Web Framework
- SQLite3

Verify installation:
```bash
git --version
c++ --version
cmake --version
drogon_ctl version
```

---

## Build & Run Instructions

### 1. Configure the Project
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### 2. Compile the Application
```bash
cmake --build build --parallel
```

### 3. Run the Server
```bash
./build/csms_cpp
```
*(On Windows: `.\build\csms_cpp.exe`)*

The server will start listening at:
- **Home:** `http://127.0.0.1:8888/`
- **Health Check:** `http://127.0.0.1:8888/health`

Expected response for `/health`:
```json
{
  "application": "Community Services Management System",
  "status": "ok",
  "version": "1.0.0"
}
```

---

## Running Automated Tests

Run the complete CTest test suite:
```bash
ctest --test-dir build --output-on-failure
```

---

## Completed Sprints & Tickets
- **Sprint 0: Developer Onboarding:** Established C++ Drogon toolchain, test runner, and `/health` endpoint.
- **Sprint 1: Resident Domain & Validation (T01, T02):** Implemented in-memory `Resident` model and `ResidentValidator`.
- **Sprint 2: Resident Persistence & Services (T03, T04, T05):** SQLite persistence, registration, search, and listing services.
- **Sprint 3: Resident Updates, Deactivation & Service Request Foundation (T06, T07, T08):** Information update, soft deactivation, and `ServiceRequest` model.
- **Sprint 4: Service Request Workflow, Management & Views (T09, T10, T11, T12, T13):** Submission, status transitions, search/filter, and native CSP views.
- **Sprint 5: Dashboard & System Hardening (T14, T15):** SQLite aggregation metrics, defensive route parsing, and 165 regression tests.
- **Sprint 6: Final Release & System Verification (T16):** Single source of truth versioning, final integration test, release documentation, and CSMS v1.0.0 release.

---

## Documentation References

For further details, refer to the documents in the `docs/` folder:
- [Developer Handbook](docs/developer-handbook.md)
- [Architecture Overview](docs/architecture.md)
- [Git Cheatsheet](docs/git-cheatsheet.md)
- [Release Workflow](docs/release-workflow.md)
- [Preliminary Checkpoint Deliverable](docs/prelim-checkpoint.md)
- [Release Notes v1.0.0](docs/release-v1.0.0.md)
- [Final Checkpoint Deliverable](docs/final-checkpoint.md)


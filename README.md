[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-2e0aaae1b6195c2367325f4f02e2d04e9abb55f0b24a779b69b11b9e10269abc.svg)](https://classroom.github.com/online_ide?assignment_repo_id=24293563&assignment_repo_type=AssignmentRepo)

# Community Services Management System (CSMS)

## C++ with Drogon

This repository is the C++ implementation of the Community Services Management System for the Programming Languages Laboratory.

- **Current Sprint:** Sprint 2 – Resident Management Foundation
- **Technology Stack:** C++20, Drogon Web Framework, SQLite3, CMake, CTest
- **Current Version:** `0.1.0`

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
  "version": "0.1.0"
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

- **Sprint 0: Developer Onboarding** — Established C++ Drogon toolchain, test runner, and `/health` endpoint.
- **Sprint 1 - T01: Resident Domain Model** — Implemented in-memory `Resident` model with unassigned ID representation (`std::optional<int>`) and default `Active` status.
- **Sprint 1 - T02: Resident Validation** — Implemented `ResidentValidator` enforcing required fields, whitespace rejection, `09XXXXXXXXX` contact format, and email syntax.
- **Sprint 2 - T03: Establish Resident Persistence** — Implemented file-backed SQLite persistence with `DatabaseConnection`, `ResidentRepository`, prepared statements, and auto-generated IDs.

---

## Documentation References

For further details, refer to the documents in the `docs/` folder:
- [Developer Handbook](docs/developer-handbook.md)
- [Architecture Overview](docs/architecture.md)
- [Git Cheatsheet](docs/git-cheatsheet.md)
- [Release Workflow](docs/release-workflow.md)
- [Preliminary Checkpoint Deliverable](docs/prelim-checkpoint.md)



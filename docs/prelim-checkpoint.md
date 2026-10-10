# Developer Information
- **Name:** Radge Michael A. Emperador
- **GitHub Username:** radzzxc
- **Primary Technology Stack:** C++ with Drogon
- **T03 Branch:** feature/t03-resident-persistence

# My T03 Implementation
Resident data is stored in a file-backed SQLite database using the SQLite C API directly. The persistence architecture is separated into a `DatabaseConnection` class in `database/` that manages connection lifecycles and schema creation, and a `ResidentRepository` class in `repositories/` that handles domain data access. When `save()` is called, the repository prepares an `INSERT` statement, securely binds parameters, and executes the statement against SQLite. The generated row identifier is fetched via `sqlite3_last_insert_rowid()` and mapped to the returned `Resident` object. When `findById()` is called, a parameterized `SELECT` query retrieves the matching record and reconstructs the `Resident` object. If the resident ID does not exist, the repository safely returns `std::nullopt`.

# My Persistence Design Decision
I decided to extract SQLite connection management, directory creation, and schema initialization into a dedicated `DatabaseConnection` class, while keeping domain query logic in `ResidentRepository`. This separates infrastructure concerns from domain data access and allows future repositories (e.g., for other entities in subsequent tickets) to reuse the same database connection wrapper without duplicating connection logic. Additionally, both classes accept configurable database paths, enabling automated tests to execute against isolated, temporary database files that are cleaned up automatically.

# My Database Resource Management
Database connections are opened using `sqlite3_open()` within `DatabaseConnection` and closed using `sqlite3_close()` in its destructor via RAII. Prepared statements (`sqlite3_stmt*`) are finalized using `sqlite3_finalize()` immediately after execution in `save()` and `findById()`. If any database operation fails, error details are captured via `sqlite3_errmsg()`, open statement handles are finalized to prevent leaks, and a `std::runtime_error` exception is thrown.

# My Resident Table Design
- Primary key design: `id INTEGER PRIMARY KEY AUTOINCREMENT`
- Contact number column type: `contact_number TEXT NOT NULL`
- Status column type: `status TEXT NOT NULL DEFAULT 'Active'`

I chose `INTEGER PRIMARY KEY AUTOINCREMENT` so SQLite assigns unique, sequential identifiers automatically upon insert. The `contact_number` column is stored as `TEXT` to preserve leading zeros (e.g., `09171234567`) and prevent arithmetic coercion. The `status` column is defined as `TEXT` with a default of `'Active'` to preserve domain model status constraints.

# Files I Changed
- **File:** `database/DatabaseConnection.h` & `database/DatabaseConnection.cc`\
  **Purpose:** Manages SQLite database connection initialization, directory creation, RAII lifecycle cleanup, and schema creation (`CREATE TABLE IF NOT EXISTS`).
- **File:** `repositories/ResidentRepository.h` & `repositories/ResidentRepository.cc`\
  **Purpose:** Implements the Resident repository pattern, handling parameterized `INSERT` and `SELECT` queries, ID mapping, and `std::nullopt` not-found handling.
- **File:** `test/test_resident_repository.cc`\
  **Purpose:** Contains 8 automated test scenarios (7 required tests and 1 student-designed test) validating persistence, ID generation, field and status preservation, and cross-instance isolation.

# SQL I Can Explain
```sql
INSERT INTO residents (first_name, last_name, address, contact_number, email, status) VALUES (?, ?, ?, ?, ?, ?);
```
This statement inserts a new record into the `residents` table. Each `?` placeholder is a positional parameter safely bound at runtime using `sqlite3_bind_text()` with values from the `Resident` object. This prevents SQL injection attacks. This statement is executed within `ResidentRepository::save()`.

# My Resident Mapping
When `findById()` executes a query, SQLite column values are retrieved using `sqlite3_column_*` functions and passed to the `Resident` constructor. For example, the SQLite column `first_name` (snake_case) maps to the `firstName` parameter, and `contact_number` is read as text via `sqlite3_column_text()` to ensure leading zeros are preserved when instantiating the `Resident` domain object.

# Problem I Encountered
- **Problem or error:**
  Configuring the project after adding the persistence layer produced multiple CMake errors in sequence. First:
  `CMake Error at C:/msys64/ucrt64/lib/cmake/Drogon/FindSQLite3.cmake:47 (add_library):
  add_library cannot create imported target "SQLite3_lib" because another target with the same name already exists.`
  After removing my own redundant `find_package(SQLite3 REQUIRED)` call, a second error appeared:
  `CMake Error at test/CMakeLists.txt:25 (target_link_libraries):
  Target "csms_cpp_test" links to: SQLite::SQLite3, but the target was not found.`
  Once configuration succeeded, the build then failed with undefined-reference linker errors originating from `ResidentRepository.cc`, since it depends on `DatabaseConnection`.

- **Cause:**
  Drogon's own bundled CMake package config already runs its own internal `FindSQLite3.cmake`, which creates an imported target named `SQLite3_lib`. My own explicit `find_package(SQLite3 REQUIRED)` call attempted to register the same imported target a second time, which CMake does not allow. Separately, `test/CMakeLists.txt` referenced the target name `SQLite::SQLite3`, which is the name produced by CMake's own upstream `FindSQLite3` module — not the name Drogon's bundled module actually creates in this project. Finally, `database/DatabaseConnection.cc` was never added to either the root `CMakeLists.txt`'s `target_sources()` call (its collected `DB_SRC` variable was never used) or to the test executable's `add_executable()` source list, so the compiled object file for `DatabaseConnection` never existed for the linker to find.

- **How I resolved it:**
  I removed my own duplicate `find_package(SQLite3 REQUIRED)` call from both `CMakeLists.txt` files and linked directly against the `SQLite3_lib` target that Drogon's package already provides, correcting the mismatched target name in `test/CMakeLists.txt` from `SQLite::SQLite3` to `SQLite3_lib`. I then added `../database/DatabaseConnection.cc` to the test executable's `add_executable()` sources and its corresponding include directory, and added `${DB_SRC}` to the root `target_sources()` call along with the `database/` include directory, so both the main application and the test suite compile and link `DatabaseConnection` correctly. After a full clean rebuild (`Remove-Item -Recurse -Force build`, then reconfigure and rebuild), both the application and the full test suite built and ran successfully with no CMake or linker errors.

# My Student-Designed Test
- **Test name:** `MultipleResidentsPersistWithDistinctAutoIncrementIds`
- **What it tests:** Verifies that multiple distinct `Resident` records with different statuses (`Active` and `Inactive`) can be saved sequentially, that each record receives a distinct auto-incremented primary key, and that each record can be independently retrieved without cross-contamination.
- **Why I chose this scenario:** It confirms that SQLite's auto-increment sequence behaves correctly across consecutive operations and that multiple records of varying states remain isolated and durable.

# Tools and References Used
- **VS Code & CMake Tools:** primary development environment for building, running, and debugging the project.
- **SQLite3 C/C++ API documentation:** reference for correct usage of `sqlite3_prepare_v2`, `sqlite3_bind_*`, `sqlite3_step`, and `sqlite3_finalize`.
- **Drogon Test & CTest frameworks:** used to write and execute the `DROGON_TEST` automated test suite.
- **Gemini AI:** since I am a beginner to C++, I used Gemini to help write parts of the implementation code, and to explain language/library concepts I did not understand (e.g., RAII, move semantics, SQLite C API usage).
- **Claude AI:** used to review and suggest improvements to code I had written, propose architecture structuring ideas (e.g., separating `DatabaseConnection` from `ResidentRepository`), verify that my implementation was correct, and explain how specific parts of the code behaved.


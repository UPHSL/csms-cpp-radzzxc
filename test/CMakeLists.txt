cmake_minimum_required(VERSION 3.5)
project(csms_cpp_test CXX)

add_executable(${PROJECT_NAME} 
    test_main.cc
    test_resident.cc
    test_resident_validator.cc
    test_resident_repository.cc
    ../models/Resident.cc
    ../models/ResidentValidator.cc
    ../repositories/ResidentRepository.cc
    ../database/DatabaseConnection.cc
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../repositories
    ${CMAKE_CURRENT_SOURCE_DIR}/../database)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
# find_package(SQLite3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon SQLite3_lib)

ParseAndAddDrogonTests(${PROJECT_NAME})

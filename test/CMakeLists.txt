cmake_minimum_required(VERSION 3.5)
project(csms_cpp_test CXX)

add_executable(${PROJECT_NAME} 
    test_main.cc
    test_resident.cc
    test_resident_validator.cc
    test_resident_repository.cc
    test_resident_registration_service.cc
    test_resident_query_service.cc
    test_resident_update_service.cc
    test_resident_deactivation_service.cc
    test_service_request.cc
    test_service_request_submission.cc
    test_service_request_status.cc
    test_service_request_query_service.cc
    test_resident_management_interface.cc
    test_service_request_management_interface.cc
    ../models/Resident.cc
    ../models/ResidentValidator.cc
    ../models/ServiceRequest.cc
    ../models/ServiceRequestValidator.cc
    ../repositories/ResidentRepository.cc
    ../repositories/ServiceRequestRepository.cc
    ../database/DatabaseConnection.cc
    ../services/ResidentRegistrationService.cc
    ../services/ResidentQueryService.cc
    ../services/ResidentUpdateService.cc
    ../services/ResidentDeactivationService.cc
    ../services/ServiceRequestSubmissionService.cc
    ../services/ServiceRequestStatusService.cc
    ../services/ServiceRequestQueryService.cc
    ../controllers/ResidentController.cc
    ../controllers/ServiceRequestController.cc
    ../views/ResidentView.cc
    ../views/Layout.cc
)

# Compile the CSP templates in ../views (T13) into this test executable too,
# so controller tests render the real views
drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../views
                    ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../repositories
    ${CMAKE_CURRENT_SOURCE_DIR}/../database
    ${CMAKE_CURRENT_SOURCE_DIR}/../services
    ${CMAKE_CURRENT_SOURCE_DIR}/../controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/../views)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
# find_package(SQLite3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon SQLite3_lib)

ParseAndAddDrogonTests(${PROJECT_NAME})
project(test_serialization)

add_definitions(-DDEV_MODE)

set(${PROJECT_NAME}_SOURCES
        #bae_class_json.cpp
        base_container_json.cpp
        #base_tuple_json.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        otterbrix::serialization
        CONAN_PKG::catch2
        CONAN_PKG::boost
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

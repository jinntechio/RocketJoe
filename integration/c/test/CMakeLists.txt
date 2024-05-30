project(test_c_otterbrix)

add_definitions(-DDEV_MODE)

set( ${PROJECT_NAME}_SOURCES

)

add_executable(${PROJECT_NAME} main.cpp ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
        ${PROJECT_NAME} PRIVATE

        cpp_otterbrix

        otterbrix::test_generaty
        otterbrix::log
        otterbrix::memory_storage
        otterbrix::collection
        otterbrix::cursor
        otterbrix::session
        otterbrix::document
        otterbrix::wal
        otterbrix::disk
        otterbrix::locks

        CONAN_PKG::spdlog
        CONAN_PKG::msgpack-cxx
        #CONAN_PKG::date
        CONAN_PKG::catch2
        CONAN_PKG::crc32c
        CONAN_PKG::abseil
        CONAN_PKG::rocksdb
        CONAN_PKG::benchmark
        CONAN_PKG::actor-zeta
        CONAN_PKG::boost
        CONAN_PKG::fmt
        #CONAN_PKG::libcurl
        CONAN_PKG::zlib
        CONAN_PKG::bzip2
        CONAN_PKG::libbacktrace

)

if (WASM_BUILD)
    target_link_libraries(
            ${PROJECT_NAME} PRIVATE
            CONAN_PKG::proxy-wasm-cpp-host
    )
endif ()

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

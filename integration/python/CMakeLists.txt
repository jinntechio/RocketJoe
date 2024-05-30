project(otterbrix)

set(${PROJECT_NAME}_headers
        convert.hpp
        spaces.hpp
        wrapper_client.hpp
        wrapper_collection.hpp
        wrapper_database.hpp
        wrapper_cursor.hpp
        wrapper_document.hpp
        wrapper_document_id.hpp
)

set(${PROJECT_NAME}_sources
        convert.cpp
        main.cpp
        spaces.cpp
        wrapper_client.cpp
        wrapper_collection.cpp
        wrapper_database.cpp
        wrapper_cursor.cpp
        wrapper_document.cpp
        wrapper_document_id.cpp
)

set(otterbrix_LIBS
        otterbrix::log
        otterbrix::memory_storage
        otterbrix::collection
        otterbrix::cursor
        otterbrix::session
        otterbrix::document
        otterbrix::wal
        otterbrix::disk
        otterbrix::locks
        otterbrix::ql
)

#SET(PYTHON_EXECUTABLE python3)
#find_package(Python COMPONENTS Interpreter Development REQUIRED)
#find_package(pybind11 REQUIRED)
#find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
#find_package(Python3 COMPONENTS Development)
find_package(pybind11 CONFIG REQUIRED)
#include_directories(${Python3_INCLUDE_DIRS})

#include_directories(${PROJECT_SOURCE_DIR})

pybind11_add_module(${PROJECT_NAME} ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})

target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        ${otterbrix_LIBS}
        cpp_otterbrix
        CONAN_PKG::spdlog
        CONAN_PKG::pybind11
        CONAN_PKG::msgpack-cxx
        #CONAN_PKG::date
        CONAN_PKG::crc32c
        CONAN_PKG::abseil
        CONAN_PKG::rocksdb
        CONAN_PKG::actor-zeta
        CONAN_PKG::boost
        CONAN_PKG::fmt
        #CONAN_PKG::libcurl
        CONAN_PKG::zlib
        CONAN_PKG::bzip2
        #CONAN_PKG::libbacktrace
        #CONAN_PKG::openssl

)
if (WASM_BUILD)
    target_link_libraries(
            ${PROJECT_NAME} PRIVATE
            ${otterbrix_LIBS}
            CONAN_PKG::proxy-wasm-cpp-host
    )
endif ()

pybind11_extension(${PROJECT_NAME})

if(NOT MSVC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug|RelWithDebInfo)
    # Strip unnecessary sections of the binary on Linux/macOS
    pybind11_strip(${PROJECT_NAME})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")

#target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})

if(SKBUILD)
    install(DIRECTORY otterbrix DESTINATION  . )
    install(TARGETS ${PROJECT_NAME} DESTINATION  ./otterbrix )
    if (WASM_BUILD)
        install(FILES "${CMAKE_BINARY_DIR}/build_tools/libiwasm.so" DESTINATION  ./otterbrix )
    endif()
endif()

if (DEV_MODE)
    file(GLOB PY_TESTS
        "tests/*.py"
    )
    file(COPY ${PY_TESTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

project(cpp_otterbrix)


set(${PROJECT_NAME}_sources
      base_spaces.cpp
      wrapper_dispatcher.cpp
      otterbrix.cpp
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
        otterbrix::sql
)


include_directories(
        ${PROJECT_SOURCE_DIR}
)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_sources})


target_link_libraries(
        ${PROJECT_NAME} PUBLIC
        ${otterbrix_LIBS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "otterbrix")

add_library(${PROJECT_NAME}_shared SHARED ${${PROJECT_NAME}_sources})

target_link_libraries(
        ${PROJECT_NAME}_shared PRIVATE
        ${otterbrix_LIBS}
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

#set_target_properties({PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "cpp_otterbrix_shared")


if (DEV_MODE OR ALLOW_BENCHMARK)
    set(${PROJECT_NAME}_headers
            wrapper_dispatcher.hpp
    )

    set(${PROJECT_NAME}_sources
            wrapper_dispatcher.cpp
    )

    add_library(otterbrix_${PROJECT_NAME}
            ${${PROJECT_NAME}_headers}
            ${${PROJECT_NAME}_sources}
    )

    add_library(otterbrix::${PROJECT_NAME} ALIAS otterbrix_${PROJECT_NAME})

    set_property(TARGET otterbrix_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

    target_link_libraries(
            otterbrix_${PROJECT_NAME} PUBLIC

            ${otterbrix_LIBS}

            CONAN_PKG::spdlog
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
endif()

if (WASM_BUILD)
    target_link_libraries(
            otterbrix_${PROJECT_NAME} PUBLIC
            CONAN_PKG::proxy-wasm-cpp-host
    )
endif ()

if (DEV_MODE)
    add_subdirectory(test)
endif()

if (ALLOW_BENCHMARK)
    add_subdirectory(benchmark)
endif()

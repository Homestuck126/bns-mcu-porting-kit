
project(bns-client C)

if (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
            "FATAL: In-source builds are not allowed. You should create a separate directory for build files."
            )
endif ()

add_definitions(-Wall -Wextra)

include(CTest)

enable_testing()

# 3rdparty secp256k1
add_definitions(-DUSE_NUM_NONE=1)
add_definitions(-DUSE_FIELD_INV_BUILTIN=1)
add_definitions(-DUSE_SCALAR_INV_BUILTIN=1)
add_definitions(-DECMULT_WINDOW_SIZE=2)
add_definitions(-DECMULT_GEN_PREC_BITS=2)
add_definitions(-DENABLE_MODULE_RECOVERY)
add_definitions(-DUSE_ECMULT_STATIC_PRECOMPUTATION)
add_definitions(-DUSE_FORCE_WIDEMUL_INT64)
add_subdirectory(3rdparty/secp256k1)

# 3rdparty cJSON
add_subdirectory(3rdparty/cJSON)

# bns-client-lib
set(LOG_LEVEL "Debug" CACHE STRING "LOG_LEVEL")
if (${LOG_LEVEL} STREQUAL "Off")
    add_definitions(-DLOG_LEVEL=0)
elseif (${LOG_LEVEL} STREQUAL "Error")
    add_definitions(-DLOG_LEVEL=1)
elseif (${LOG_LEVEL} STREQUAL "Warn")
    add_definitions(-DLOG_LEVEL=2)
elseif (${LOG_LEVEL} STREQUAL "Info")
    add_definitions(-DLOG_LEVEL=3)
elseif (${LOG_LEVEL} STREQUAL "Debug")
    add_definitions(-DLOG_LEVEL=4)
elseif (${LOG_LEVEL} STREQUAL "Trace")
    add_definitions(-DLOG_LEVEL=5)
endif (${LOG_LEVEL} STREQUAL "Off")

option(BNS_LOG_DETAIL "BNS_LOG_DETAIL" OFF)
if (${BNS_LOG_DETAIL})
    message("BNS_LOG_DETAIL=${BNS_LOG_DETAIL}")
    add_definitions(-DLOG_DETAIL)
endif ()

option(BNS_LOG_LINE_NUM "BNS_LOG_LINE_NUM" OFF)
if (${BNS_LOG_LINE_NUM})
    message("BNS_LOG_LINE_NUM=${BNS_LOG_LINE_NUM}")
    add_definitions(-DLOG_LINE_NUM)
endif ()

add_subdirectory(src)

# bns-client-test
option(BNS_CLIENT_BUILD_TEST "BNS_CLIENT_BUILD_TEST" ON)
if (${BNS_CLIENT_BUILD_TEST})
    add_subdirectory(test)
endif ()

# bns-client-example
option(BNS_CLIENT_BUILD_EXAMPLE "BNS_CLIENT_BUILD_EXAMPLE" ON)
if (${BNS_CLIENT_BUILD_EXAMPLE})
    add_subdirectory(example)
endif ()

if (NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    set(BNS_CLIENT_INCLUDE_DIRS ${BNS_CLIENT_INCLUDE_DIRS} PARENT_SCOPE)
endif ()

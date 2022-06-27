cmake_minimum_required(VERSION 2.8.10)
project(bns-client-example)

file(GLOB SOURCES LIST_DIRECTORIES false *.c)

set(SOURCES ${SOURCES})

include_directories(${CURL_INCLUDES})
include_directories(${BNS_CLIENT_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE bns-client-lib curl)

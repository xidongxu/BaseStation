cmake_minimum_required (VERSION 3.15)

file(GLOB SOURCES *.c *.cpp)
add_library(source ${SOURCES})

target_link_libraries(source cJSON)
include_directories(${CMAKE_SOURCE_DIR}/vendor/cJSON/)

target_link_libraries(source asio)
include_directories(${CMAKE_SOURCE_DIR}/vendor/asio/include/)

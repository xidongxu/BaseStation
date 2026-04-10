cmake_minimum_required (VERSION 3.8)

file(GLOB SOURCES *.c *.cpp)
add_library(source ${SOURCES})
set_property(TARGET source PROPERTY CXX_STANDARD 20)

target_link_libraries(source cJSON)
include_directories(${CMAKE_SOURCE_DIR}/vendor/cJSON/)

target_link_libraries(source asio)
include_directories(${CMAKE_SOURCE_DIR}/vendor/asio/include/)

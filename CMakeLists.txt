cmake_minimum_required (VERSION 3.8)

project ("BaseStation")
add_executable (BaseStation "startup.cpp" "startup.h")
set_property(TARGET BaseStation PROPERTY CXX_STANDARD 20)

add_subdirectory(vendor)
add_subdirectory(source)

target_link_libraries(BaseStation cli::cli)
include_directories(${CMAKE_SOURCE_DIR}/vendor/cli/include/)

target_link_libraries(BaseStation lyra)
include_directories(${CMAKE_SOURCE_DIR}/vendor/lyra/include/)

target_link_libraries(BaseStation source)
include_directories(${CMAKE_SOURCE_DIR}/source/)

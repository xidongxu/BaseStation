cmake_minimum_required (VERSION 3.15)

project("BaseStation")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if(NOT DEFINED CMAKE_SYSTEM_VERSION)
        set(CMAKE_SYSTEM_VERSION 10.0)
    endif()
    # Windows 10
    if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 10.0)
        add_definitions(-D_WIN32_WINNT=0x0A00)
    # Windows 8.1
    elseif(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 6.3)
        add_definitions(-D_WIN32_WINNT=0x0603)
    # Windows 8
    elseif(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 6.2)
        add_definitions(-D_WIN32_WINNT=0x0602)
    # Windows 7
    else()
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()
endif()

if(MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling AddressSanitizer for MSVC")
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
        add_compile_options(/Od)
        add_compile_options(/Zi)
        add_link_options(/debug)
    endif()
endif()

add_executable(BaseStation "startup.cpp" "startup.h")

add_subdirectory(vendor)
add_subdirectory(source)

target_link_libraries(BaseStation cli::cli)
include_directories(${CMAKE_SOURCE_DIR}/vendor/cli/include/)

target_link_libraries(BaseStation lyra)
include_directories(${CMAKE_SOURCE_DIR}/vendor/lyra/include/)

target_link_libraries(BaseStation source)
include_directories(${CMAKE_SOURCE_DIR}/source/)

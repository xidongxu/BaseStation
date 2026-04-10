set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
add_definitions(-DASIO_STANDALONE)
add_definitions(-DUSE_BF_DEC)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE include)


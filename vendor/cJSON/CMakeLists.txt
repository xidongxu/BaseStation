file(GLOB SOURCES *.c *.cpp)
add_library(cJSON ${SOURCES})
target_include_directories(cJSON PUBLIC .)

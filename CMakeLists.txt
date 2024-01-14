cmake_minimum_required(VERSION 3.20.0)

set(lib xtty)

file(GLOB SRCSC "./*.c")
file(GLOB SRCSH "./*.h")
add_library(${lib} ${SRCSH} ${SRCSC})
set_target_properties(${lib} PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(${lib} PUBLIC zephyr_interface)
target_include_directories(${lib} PUBLIC ./)
target_link_libraries(app PRIVATE ${lib})

cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 17)

project(ipr_2_1 C)

#add_custom_target(example)

add_executable(ipr_2_1 main.c)

add_executable(example example.c)

add_executable(new_ta exammple2.c)
cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_STANDART 11)
set(CMAKE_CXX_STANDARD 17)
project (waterpipe C CXX ASM)
add_executable( waterpipe 
                waterpipe.c)

target_link_libraries(  waterpipe
                        wiringPi)
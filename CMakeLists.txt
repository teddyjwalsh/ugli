cmake_minimum_required (VERSION 2.8.11)
project (ugli_proj)

file(GLOB ugli_src "include/*.h" "src/*.cpp")
set(glad_src "./glad/src/glad.c")

link_directories("./lib64")
find_library(glfw glfw)
add_library(ugli ${ugli_src} ${glad_src})

target_include_directories(ugli PUBLIC ./)
target_include_directories(ugli PUBLIC ./include)
target_include_directories(ugli PUBLIC ./glad/include)
target_compile_options(ugli PRIVATE "-ggdb")
target_compile_options(ugli PRIVATE "-DGLM_ENABLE_EXPERIMENTAL")
set_property(TARGET ugli PROPERTY CXX_STANDARD 17)
target_link_libraries(ugli ${glfw})
target_compile_options(ugli PRIVATE "-std=c++17")


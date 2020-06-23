cmake_minimum_required (VERSION 3.0.0)
project (ugli_proj)

file(GLOB ugli_src "include/*.h" "src/*.cpp")
set(glad_src "./glad/src/glad.c")

link_directories("./lib64")
find_library(glfw glfw)
find_library(dl dl)
find_library(freetype freetype)
add_executable(ugli ${ugli_src} ${glad_src})

target_include_directories(ugli PUBLIC ./)
target_include_directories(ugli PUBLIC ./include)
target_include_directories(ugli PUBLIC ./glad/include)
target_include_directories(ugli PUBLIC ./freetype)
target_compile_options(ugli PRIVATE "-ggdb" "-O3")
target_compile_options(ugli PRIVATE "-DGLM_ENABLE_EXPERIMENTAL")
target_compile_options(ugli PRIVATE "-DTINYOBJLOADER_IMPLEMENTATION")
set_property(TARGET ugli PROPERTY CXX_STANDARD 17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ugli PRIVATE -Wno-psabi)
    target_compile_options(ugli PRIVATE -std=c++17)
    find_library(glfw glfw)
    target_link_libraries(ugli ${glfw})
    find_library(fs stdc++fs)
    find_library(dl dl)
    find_library(freetype freetype)
    find_library(boost_iostreams boost_iostreams)
    message(CMAKE_BUILD_TYPE)
    set(MY_DEFINITION 
        $<$<CONFIG:Debug>:definition_for_debug>
        $<$<CONFIG:RelWithDebInfo>:definition_for_rel_with_debug>
        $<$<CONFIG:Release>:definition_for_release> 
        $<$<CONFIG:MinSizeRel>:definition_for_tight_release>
                        )
    target_link_libraries(ugli ${dl})
    target_link_libraries(ugli stdc++fs)
    target_link_libraries(ugli ${boost_iostreams})
elseif( "\"${CMAKE_CXX_COMPILER_ID}\"" MATCHES MSVC )
    link_directories("C:/Boost/lib")
    target_include_directories(ugli PUBLIC "C:/Boost/include/boost-1_73")
    find_library(glfw glfw3 "./lib64")
    find_library(freetype freetype.lib "./lib64")
    target_link_libraries(ugli ${glfw})
endif()
target_link_libraries(ugli ${freetype})
target_compile_options(ugli PRIVATE "-std=c++17")


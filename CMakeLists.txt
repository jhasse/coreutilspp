cmake_minimum_required (VERSION 3.8)

project(rm++ LANGUAGES CXX)

file(GLOB SRC src/*.cpp)
add_executable(rm++ ${SRC})

target_compile_features(rm++ PUBLIC cxx_std_17)

find_package(Boost COMPONENTS program_options)
target_link_libraries(rm++ PRIVATE ${Boost_LIBRARIES})
target_link_libraries(rm++ PRIVATE stdc++fs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")

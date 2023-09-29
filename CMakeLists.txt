cmake_minimum_required (VERSION 3.15)

project(coreutilspp LANGUAGES CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
	message(STATUS "IPO / LTO enabled")
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
	message(STATUS "IPO / LTO not supported: <${error}>")
endif()

add_subdirectory(subprojects/ThreadPool)

file(GLOB SRC src/*.cpp)
add_executable(rm ${SRC} windows/default.manifest)
target_link_libraries(rm PRIVATE ThreadPool)

target_compile_features(rm PUBLIC cxx_std_17)
target_include_directories(rm PRIVATE subprojects/Lyra/include)
if(WIN32)
	target_compile_definitions(rm PRIVATE UNICODE)
endif()

add_executable(cat src/cat/cat.cpp windows/default.manifest)
target_compile_features(cat PUBLIC cxx_std_17)
target_include_directories(cat PRIVATE subprojects/Lyra/include)

add_executable(touch src/touch/touch.cpp windows/default.manifest)
target_compile_features(touch PUBLIC cxx_std_17)
target_include_directories(touch PRIVATE subprojects/Lyra/include)

add_executable(ls src/ls/ls.cpp windows/default.manifest)
target_compile_features(ls PUBLIC cxx_std_17)
target_include_directories(ls PRIVATE subprojects/Lyra/include)

add_executable(env src/env/env.cpp windows/default.manifest)
target_compile_features(env PUBLIC cxx_std_17)
target_include_directories(env PRIVATE subprojects/Lyra/include)

if(NOT MSVC)
	target_link_libraries(rm PRIVATE stdc++fs)
	target_link_libraries(touch PRIVATE stdc++fs)
	target_link_libraries(ls PRIVATE stdc++fs)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
endif()

# FetchContent = CMake 3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(IntroProject C CXX)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
else()
	# During development the framework lives in parent folder.
	add_subdirectory("../../../framework/" "${CMAKE_BINARY_DIR}/framework/")
endif()

add_executable(Application "src/Exercises.cpp")
target_compile_features(Application PRIVATE cxx_std_20)
target_link_libraries(Application PRIVATE CGFramework)
enable_sanitizers(Application)
set_project_warnings(Application)

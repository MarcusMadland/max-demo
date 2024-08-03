# Project
cmake_minimum_required(VERSION 3.10.2) # version supported by android studio
project(max-demo)
cmake_policy(SET CMP0054 NEW)

# Standard
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Makefile generators on apple need this flag to compile mixed objective/c++
if(APPLE AND NOT XCODE)
	set(CMAKE_CXX_FLAGS "-ObjC++")
endif()

# Directories
if(NOT ENGINE_DIR)
	set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../max" CACHE STRING "Location of max.")
elseif(NOT IS_ABSOLUTE "${ENGINE_DIR}")
	get_filename_component(ENGINE_DIR "${ENGINE_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Sub directories
add_subdirectory(${ENGINE_DIR} max)

# max-demo
include_directories("${ENGINE_DIR}/include")
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} max)
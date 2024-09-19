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
include(${ENGINE_DIR}/cmake/max/project.cmake)

# max-demo
add_max_project(${PROJECT_NAME})

# Include 3rdparties
include(cmake/dear-imgui.cmake)

# Link to 3rdparties
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
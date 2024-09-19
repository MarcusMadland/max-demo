file(
	GLOB #
	DEAR_IMGUI_SOURCES #
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui/*.cpp #
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui/*.h #
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui/*.inl #
)
set(DEAR_IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

add_library(imgui STATIC ${DEAR_IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${DEAR_IMGUI_INCLUDE_DIR})

set_target_properties(imgui PROPERTIES FOLDER "3rdparty")


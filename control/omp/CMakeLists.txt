cmake_minimum_required(VERSION 3.19)
project(SampVoiceControlOmp LANGUAGES CXX)

# open.mp SDK checkout (MIT); not stored in this repository, see specs/001-port-03dl-openmp/quickstart.md
set(OMP_DEPS_DIR "" CACHE PATH "Folder that contains the open.mp-sdk and open.mp-network checkouts")
if(NOT OMP_DEPS_DIR)
	message(FATAL_ERROR "Set -DOMP_DEPS_DIR=<folder containing open.mp-sdk and open.mp-network>")
endif()

add_subdirectory(${OMP_DEPS_DIR}/open.mp-sdk ${CMAKE_BINARY_DIR}/omp-sdk)
add_subdirectory(${OMP_DEPS_DIR}/open.mp-network ${CMAKE_BINARY_DIR}/omp-network)

set(CMAKE_CXX_STANDARD 17)

# The control core (../main.cpp) is compiled unchanged into the component. `shim` is searched first so that
# <ysf/globals.h>, <util/raknet.hpp> and <util/host.hpp> resolve to the open.mp versions.
add_library(SampVoiceControl SHARED
	Component.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../libraries/pawn/amxplugin.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../voice/embed.cpp)

target_include_directories(SampVoiceControl BEFORE PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/shim)
target_include_directories(SampVoiceControl PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../common
	${CMAKE_CURRENT_SOURCE_DIR}/../libraries
	${CMAKE_CURRENT_SOURCE_DIR}/../libraries/pawn)

target_link_libraries(SampVoiceControl PRIVATE OMP-SDK OMP-Network)
set_target_properties(SampVoiceControl PROPERTIES PREFIX "")

target_compile_definitions(SampVoiceControl PRIVATE SAMPVOICE_OMP)

if(MSVC)
	target_compile_definitions(SampVoiceControl PRIVATE
		WIN32_LEAN_AND_MEAN NOMINMAX _CRT_RAND_S _CRT_SECURE_NO_WARNINGS)
	target_link_libraries(SampVoiceControl PRIVATE ws2_32)
endif()

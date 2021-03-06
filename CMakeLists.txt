# Copyright (c) 2017 nyorain
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required (VERSION 3.6.0)
project(ny)

# increase this version on a new release
set(vmajor 0)
set(vminor 1)
set(vpatch 0)

# general building options
option(Examples "Build the ny examples" off)
option(Debug "Include debug symbols" on)
option(Depend "Make ny depend on nytl. Developement option" on)
option(Android "Build nytl for android. Experimental at the moment. Requires ndk" off)

# to disable ninja byproduct warning
if(${CMAKE_VERSION} GREATER 3.1)
	cmake_policy(SET CMP0058 NEW)
endif()

# include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") # public headers
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src") # private src headers
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include") # e.g. config.hpp

# external libraries and headers
include_directories("${CMAKE_BINARY_DIR}/external/install/include")
link_directories("${CMAKE_BINARY_DIR}/external/install/lib")

# cmake build options
# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake")

# enable all sane warnings
# for msvc we first check if some level is set by default and replace it
set(is_clang CMAKE_CXX_COMPILER_ID MATCHES "Clang")
if(MSVC)
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		list(APPEND CMAKE_CXX_FLAGS "/W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR is_clang)
	set(CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} \
		-Wall -Wextra -Wpedantic \
		-Wno-missing-braces -Wno-unused-const-variable")
endif()

# build type
if(Debug)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

# dependent config options
# check for available packages
find_package(OpenGL)
find_package(EGL)
find_package(Vulkan)

if(UNIX)
	find_package(X11 COMPONENTS Xcursor)
	find_package(XCB COMPONENTS ewmh xkb image icccm shm)
	find_package(Wayland COMPONENTS client egl)
	find_package(XKBCommon)
endif()

# dependent options
include(CMakeDependentOption)

cmake_dependent_option(WithEgl
	"Built ny with EGL support. Requires GL" on
	"EGL_FOUND" off)

cmake_dependent_option(WithVulkan
	"Build ny with Vulkan support" on
	"VULKAN_FOUND" off)

cmake_dependent_option(WithXkbCommon
	"Build with xkbcommon support. Required for wayland and x11" on
	"XKBCOMMON_FOUND" off)

cmake_dependent_option(WithX11
	"Build ny with an x11 backend" on
	"WithXkbCommon;X11_FOUND;XCB_FOUND" off)

cmake_dependent_option(WithWayland
	"Build ny with a wayland backend" on
	"WithXkbCommon;WAYLAND_FOUND" off)

cmake_dependent_option(WithWinapi
	"Build ny with a winapi backend" on
	"WIN32" off)

cmake_dependent_option(WithGl
	"Build ny with wgl/glx support" on
	"WithX11 OR WithWinapi;OPENGL_FOUND" off)


# warn if no built-in backends can be compiled
if(NOT WithWayland AND NOT WithX11 AND NOT WithWinapi AND NOT Android)
	message(WARNING "Could not find any backend to build ny with")
endif()

# in future (when nytl is stable) we can first check whether there is a
# suitable version of nytl installed and if so, use it.
set(nytl_location "own")
set(nytl_own true)

include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external/install
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

ExternalProject_Add(nytl_ep
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/nytl
	GIT_REPOSITORY https://github.com/nyorain/nytl.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

ExternalProject_Add_Step(nytl_ep
	forceinstall
	DEPENDEES configure
	DEPENDERS install
	ALWAYS 1)

ExternalProject_Add(dlg_ep
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/dlg
	GIT_REPOSITORY https://github.com/nyorain/dlg.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

ExternalProject_Add_Step(dlg_ep
	forceinstall
	DEPENDEES configure
	DEPENDERS install
	ALWAYS 1)

add_definitions(-DDLG_BASE_PATH="${CMAKE_SOURCE_DIR}")
add_definitions(-DDLG_HEADER_ONLY=0)

# install nytl as well since public ny headers depend on them
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/external/install/." DESTINATION .)

# config file
# first set all macros (they have NY_ prefix since they are macros) for config.hpp
set(NY_WithGl ${WithGl})
set(NY_WithEgl ${WithEgl})
set(NY_WithVulkan ${WithVulkan})
set(NY_WithXkbCommon ${WithXkbCommon})

set(NY_WithAndroid ${Android})
set(NY_WithWinapi ${WithWinapi})
set(NY_WithX11 ${WithX11})
set(NY_WithWayland ${WithWayland})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp" DESTINATION include/ny)

# pkg-config
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/ny.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/ny.pc"
	@ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ny.pc" DESTINATION lib/pkgconfig)

# subdirs
# src/ny for building libny, include for installing headers
add_subdirectory(src/ny)
add_subdirectory(include)

if(Examples)
	add_subdirectory(src/examples)
endif()


# uninstall target
# this makes it possible to uninstall ny ('ninja uninstall') after it has been installed
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/src/cmake/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P
	${CMAKE_CURRENT_BINARY_DIR}/src/cmake/uninstall.cmake)

# print configuration at the end of the cmake config process
# normalize all vars (i.e. true/ON/1/yes all will be on) to make it look uniform
# function that normalizes bool vars to on or off.
function(normalize arg)
	if(${arg})
		set(${arg} 1 PARENT_SCOPE)
	else()
		set(${arg} 0 PARENT_SCOPE)
	endif()
endfunction()

normalize(Android)
normalize(WithGl)
normalize(WithEgl)
normalize(WithVulkan)
normalize(WithWinapi)
normalize(WithX11)
normalize(WithWayland)
normalize(Examples)

# print configuratoin
message("\tFinal Configuration:")
message(" ")

message("\tAndroid:         " ${Android})
message("\tWithWinapi:      " ${WithWinapi})
message("\tWithWayland:     " ${WithWayland})
message("\tWithX11:         " ${WithX11})
message("\tWithGl:          " ${WithGl})
message("\tWithEgl:         " ${WithEgl})
message("\tWithVulkan:      " ${WithVulkan})
message(" ")

message("\tUsed nytl:       " ${nytl_location})
message("\tExamples:        " ${Examples})

message(" ")
message("\tVersion:       " ${vmajor} "." ${vminor} "." ${vpatch})
message(" ")

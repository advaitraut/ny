# Boost Software License - Version 1.0 - August 17th, 2003
# 
# Copyright (c) 2017 nyorain
# 
# Permission is hereby granted, free of charge, to any person or organization
# obtaining a copy of the software and accompanying documentation covered by
# this license (the "Software") to use, reproduce, display, distribute,
# execute, and transmit the Software, and to prepare derivative works of the
# Software, and to permit third-parties to whom the Software is furnished to
# do so, all subject to the following:
# 
# The copyright notices in the Software and this entire statement, including
# the above license grant, this restriction and the following disclaimer,
# must be included in all copies of the Software, in whole or in part, and
# all derivative works of the Software, unless such copies or derivative
# works are solely in the form of machine-executable object code generated by
# a source language processor.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
# FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# Skia_FOUND
# Skia_INCLUDE_DIRS (many)
# Skia_LIBRARY (only one - works only if build with gn. Otherwise dont even try to cmake it...)

# Note that skia has usually additionaly dependencies such as fontconfig or freetype

set(Skia_BUILD_DIR "" CACHE PATH "Skia build directory")

find_path(Skia_INCLUDE_DIR_base
	NAMES
		core/SkCanvas.h
	PATH_SUFFIXES
		skia
		chromium/skia
	HINTS 
		${Skia_BUILD_DIR}/include)

find_library(Skia_LIBRARY skia PATH 
		${Skia_BUILD_DIR}/out
		${Skia_BUILD_DIR}/out/Debug
		${Skia_BUILD_DIR}/out/Release
		${Skia_BUILD_DIR}/out/Static
		${Skia_BUILD_DIR}/out/Shared)

mark_as_advanced(Skia_INCLUDE_DIR_base Skia_LIBRARY)

set(skia-include core effects gpu images pathops private svg utils views xml ports codec config)
foreach(inc ${skia-include})
	set(Skia_INCLUDE_DIR_${inc} "${Skia_INCLUDE_DIR_base}/${inc}")
	list(APPEND Skia_INCLUDE_DIRS ${Skia_INCLUDE_DIR_${inc}})
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Skia DEFAULT_MSG Skia_INCLUDE_DIR_base Skia_LIBRARY)
set(Skia_LIBRARIES ${Skia_LIBRARY})

cmake_minimum_required(VERSION 3.20)

project(BENDY LANGUAGES CXX OBJCXX)

if(NOT APPLE)
	message(FATAL_ERROR "This CMake project currently targets macOS only. Use bendy_redo.sln for Windows.")
endif()

if(NOT CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS target architecture" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)

set(BENDY_SOURCES
	src/Parameter.cpp
	src/Plugin.cpp
	src/PluginGUI.cpp
	src/Preset.cpp
	src/Scale.cpp
	src/Serializable.cpp
	src/State.cpp
	src/StreamWrapper.cpp
	src/vstcontrols_ext.cpp
	src/fl/fp_cplug.cpp
	src/fl/fp_plugclass.cpp
	src/vstgui/plugguieditor.cpp
	src/vstgui/vstcontrols.cpp
	src/vstgui/cvstguitimer.cpp
	src/vstgui/vstgui.cpp
	src/vstgui/cocoasupport.mm
)

add_library(BENDY SHARED ${BENDY_SOURCES})

target_include_directories(BENDY PRIVATE
	src
	src/fl
	src/vstgui
)

target_compile_definitions(BENDY PRIVATE
	MAC=1
	MAC_COCOA=1
	VSTGUI_USES_UTF8=1
	VSTGUI_USES_COREGRAPHICS=1
)

target_compile_options(BENDY PRIVATE
	-Wno-deprecated-declarations
	-Wno-unknown-pragmas
	-Wno-macro-redefined
	-Wno-ignored-attributes
	-Wno-inconsistent-missing-override
)

set_source_files_properties(src/vstgui/cocoasupport.mm PROPERTIES
	COMPILE_FLAGS "-fno-objc-arc"
)

target_link_libraries(BENDY PRIVATE
	"-framework Cocoa"
	"-framework CoreFoundation"
	"-framework CoreGraphics"
	"-framework CoreServices"
	"-framework CoreText"
	"-framework ApplicationServices"
	"-framework ImageIO"
)

set_target_properties(BENDY PROPERTIES
	OUTPUT_NAME "BENDY_x64"
	PREFIX ""
	SUFFIX ".dylib"
	BUILD_RPATH ""
)

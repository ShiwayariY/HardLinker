cmake_minimum_required(VERSION 3.14)
project(HardLinker)

add_compile_options(-Wall -Wextra -pedantic)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
cmake_policy(SET CMP0100 NEW)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(NOT DEFINED PLATFORM)
	set(PLATFORM "." CACHE STRING "current platform")
endif()
set(BUILD_SFX "${PLATFORM}/${CMAKE_BUILD_TYPE}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "CMake install prefix" FORCE)
endif()

include_directories(
	include
)

add_executable(HardLinker WIN32
	src/main.cc
)

target_link_libraries(HardLinker
	Qt::Widgets
)

install(TARGETS
	HardLinker
DESTINATION "bin/${BUILD_SFX}")

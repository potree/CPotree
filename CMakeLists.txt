cmake_minimum_required(VERSION 3.16)
set(CMAKE_SUPPRESS_REGENERATION true)

project(CPotree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

#message(${PROJECT_SOURCE_DIR})

file(GLOB HEADER_FILES
    "./include/*.h"
)

file(GLOB CPP_FILES
    "./src/*.cpp"
)

add_executable(CPotree 
	${HEADER_FILES}
	${CPP_FILES}
	./modules/unsuck/unsuck.hpp
	./modules/unsuck/unsuck_platform_specific.cpp
)

set(LASZIP_DIR "${PROJECT_SOURCE_DIR}/libs/laszip")
add_subdirectory(${LASZIP_DIR})
target_link_libraries(CPotree laszip)

set(BROTLI_DIR "${PROJECT_SOURCE_DIR}/libs/brotli")
add_subdirectory(${BROTLI_DIR})
target_link_libraries(CPotree brotlienc-static)
target_link_libraries(CPotree brotlidec-static)


target_include_directories(CPotree PRIVATE "./include")
target_include_directories(CPotree PRIVATE "./modules")
target_include_directories(CPotree PRIVATE "./libs")


if (UNIX)
	find_package(Threads REQUIRED)
	find_package(TBB REQUIRED)
	
	target_link_libraries(${PROJECT_NAME} Threads::Threads)
	target_link_libraries(${PROJECT_NAME} tbb)


	#SET(CMAKE_CXX_FLAGS "-pthread -ltbb")
endif (UNIX)

###############################################
# COPY LICENSE FILES TO BINARY DIRECTORY
###############################################

# laszip
add_custom_command(
	TARGET CPotree POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${LASZIP_DIR}/COPYING
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/license_laszip.txt)

# brotli
add_custom_command(
	TARGET CPotree POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${BROTLI_DIR}/LICENSE
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/license_brotli.txt)

# json
add_custom_command(
	TARGET CPotree POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${PROJECT_SOURCE_DIR}/libs/json/LICENSE
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/license_json.txt)

# glm
add_custom_command(
	TARGET CPotree POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${PROJECT_SOURCE_DIR}/libs/glm/copying.txt
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/license_glm.txt)

# CPotree
#add_custom_command(
#	TARGET CPotree POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy
#		${PROJECT_SOURCE_DIR}/LICENSE
#		$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/license_potree_converter.txt)
#
#add_custom_command(
#	TARGET CPotree POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy
#		${PROJECT_SOURCE_DIR}/README.md
#		$<TARGET_FILE_DIR:${PROJECT_NAME}>/README.md)


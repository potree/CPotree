cmake_minimum_required(VERSION 3.16)
set(CMAKE_SUPPRESS_REGENERATION true)

option(WITH_AWS_SDK "Build with AWS SDK" OFF)

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
list(FILTER CPP_FILES EXCLUDE REGEX ".*executable_.*.cpp$")


set(LASZIP_DIR "${PROJECT_SOURCE_DIR}/libs/laszip")
set(BROTLI_DIR "${PROJECT_SOURCE_DIR}/libs/brotli")

add_subdirectory(${LASZIP_DIR})
add_subdirectory(${BROTLI_DIR})

#######################
# Initialize AWS SDK
#######################

if (WITH_AWS_SDK)
	add_definitions(-DWITH_AWS_SDK)
	find_package(AWSSDK REQUIRED COMPONENTS s3)
endif (WITH_AWS_SDK)

###############################################
# COPY LICENSE FILES TO BINARY DIRECTORY
###############################################

function(buildLicenses targetname)

	# laszip
	add_custom_command(
		TARGET ${targetname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${LASZIP_DIR}/COPYING
			$<TARGET_FILE_DIR:${targetname}>/licenses/license_laszip.txt)

	# brotli
	add_custom_command(
		TARGET ${targetname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${BROTLI_DIR}/LICENSE
			$<TARGET_FILE_DIR:${targetname}>/licenses/license_brotli.txt)

	# json
	add_custom_command(
		TARGET ${targetname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${PROJECT_SOURCE_DIR}/libs/json/LICENSE
			$<TARGET_FILE_DIR:${targetname}>/licenses/license_json.txt)

	# glm
	add_custom_command(
		TARGET ${targetname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${PROJECT_SOURCE_DIR}/libs/glm/copying.txt
			$<TARGET_FILE_DIR:${targetname}>/licenses/license_glm.txt)

endfunction()


###############################################
# extract_profile
###############################################


add_executable(extract_profile 
	${HEADER_FILES}
	${CPP_FILES}
	./modules/unsuck/unsuck.hpp
	./modules/unsuck/unsuck_platform_specific.cpp
	./src/executable_extract_profile.cpp
)


if (WITH_AWS_SDK)
target_link_libraries(extract_profile ${AWSSDK_LINK_LIBRARIES} ${AWSSDK_PLATFORM_DEPS})
endif (WITH_AWS_SDK)

target_link_libraries(extract_profile laszip)
target_link_libraries(extract_profile brotlienc-static)
target_link_libraries(extract_profile brotlidec-static)

target_include_directories(extract_profile PRIVATE "./include")
target_include_directories(extract_profile PRIVATE "./modules")
target_include_directories(extract_profile PRIVATE "./libs")

if (UNIX)
	find_package(Threads REQUIRED)
#	find_package(TBB REQUIRED)
	
	target_link_libraries(extract_profile Threads::Threads)
#	target_link_libraries(extract_profile tbb)
endif (UNIX)

buildLicenses(extract_profile)


###############################################
# extract_area
###############################################


add_executable(extract_area 
	${HEADER_FILES}
	${CPP_FILES}
	./modules/unsuck/unsuck.hpp
	./modules/unsuck/unsuck_platform_specific.cpp
	./src/executable_extract_area.cpp
)

if (WITH_AWS_SDK)
target_link_libraries(extract_area ${AWSSDK_LINK_LIBRARIES} ${AWSSDK_PLATFORM_DEPS})
endif (WITH_AWS_SDK)

target_link_libraries(extract_area laszip)
target_link_libraries(extract_area brotlienc-static)
target_link_libraries(extract_area brotlidec-static)

target_include_directories(extract_area PRIVATE "./include")
target_include_directories(extract_area PRIVATE "./modules")
target_include_directories(extract_area PRIVATE "./libs")

if (UNIX)
	find_package(Threads REQUIRED)
#	find_package(TBB REQUIRED)
	
	target_link_libraries(extract_area Threads::Threads)
#	target_link_libraries(extract_area tbb)
endif (UNIX)

buildLicenses(extract_area)








cmake_minimum_required(VERSION 2.8)

project(CPFS)

add_library("${PROJECT_NAME}" "${PROJECT_SOURCE_DIR}/src/cpfs.c")

add_executable("ls" "${PROJECT_SOURCE_DIR}/src/ls.c")
target_link_libraries("ls" ${PROJECT_NAME})

add_executable("cpfs-test" "${PROJECT_SOURCE_DIR}/src/test.c")
target_link_libraries("cpfs-test" ${PROJECT_NAME})

if (MINGW)
	# MinGW thinks that there's something wrong with PRIu64.
	set(EXTRA_CFLAGS "-Wno-format")
endif()

set_target_properties("${PROJECT_NAME}" "ls" "cpfs-test" PROPERTIES
	COMPILE_FLAGS "-std=c99 -Wall -Wextra -Wpedantic"
	RUNTIME_OUTPUT_DIRECTORY "bin"
	ARCHIVE_OUTPUT_DIRECTORY "lib")

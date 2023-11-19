cmake_minimum_required(VERSION 3.10)
project(self-tftp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")

file(GLOB SOURCES "source/*.cpp")

add_executable(self-tftp ${SOURCES})

target_include_directories(self-tftp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

if(WIN32)
	target_link_libraries(self-tftp PRIVATE wsock32 ws2_32)
endif()

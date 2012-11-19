cmake_minimum_required(VERSION 2.6)
project(Utilities)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif()

set(util_sources Array.cpp Cryptography.cpp DataStream.cpp Misc.cpp
	Semaphore.cpp Socket.cpp TCPClient.cpp TCPServer.cpp Time.cpp)

file(GLOB util_headers *.h)

add_library(Utilities SHARED ${util_sources})

find_package(Threads)

target_link_libraries(Utilities ${CMAKE_THREADS_LIBS_INIT})
if(NOT WIN32)
	target_link_libraries(Utilities ssl)
	install(FILES ${util_headers} DESTINATION include/Utilities)
	install(FILES ${CMAKE_BINARY_DIR}/libUtilities.so DESTINATION lib)
endif()

cmake_minimum_required(VERSION 2.6)
project(Utilities)

set(util_sources Array.cpp Cryptography.cpp DataStream.cpp Misc.cpp
	Semaphore.cpp Socket.cpp TCPClient.cpp TCPServer.cpp Time.cpp)

file(GLOB util_headers *.h)

add_library(Utilities SHARED ${util_sources})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif()

find_package(Threads REQUIRED)

target_link_libraries(Utilities ${CMAKE_THREADS_LIBS_INIT})

if(NOT WIN32)
	find_package(OpenSSL REQUIRED)
	target_link_libraries(Utilities ${OPENSSL_LIBRARIES})
	install(FILES ${util_headers} DESTINATION include/Utilities)
	install(FILES ${CMAKE_BINARY_DIR}/libUtilities.so DESTINATION lib)
endif()

project(net)

if(MSVC)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wl,--no-as-needed -std=c++11 -pthread")
endif()

add_subdirectory(src)
add_subdirectory(test)
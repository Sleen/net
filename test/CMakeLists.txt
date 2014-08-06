project(net_test)

include_directories(${net_SOURCE_DIR}/src)
set(SRC test.cpp)

add_executable(net_test ${SRC})

link_directories(${net_SOURCE_DIR}/bin)
target_link_libraries(net_test net)

if(MINGW)
	target_link_libraries(net_test ws2_32)
endif()

set(EXECUTABLE_OUTPUT_PATH ${net_SOURCE_DIR}/bin)
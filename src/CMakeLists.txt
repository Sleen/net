project(net_lib)

set(SRC Socket.cpp)

add_library(net ${SRC})

set(LIBRARY_OUTPUT_PATH ${net_SOURCE_DIR}/bin)
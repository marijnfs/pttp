# CMakeLists files in this project can
# refer to the root source directory of the project as ${MCDNN_SOURCE_DIR} and

cmake_minimum_required (VERSION 2.8.11)
project (PTTP)
#SET(CMAKE_BUILD_TYPE CACHE RELEASE STRING "Build Type")
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cap")

find_package(CapnProto REQUIRED)
include_directories(${CAPNP_INCLUDE_DIRS})
add_definitions(${CAPNP_DEFINITIONS})

file(GLOB CAPNP_FILES cap/*.capnp)
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${CAPNP_FILES})
#include_directories($CMAKE_CURRENT_LIST_DIR)
include_directories(${CMAKE_CURRENT_LIST_DIR}/cap)
#MESSAGE(${CMAKE_CURRENT_LIST_DIR}/cap)
#add_executable(a a.cc ${CAPNP_SRCS} ${CAPNP_HDRS})
#target_link_library(a ${CAPNP_LIBRARIES})

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif()


#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_FLAGS "-std=c++14 -g -fpermissive")

include_directories(
    inc
)

file(GLOB CPP_SOURCE src/*.cc)
file(GLOB CAPNP_SRCS_2 ${CMAKE_CURRENT_SOURCE_DIR}/cap/*.c++)
file(GLOB CAPNP_HDRS_2 ${CMAKE_CURRENT_SOURCE_DIR}/cap/*.h)

#file(GLOB CU_SOURCE src/*.cu)
file(GLOB CPP_HEADERS inc/*.h)

add_executable(node ${CPP_HEADERS} ${CPP_SOURCE} ${CAPNP_SRCS_2} ${CAPNP_HDRS_2} bin/node.cc)
target_link_libraries(node ${CAPNP_LIBRARIES} -lzmq -lsodium -lrocksdb -lpthread)

add_executable(p2p ${CPP_HEADERS} ${CPP_SOURCE} ${CAPNP_SRCS_2} ${CAPNP_HDRS_2} bin/p2p.cc)
target_link_libraries(p2p ${CAPNP_LIBRARIES} -lzmq -lsodium -lrocksdb -lpthread)

#add_executable(nodetest ${CPP_HEADERS} ${CPP_SOURCE} ${CAPNP_SRCS} ${CAPNP_HDRS} bin/nodetest.cc)
#target_link_libraries(nodetest ${CAPNP_LIBRARIES} -lzmq -lsodium)


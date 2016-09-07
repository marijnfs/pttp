# CMakeLists files in this project can
# refer to the root source directory of the project as ${MCDNN_SOURCE_DIR} and

cmake_minimum_required (VERSION 2.8.11)
project (PTTP)
#SET(CMAKE_BUILD_TYPE CACHE RELEASE STRING "Build Type")

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif()

find_package(Protobuf QUIET REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS inc/msg.proto)

#list(APPEND CUDA_NVCC_FLAGS "-arch=sm_30;-std=c++11;-O2;-DVERBOSE")

set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(
    inc
)

file(GLOB CPP_SOURCE src/*.cc)
#file(GLOB CU_SOURCE src/*.cu)
file(GLOB CPP_HEADERS inc/*.h)


add_executable(node ${CPP_HEADERS} ${CPP_SOURCE} bin/node.cc)
target_link_libraries(node ${PROTOBUF_LIBRARIES} -lzmq -lsodium)

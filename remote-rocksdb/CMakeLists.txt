cmake_minimum_required(VERSION 3.10)
project(remoterocksdb CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_C_FLAGS_DEBUG " -std=c99 -g -ggdb -O0 -Wall -Wno-unused-function -fpic -fPIC -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG " -std=c++17 -g -ggdb -O0 -Wall -Wno-unused-function -fpic -fPIC -D_DEBUG")

set(CMAKE_C_FLAGS_RELEASE " -std=c99 -O3 -Wall -Wno-unused-function -fpic -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE " -std=c++17 -O3 -Wall -Wno-unused-function -fpic -fPIC")

# find Protobuf installation
# looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

# find gRPC installation
# looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# project include directory
set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
message(STATUS "PROJECT_INCLUDE_DIR: ${PROJECT_INCLUDE_DIR}")

# rocksdb static library directory
set(ROCKSDB_STATIC_LIB "${CMAKE_SOURCE_DIR}/lib/librocksdb.a")
message(STATUS "ROCKSDB_STATIC_LIB: ${ROCKSDB_STATIC_LIB}")

include_directories(
    ${GRPC_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
    )

aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_LIST)
aux_source_directory(${PROJECT_INCLUDE_DIR}/${PROJECT_NAME} PROTOBUF_SRC_LIST)

add_executable(db_server
    src/db_server.cc
    ${PROTOBUF_SRC_LIST}
    )
add_executable(db_client
    src/db_client.cc
    ${PROTOBUF_SRC_LIST}
    )

target_link_libraries(db_server
    pthread
    absl::flags
    absl::flags_parse
    protobuf::libprotobuf
    gRPC::grpc++_reflection
    gRPC::grpc++
    ${ROCKSDB_STATIC_LIB}
    )
target_link_libraries(db_client
    pthread
    absl::flags
    absl::flags_parse
    protobuf::libprotobuf
    gRPC::grpc++_reflection
    gRPC::grpc++
    ${ROCKSDB_STATIC_LIB}
    )
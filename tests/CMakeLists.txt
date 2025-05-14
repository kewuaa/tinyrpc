find_package(protobuf REQUIRED)

CPMAddPackage("gh:msgpack/msgpack-c#cpp-7.0.0")

add_executable(test_rpc_server)
target_sources(test_rpc_server PRIVATE rpc_server.cpp)
target_link_libraries(
    test_rpc_server
    PRIVATE
        ${PROJECT_NAME}::server
        msgpack-cxx
        protobuf
)

add_executable(test_rpc_client)
target_sources(test_rpc_client PRIVATE rpc_client.cpp)
target_link_libraries(
    test_rpc_client
    PRIVATE
        ${PROJECT_NAME}::client
        msgpack-cxx
        protobuf
)

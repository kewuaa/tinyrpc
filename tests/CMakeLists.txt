include(FindProtobuf)

protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS test_rpc.proto)

add_executable(test_rpc_server)
target_sources(
    test_rpc_server
    PRIVATE
        rpc_server.cpp
        ${PROTO_SRCS}
        ${PROTO_HEADERS}
)
target_link_libraries(
    test_rpc_server
    PRIVATE
        ${PROJECT_NAME}::server
)

add_executable(test_rpc_client)
target_sources(
    test_rpc_client
    PRIVATE
        rpc_client.cpp
        ${PROTO_SRCS}
        ${PROTO_HEADERS}
)
target_link_libraries(
    test_rpc_client
    PRIVATE
        ${PROJECT_NAME}::client
)

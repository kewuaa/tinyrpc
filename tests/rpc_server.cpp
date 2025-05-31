#include <asyncio.hpp>
#include "tinyrpc.hpp"
#include "tests/test_rpc.pb.h"


int add(int a, int b) {
    return a+b;
}


int get_value() {
    return 999;
}


void hello() {
    std::cout << "Hello World" << std::endl;
}


void hello_to(const std::string& name) {
    std::cout << "Hello " << name << std::endl;
}


void test_proto(const test_rpc::Msg& msg) {
    std::cout << msg.query() << std::endl;
}


test_rpc::Msg return_proto() {
    test_rpc::Msg msg;
    msg.set_query("query body");
    msg.set_page_number(1000);
    return msg;
}


ASYNCIO_NS::Task<> test_async() {
    std::cout << "sleep 1000" << std::endl;
    co_await ASYNCIO_NS::sleep<1000>();
    std::cout << "sleep done" << std::endl;
}


ASYNCIO_NS::Task<int> test_async_return() {
    std::cout << "sleep 1000" << std::endl;
    co_await ASYNCIO_NS::sleep<1000>();
    co_return 999;
}


ASYNCIO_NS::Task<> async_hello_to(std::string name) {
    std::cout << "sleep 1000" << std::endl;
    co_await ASYNCIO_NS::sleep<1000>();
    std::cout << "hello " << name << std::endl;
}


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    TINYRPC_NS::Server server;
    server.init("127.0.0.1", 12345, 1024);
    TINYRPC_NS::register_func(server, "add", add);
    TINYRPC_NS::register_func(server, "get_value", get_value);
    TINYRPC_NS::register_func(server, "hello", hello);
    TINYRPC_NS::register_func(server, "hello_to", hello_to);
    TINYRPC_NS::register_func(server, "test_proto", test_proto);
    TINYRPC_NS::register_func(server, "return_proto", return_proto);
    TINYRPC_NS::register_func(server, "test_async", test_async);
    TINYRPC_NS::register_func(server, "test_async_return", test_async_return);
    TINYRPC_NS::register_func(server, "async_hello_to", async_hello_to);
    ASYNCIO_NS::run(server.run());
}

#include <asyncio.hpp>
#include "tinyrpc.hpp"


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


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    auto& server = TINYRPC_NS::Server::get();
    server.init("127.0.0.1", 12345, 1024);
    TINYRPC_NS::register_func("add", add);
    TINYRPC_NS::register_func("get_value", get_value);
    TINYRPC_NS::register_func("hello", hello);
    TINYRPC_NS::register_func("hello_to", hello_to);
    ASYNCIO_NS::run(server.run());
}

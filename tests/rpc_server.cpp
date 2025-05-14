#include <asyncio.hpp>
#include "tinyrpc.hpp"


int add(int a, int b) {
    return a+b;
}


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    auto& server = TINYRPC_NS::Server::get();
    server.init("127.0.0.1", 12345, 1024);
    TINYRPC_NS::register_func("add", add);
    kwa::asyncio::run(server.run());
}

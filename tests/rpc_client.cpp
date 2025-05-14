#include <iostream>

# include <asyncio.hpp>

#include "tinyrpc.hpp"


kwa::asyncio::Task<> add() {
    TINYRPC_NS::Client c;
    co_await c.connect("127.0.0.1", 12345);
    int res;
    co_await kwa::tinyrpc::call_func(c, "add", res, 1, 3);
    std::cout << "1 + 3 = " << res << std::endl;
}


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    kwa::asyncio::run(add());
}

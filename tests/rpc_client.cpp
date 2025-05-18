#include <iostream>

# include <asyncio.hpp>

#include "tinyrpc.hpp"


kwa::asyncio::Task<> add() {
    TINYRPC_NS::Client c;
    co_await c.connect("127.0.0.1", 12345);
    int res1 = co_await kwa::tinyrpc::call_func<int>(c, "add", 1, 3);
    std::cout << "1 + 3 = " << res1 << std::endl;
    auto value = co_await kwa::tinyrpc::call_func<int>(c, "get_value");
    std::cout << value << std::endl;
    co_await kwa::tinyrpc::call_func<void>(c, "hello");
    std::string name = "kewuaa";
    co_await kwa::tinyrpc::call_func<void>(c, "hello_to", name);
}


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    kwa::asyncio::run(add());
}

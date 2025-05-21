#include <iostream>

# include <asyncio.hpp>

#include "tests/test_rpc.pb.h"
#include "tinyrpc.hpp"


ASYNCIO_NS::Task<> add() {
    TINYRPC_NS::Client c;
    co_await c.connect("127.0.0.1", 12345);
    int res1 = co_await TINYRPC_NS::call_func<int>(c, "add", 1, 3);
    std::cout << "1 + 3 = " << res1 << std::endl;
    auto value = co_await TINYRPC_NS::call_func<int>(c, "get_value");
    std::cout << value << std::endl;
    co_await TINYRPC_NS::call_func<void>(c, "hello");
    std::string name = "kewuaa";
    co_await TINYRPC_NS::call_func<void>(c, "hello_to", name);
    test_rpc::Msg msg;
    msg.set_query("query body");
    msg.set_page_number(999);
    co_await TINYRPC_NS::call_func<void>(c, "test_proto", msg);
    msg = co_await TINYRPC_NS::call_func<test_rpc::Msg>(c, "return_proto");
    std::cout << msg.page_number() << std::endl;
}


int main() {
#if _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    ASYNCIO_NS::run(add());
}

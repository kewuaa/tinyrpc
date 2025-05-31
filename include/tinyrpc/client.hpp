#pragma once
#include <asyncio.hpp>

#include "tinyrpc_export.hpp"
#include "../tinyrpc_ns.hpp"
#include "./message.hpp"


TINYRPC_NS_BEGIN()

class TINYRPC_EXPORT Client {
public:
    Client() noexcept;
    Client(Client&) = delete;
    Client(Client&&) noexcept;
    ~Client() noexcept;
    Client& operator=(Client&) = delete;
    Client& operator=(Client&&) noexcept;
    asyncio::Task<bool> connect(const char* host, short port) noexcept;
    asyncio::Task<Message, std::nullptr_t> call(std::string_view name, std::string_view data) noexcept;
private:
    struct impl;
    impl* _pimpl;
};

TINYRPC_NS_END

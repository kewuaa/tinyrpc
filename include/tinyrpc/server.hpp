#pragma once
#include <asyncio.hpp>
#include <growable_buffer.hpp>

#include "tinyrpc_export.hpp"
#include "./message.hpp"
#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN()

using Function = std::function<void(Message&&, GrowableBuffer&)>;
using AFunction = std::function<ASYNCIO_NS::Task<>(Message&&, GrowableBuffer&)>;

class TINYRPC_EXPORT Server {
public:
    Server() noexcept;
    Server(Server&) = delete;
    Server(Server&&) noexcept;
    Server& operator=(Server&) = delete;
    Server& operator=(Server&&) noexcept;
    void init(const char* host, short port, int max_listen_num) noexcept;
    asyncio::Task<> run() noexcept;
    void register_func(const std::string& name, Function&& func) noexcept;
    void register_afunc(const std::string& name, AFunction&& afunc) noexcept;
private:
    struct impl;
    impl* _pimpl;
};

TINYRPC_NS_END

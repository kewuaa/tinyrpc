#pragma once
#include <asyncio.hpp>
#include <growable_buffer.hpp>

#include "tinyrpc_export.hpp"
#include "./message.hpp"
#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN()

using Function = std::function<void(Message&&, GrowableBuffer&)>;

class TINYRPC_EXPORT Server {
public:
    Server(Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&) = delete;
    Server& operator=(Server&&) = delete;
    static Server& get() noexcept;
    void init(const char* host, short port, int max_listen_num) noexcept;
    asyncio::Task<> run() noexcept;
    void register_func(const std::string& name, Function&& func) noexcept;
private:
    struct impl;
    std::unique_ptr<impl> _pimpl;
    Server() noexcept;
};

TINYRPC_NS_END

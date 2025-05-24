#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <growable_buffer.hpp>
#include <tiny_thread_pool.hpp>

#include "tinyrpc/server.hpp"
#include "tinyrpc/message/parser.hpp"


TINYRPC_NS_BEGIN()

struct Server::impl {
    std::mutex mtx;
    TinyThreadPool pool { TINYRPC_THREAD_POOL_SIZE, std::chrono::seconds(60) };
    asyncio::Socket sock {};
    std::unordered_map<std::string, Function> funcs {};

    inline void register_func(const std::string& name, Function&& func) noexcept {
        if (funcs.contains(name)) {
            SPDLOG_INFO("update function {}", name);
        } else {
            SPDLOG_INFO("register function {}", name);
        }
        funcs[name] = std::move(func);
    }

    asyncio::Task<> write_forever(
        asyncio::Socket& sock,
        asyncio::Event<bool>& ev,
        GrowableBuffer& buffer
    ) noexcept {
        SPDLOG_INFO("start write task for fd {}", sock.fd());
        char temp_buffer[TINYRPC_DEFAULT_BUFFER_SIZE];
        while (true) {
            if (buffer.readable_bytes() == 0) {
                auto stop = co_await ev.wait();
                if (stop && *stop) {
                    break;
                }
            }
            auto nbytes = std::min(buffer.readable_bytes(), TINYRPC_DEFAULT_BUFFER_SIZE);
            auto view = buffer.read(nbytes);
            std::copy(view.begin(), view.end(), temp_buffer);
            auto res = co_await sock.write(temp_buffer, nbytes);
            if (!res) {
                SPDLOG_ERROR("error while write to fd {}: {}", sock.fd(), res.error());
                continue;
            }
            SPDLOG_DEBUG(
                "write {} bytes data to fd {}:{}",
                nbytes,
                sock.fd(),
                spdlog::to_hex(std::span(temp_buffer, nbytes))
            );
        }
        SPDLOG_INFO("stop write task for fd {}", sock.fd());
    }

    asyncio::Task<> handle_message(Message msg, GrowableBuffer& write_buffer, asyncio::Event<bool>& ev) noexcept {
        auto func = funcs.find(msg.func_name());
        if (func != funcs.end()) {
            // func->second(mtx, std::move(msg), write_buffer);
            auto& loop = asyncio::EventLoop::get();
            auto fut = loop.run_in_thread(pool, [this, msg = std::move(msg), &f = func->second, &write_buffer] mutable {
                f(mtx, std::move(msg), write_buffer);
            });
            co_await *fut;
        } else {
            msg.body_size() = 0;
            write_buffer.write(msg.header());
        }
        if (!ev.is_set()) {
            ev.set();
        }
    }

    asyncio::Task<> handle_connection(int fd) noexcept {
        char buffer[TINYRPC_DEFAULT_BUFFER_SIZE];
        asyncio::Socket sock(fd);
        GrowableBuffer write_buffer;
        message::Parser message_parser;
        asyncio::Event<bool> ev;
        write_forever(sock, ev, write_buffer);
        while (true) {
            auto res = co_await sock.read(buffer, TINYRPC_DEFAULT_BUFFER_SIZE);
            if (!res) {
                break;
            }
            auto nbytes = *res;
            SPDLOG_DEBUG(
                "recv {} bytes data:{}",
                nbytes,
                spdlog::to_hex(std::span(buffer, (size_t)nbytes))
            );
            auto msgs = message_parser.process(buffer, nbytes);
            for (auto& msg : msgs) {
                handle_message(std::move(msg), write_buffer, ev);
            }
        }
    }

    void init(const char* host, short port, int max_listen_num) noexcept {
        auto res = sock.bind(host, port);
        if (res == -1) {
            std::perror("failed to bind");
            exit(EXIT_FAILURE);
        }
        SPDLOG_INFO("successfully bind to {}:{}", host, port);
        res = sock.listen(max_listen_num);
        if (res == -1) {
            std::perror("failed to listen");
            exit(EXIT_FAILURE);
        }
        SPDLOG_INFO("start listenning, listen number: {}", max_listen_num);
    }

    asyncio::Task<> run() noexcept {
        while (true) {
            auto conn = co_await sock.accept();
            handle_connection(conn);
        }
    }
};


Server::Server() noexcept: _pimpl(std::make_unique<impl>()) {}

Server& Server::get() noexcept {
    static Server server;
    return server;
}

void Server::register_func(const std::string& name, Function&& func) noexcept {
    _pimpl->register_func(name, std::move(func));
}

void Server::init(const char* host, short port, int max_listen_num) noexcept {
    return _pimpl->init(host, port, max_listen_num);
}

asyncio::Task<> Server::run() noexcept {
    return _pimpl->run();
}

TINYRPC_NS_END

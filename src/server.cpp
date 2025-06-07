#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <growable_buffer.hpp>

#include "tinyrpc/utils.hpp"
#include "tinyrpc/server.hpp"
#include "tinyrpc/message/parser.hpp"


TINYRPC_NS_BEGIN()

struct Server::impl {
    asyncio::Socket sock {};
    std::unordered_map<std::string, Function> funcs {};
    std::unordered_map<std::string, AFunction> afuncs {};

    inline void register_func(const std::string& name, Function&& func) noexcept {
        if (funcs.contains(name)) {
            SPDLOG_INFO("update function {}", name);
        } else {
            SPDLOG_INFO("register function {}", name);
        }
        funcs[name] = std::move(func);
    }

    inline void register_afunc(const std::string& name, AFunction&& afunc) noexcept {
        if (afuncs.contains(name)) {
            SPDLOG_INFO("update async function {}", name);
        } else {
            SPDLOG_INFO("register async function {}", name);
        }
        afuncs[name] = std::move(afunc);
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
        auto view = write_buffer.malloc(msg.header().size());
        auto size = write_buffer.readable_bytes();
        bool func_found = true;
        std::string func_name { msg.func_name() };
        if (funcs.contains(func_name)) {
            funcs[func_name](std::move(msg), write_buffer);
        } else if (afuncs.contains(func_name)) {
            co_await afuncs[func_name](std::move(msg), write_buffer);
        } else {
            func_found = false;
            SPDLOG_INFO("function {} not registered yet", func_name);
        }
        if (func_found) {
            msg.body_size() = write_buffer.readable_bytes() - size;
            std::copy(msg.header().begin(), msg.header().end(), view.data());
        } else {
            auto id = msg.id();
            write_buffer.backup(view.size());
            write_buffer.write({ (const char*)&VERIFY_FLAG, sizeof(VERIFY_FLAG) });
            write_buffer.write({ (const char*)&id, sizeof(id) });
            write_buffer.write('\0');
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


Server::Server() noexcept: _pimpl(new impl()) {}

Server::Server(Server&& server) noexcept: _pimpl(std::exchange(server._pimpl, nullptr)) {}

Server& Server::operator=(Server&& server) noexcept {
    utils::free_and_null(_pimpl);
    _pimpl = std::exchange(server._pimpl, nullptr);
    return *this;
}

void Server::register_func(const std::string& name, Function&& func) noexcept {
    _pimpl->register_func(name, std::move(func));
}

void Server::register_afunc(const std::string& name, AFunction&& afunc) noexcept {
    _pimpl->register_afunc(name, std::move(afunc));
}

void Server::init(const char* host, short port, int max_listen_num) noexcept {
    return _pimpl->init(host, port, max_listen_num);
}

asyncio::Task<> Server::run() noexcept {
    return _pimpl->run();
}

TINYRPC_NS_END

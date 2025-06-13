#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <growable_buffer.hpp>

#include "tinyrpc/client.hpp"
#include "tinyrpc/message/parser.hpp"
#include "tinyrpc/utils.hpp"


TINYRPC_NS_BEGIN()

struct Client::impl {
    asyncio::Socket sock;
    asyncio::Event<> ev;
    GrowableBuffer write_buffer;
    std::unordered_map<Message::ID, asyncio::Event<Message>> waits;
    std::optional<asyncio::Task<>> read_task { std::nullopt };
    std::optional<asyncio::Task<>> write_task { std::nullopt };

    static inline Message::ID generate_message_id() noexcept {
        static Message::ID id = 0;
        return ++id;
    }

    ~impl() noexcept {
        if (read_task) {
            read_task->cancel();
        }
        if (write_task) {
            write_task->cancel();
        }
    }

    asyncio::Task<bool> connect(const char* host, short port) noexcept {
        auto res = co_await sock.connect(host, port);
        if (res == -1) {
            std::perror(std::format("({})failed to connect to {}:{}", errno, host, port).c_str());
            co_return false;
        }
        SPDLOG_INFO("successfully connect to {}:{}", host, port);

        if (read_task) {
            read_task->cancel();
        }
        read_task = read_forever();
        if (write_task) {
            write_task->cancel();
            write_buffer = {};
        }
        write_task = write_forever();
        co_return true;
    }

    void handle_message(Message&& msg) noexcept {
        auto id = msg.id();
        if (waits.contains(id)) {
            SPDLOG_DEBUG("wake up coroutine to consume message {}", id);
            waits[id].set(std::move(msg));
        } else [[unlikely]] {
            SPDLOG_WARN("seems that no coroutine waiting for message {}", id);
        }
    }

    asyncio::Task<> read_forever() noexcept {
        char temp_buffer[TINYRPC_DEFAULT_BUFFER_SIZE];
        message::Parser message_parser;
        while (true) {
            auto res = co_await sock.read(temp_buffer, TINYRPC_DEFAULT_BUFFER_SIZE);
            if (!res) {
                SPDLOG_ERROR("error while read from fd {}: {}", sock.fd(), res.error());
                break;
            }
            auto nbytes = *res;
            SPDLOG_DEBUG(
                "recv {} bytes data:{}",
                nbytes,
                spdlog::to_hex(std::span(temp_buffer, (size_t)nbytes))
            );
            auto msgs = message_parser.process(temp_buffer, *res);
            for (auto& msg : msgs) {
                handle_message(std::move(msg));
            }
        }
        // notify all coroutine that are waiting for message
        for (auto& [_, ev] : waits) {
            if (!ev.is_set()) {
                ev.set();
            }
        }
        SPDLOG_INFO("stop read task for fd", sock.fd());
        read_task.reset();
        write_task->cancel();
        write_task.reset();
        write_buffer = {};
    }

    asyncio::Task<> write_forever() noexcept {
        SPDLOG_INFO("start write task for fd {}", sock.fd());
        char temp_buffer[TINYRPC_DEFAULT_BUFFER_SIZE];
        while (true) {
            if (write_buffer.readable_bytes() == 0) {
                SPDLOG_DEBUG("empty write bufer, start waiting");
                co_await ev.wait();
            }
            auto nbytes = std::min(TINYRPC_DEFAULT_BUFFER_SIZE, write_buffer.readable_bytes());
            auto view = write_buffer.read(nbytes);
            std::copy(view.begin(), view.end(), temp_buffer);
            auto res = co_await sock.write(temp_buffer, nbytes);
            if (!res) {
                SPDLOG_ERROR("error while write to fd {}: {}", sock.fd(), res.error());
                exit(EXIT_FAILURE);
            }
            SPDLOG_DEBUG(
                "write {} bytes data to fd {}:{}",
                nbytes,
                sock.fd(),
                spdlog::to_hex(std::span(temp_buffer, nbytes))
            );
        }
        write_task.reset();
        write_buffer = {};
    }

    Message::ID send_request(std::string_view name, std::string_view body) noexcept {
        auto id = generate_message_id();
        auto header_size = sizeof(VERIFY_FLAG) + sizeof(Message::ID) + name.size()+1 + sizeof(size_t);
        auto body_size = body.size();

        auto header_buffer = write_buffer.malloc(header_size);
        auto out = header_buffer.data();
        out = std::copy((char*)&VERIFY_FLAG, ((char*)&VERIFY_FLAG+sizeof(VERIFY_FLAG)), header_buffer.data());
        out = std::copy((char*)&id, (char*)&id+sizeof(Message::ID), out);
        out = std::copy(name.begin(), name.end(), out);
        *out = '\0'; ++out;
        out = std::copy((char*)&body_size, (char*)&body_size+sizeof(size_t), out);
        assert(out-header_buffer.data() == header_size);

        if (!body.empty()) write_buffer.write(body);

        if (!ev.is_set()) {
            ev.set();
        }
        return id;
    }
};


Client::Client() noexcept: _pimpl(new impl()) {
    //
}

Client::Client(Client&& c) noexcept: _pimpl(std::exchange(c._pimpl, nullptr)) {
    //
}

Client::~Client() noexcept {
    utils::free_and_null(_pimpl);
}

Client& Client::operator=(Client&& c) noexcept {
    utils::free_and_null(_pimpl);
    _pimpl = std::exchange(c._pimpl, nullptr);
    return *this;
}

asyncio::Task<bool> Client::connect(const char* host, short port) noexcept{
    return _pimpl->connect(host, port);
}

asyncio::Task<Message, RPCError> Client::call(std::string_view name, std::string_view data) noexcept {
    if (!_pimpl->write_task) {
        co_return RPCError::ConnectionClosed;
    }
    auto id = _pimpl->send_request(name, data);
    SPDLOG_DEBUG("wait for message {}", id);
    auto [pair, success] = _pimpl->waits.insert({ id, {} });
    assert(success && "id conflict");
    auto& [_, ev] = *pair;
    auto msg = co_await ev.wait();
    _pimpl->waits.erase(id);
    if (!msg) {
        co_return RPCError::ConnectionClosed;
    }
    if (msg->func_name().empty()) {
        co_return RPCError::FunctionNotFound;
    } else {
        co_return std::move(*msg);
    }
}

TINYRPC_NS_END

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "tinyrpc/client.hpp"
#include "tinyrpc/message/parser.hpp"
#include "tinyrpc/utils.hpp"


TINYRPC_NS_BEGIN()

struct Client::impl {
    asyncio::Socket sock;
    std::unordered_map<Message::ID, Message> cache;
    std::unordered_map<Message::ID, asyncio::Event<bool>*> waits;
    std::optional<asyncio::Task<>> read_task { std::nullopt };

    ~impl() noexcept {
        if (read_task) {
            read_task->cancel();
        }
    }

    asyncio::Task<> connect(const char* host, short port) noexcept {
        auto res = co_await sock.connect(host, port);
        if (res == -1) {
            std::perror(std::format("({})failed to connect to {}:{}", errno, host, port).c_str());
            exit(EXIT_FAILURE);
        }
        SPDLOG_INFO("successfully connect to {}:{}", host, port);

        if (read_task) {
            read_task->cancel();
        }
        read_task = read_forever();
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
            message_parser.process(
                temp_buffer,
                *res,
                [this](Message&& msg) {
                    auto id = msg.id();
                    SPDLOG_DEBUG("save message {} to cache", id);
                    cache[id] = std::move(msg);
                    if (waits.contains(id)) {
                        SPDLOG_DEBUG("wake up coroutine to consume message {}", id);
                        waits[id]->set();
                    }
                }
            );
        }
        // notify all coroutine that are waiting for message
        for (auto [_, ev] : waits) {
            if (!ev->is_set()) {
                ev->set(true);
            }
        }
        SPDLOG_INFO("stop read task for fd", sock.fd());
    }

    asyncio::Task<Message::ID, const char*> send_request(std::string_view name, std::string_view body) noexcept {
        auto header_size = sizeof(VERIFY_FLAG) + sizeof(Message::ID) + name.size()+1 + sizeof(size_t);
        auto body_size = body.size();
        std::string header;
        header.reserve(header_size);
        header.append((char*)(&VERIFY_FLAG), sizeof(VERIFY_FLAG));
        header.append((char*)(&++id), sizeof(Message::ID));
        header.append(name); header.push_back('\0');
        header.append((char*)(&body_size), sizeof(size_t));
        auto res = co_await sock.write(header.data(), header.size());
        SPDLOG_DEBUG(
            "send {} bytes header:{}",
            header_size,
            spdlog::to_hex(header)
        );
        if (!res) {
            co_return res.error();
        }
        co_return (co_await sock.write(body.data(), body.size()))
            .transform(
                [&body] {
                    SPDLOG_DEBUG(
                        "send {} bytes header:{}",
                        body.size(),
                        spdlog::to_hex(body)
                    );
                    return id;
                }
            );
    }
};


Client::Client() noexcept: _pimpl(new impl()) {
    //
}

Client::Client(Client&& c) noexcept {
    *this = std::move(c);
}

Client::~Client() noexcept {
    utils::free_and_null(_pimpl);
}

Client& Client::operator=(Client&& c) noexcept {
    utils::free_and_null(_pimpl);
    _pimpl = std::exchange(c._pimpl, nullptr);
    return *this;
}

asyncio::Task<> Client::connect(const char* host, short port) noexcept{
    return _pimpl->connect(host, port);
}

asyncio::Task<Message, const char*> Client::call(std::string_view name, std::string_view data) noexcept {
    auto id = co_await _pimpl->send_request(name, data);
    if (!id) {
        co_return id.error();
    }
    if (!_pimpl->cache.contains(*id)) {
        SPDLOG_DEBUG("wait for message {}", *id);
        asyncio::Event<bool> ev;
        _pimpl->waits[*id] = &ev;
        auto terminated = co_await ev.wait();
        _pimpl->waits.erase(*id);
        if (terminated && *terminated) {
            exit(EXIT_FAILURE);
        }
    }
    auto resp = std::move(_pimpl->cache[*id]);
    _pimpl->cache.erase(*id);
    co_return std::move(resp);
}

TINYRPC_NS_END

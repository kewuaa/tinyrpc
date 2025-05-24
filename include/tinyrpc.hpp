#pragma once
#include <msgpack.hpp>

#include "tinyrpc_ns.hpp"
#include "tinyrpc/utils.hpp"
#include "tinyrpc/server.hpp"
#include "tinyrpc/client.hpp"
#include "tinyrpc/wrapped_buffer.hpp"


TINYRPC_NS_BEGIN()

template<typename F>
void register_func(const std::string& name, F&& func) noexcept {
    using traits = utils::function_traits<std::decay_t<F>>;
    using return_type = traits::return_type;
    using args_type = traits::args_type;
    auto& server = Server::get();
    server.register_func(name, [f = std::forward<F>(func)](std::mutex& mtx, Message&& msg, GrowableBuffer& out) {
        auto buffer = msg.body();
        if constexpr (std::tuple_size_v<args_type> == 0) {
            if constexpr (std::is_void_v<return_type>) {
                f();
                // write to out buffer with lock
                msg.body_size() = 0;
                std::lock_guard<std::mutex> lock { mtx };
                out.write(msg.header());
            } else {
                auto res = f();
                // write to out buffer with lock
                std::lock_guard<std::mutex> lock { mtx };
                auto view = out.malloc(msg.header().size());
                auto size = out.readable_bytes();
                WrappedBuffer buf(out);
                if constexpr (concepts::ProtoType<return_type>) {
                    res.SerializeToZeroCopyStream(&buf);
                } else {
                    msgpack::pack(buf, res);
                }
                msg.body_size() = out.readable_bytes() - size;
                std::copy(msg.header().begin(), msg.header().end(), view.data());
            }
        } else {
            if constexpr (std::is_void_v<return_type>) {
                if constexpr (utils::is_proto_args<args_type>) {
                    std::decay_t<decltype(std::get<0>(*(args_type*)nullptr))> arg;
                    arg.ParseFromArray(buffer.data(), buffer.size());
                    f(std::move(arg));
                } else {
                    args_type args;
                    msgpack::unpack(buffer.data(), buffer.size()).get().convert(args);
                    utils::expand_tuple_call(f, std::move(args));
                }
                // write to out buffer with lock
                msg.body_size() = 0;
                std::lock_guard<std::mutex> lock { mtx };
                out.write(msg.header());
            } else {
                return_type res;
                if constexpr (utils::is_proto_args<args_type>) {
                    std::decay_t<decltype(std::get<0>(*(args_type*)nullptr))> arg;
                    arg.ParseFromArray(buffer.data(), buffer.size());
                    res = f(std::move(arg));
                } else {
                    args_type args;
                    msgpack::unpack(buffer.data(), buffer.size()).get().convert(args);
                    res = utils::expand_tuple_call(f, std::move(args));
                }
                // write to out buffer with lock
                std::lock_guard<std::mutex> lock { mtx };
                auto view = out.malloc(msg.header().size());
                auto size = out.readable_bytes();
                WrappedBuffer buf(out);
                if constexpr (concepts::ProtoType<return_type>) {
                    res.SerializeToZeroCopyStream(&buf);
                } else {
                    msgpack::pack(buf, res);
                }
                msg.body_size() = out.readable_bytes() - size;
                std::copy(msg.header().begin(), msg.header().end(), view.data());
            }
        }
    });
}


template<typename R, typename... Args>
asyncio::Task<R> call_func(Client& client, std::string_view name, Args&&... args) {
    GrowableBuffer data;
    WrappedBuffer buf(data);
    if constexpr (sizeof...(Args) > 0) {
        if constexpr (utils::is_proto_args<Args...>) {
            decltype(auto) arg = utils::get_first_arg(args...);
            arg.SerializeToZeroCopyStream(&buf);
        } else {
            std::tuple<std::decay_t<Args>...> args_ { std::forward<Args>(args)... };
            msgpack::pack(buf, args_);
        }
    }
    auto resp = co_await client.call(name, data.read_all());
    auto body = resp.body();
    if constexpr (!std::is_void_v<R>) {
        R res;
        if constexpr (concepts::ProtoType<R>) {
            res.ParseFromArray(body.data(), body.size());
        } else {
            res = msgpack::unpack(body.data(), body.size())->convert();
        }
        co_return std::move(res);
    }
}

TINYRPC_NS_END

#pragma once
#include <msgpack.hpp>

#include "tinyrpc_ns.hpp"
#include "tinyrpc/utils.hpp"
#include "tinyrpc/server.hpp"
#include "tinyrpc/client.hpp"
#include "tinyrpc/wrapped_buffer.hpp"


TINYRPC_NS_BEGIN()

template<typename F>
void register_func(Server& server, const std::string& name, F&& func) noexcept {
    using traits = utils::function_traits<std::decay_t<F>>;
    using return_type = traits::return_type;
    using args_type = traits::args_type;
    if constexpr (utils::is_async_task_v<return_type>) {
        using return_type = return_type::result_type;
        server.register_afunc(name, [f = std::forward<F>(func)](Message&& msg, GrowableBuffer& out) -> ASYNCIO_NS::Task<> {
            auto buffer = msg.body();
            if constexpr (std::tuple_size_v<args_type> == 0) {
                if constexpr (std::is_void_v<return_type>) {
                    co_await f();
                } else {
                    auto res = co_await f();
                    WrappedBuffer buf(out);
                    if constexpr (concepts::ProtoType<return_type>) {
                        res.SerializeToZeroCopyStream(&buf);
                    } else {
                        msgpack::pack(buf, res);
                    }
                }
            } else {
                if constexpr (std::is_void_v<return_type>) {
                    if constexpr (utils::is_proto_args<args_type>) {
                        std::decay_t<decltype(std::get<0>(*(args_type*)nullptr))> arg;
                        arg.ParseFromArray(buffer.data(), buffer.size());
                        co_await f(std::move(arg));
                    } else {
                        args_type args;
                        msgpack::unpack(buffer.data(), buffer.size()).get().convert(args);
                        co_await utils::expand_tuple_call(f, std::move(args));
                    }
                } else {
                    return_type res;
                    if constexpr (utils::is_proto_args<args_type>) {
                        std::decay_t<decltype(std::get<0>(*(args_type*)nullptr))> arg;
                        arg.ParseFromArray(buffer.data(), buffer.size());
                        res = co_await f(std::move(arg));
                    } else {
                        args_type args;
                        msgpack::unpack(buffer.data(), buffer.size()).get().convert(args);
                        res = co_await utils::expand_tuple_call(f, std::move(args));
                    }
                    WrappedBuffer buf(out);
                    if constexpr (concepts::ProtoType<return_type>) {
                        res.SerializeToZeroCopyStream(&buf);
                    } else {
                        msgpack::pack(buf, res);
                    }
                }
            }
        });
    } else {
        server.register_func(name, [f = std::forward<F>(func)](Message&& msg, GrowableBuffer& out) {
            auto buffer = msg.body();
            if constexpr (std::tuple_size_v<args_type> == 0) {
                if constexpr (std::is_void_v<return_type>) {
                    f();
                } else {
                    auto res = f();
                    WrappedBuffer buf(out);
                    if constexpr (concepts::ProtoType<return_type>) {
                        res.SerializeToZeroCopyStream(&buf);
                    } else {
                        msgpack::pack(buf, res);
                    }
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
                    WrappedBuffer buf(out);
                    if constexpr (concepts::ProtoType<return_type>) {
                        res.SerializeToZeroCopyStream(&buf);
                    } else {
                        msgpack::pack(buf, res);
                    }
                }
            }
        });
    }
}


template<typename R, typename... Args>
asyncio::Task<R, const char*> call_func(Client& client, std::string_view name, Args&&... args) {
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
    co_return (co_await client.call(name, data.read_all()))
    .transform([](Message&& msg) -> R {
        auto body = msg.body();
        if constexpr (!std::is_void_v<R>) {
            R res;
            if constexpr (concepts::ProtoType<R>) {
                res.ParseFromArray(body.data(), body.size());
            } else {
                res = msgpack::unpack(body.data(), body.size())->convert();
            }
            return res;
        }
    });
}

TINYRPC_NS_END

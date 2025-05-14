#pragma once
#if __has_include("msgpack.hpp")
#include <msgpack.hpp>
#endif

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
    server.register_func(name, [f = std::forward<F>(func)](std::string_view buffer, GrowableBuffer& out) {
        return_type res;
        WrappedBuffer buf(out);
        if constexpr (utils::is_proto_args<args_type>) {
            decltype(std::get<0>(*(args_type*)nullptr)) arg;
            arg.ParseFromArray(buffer.data(), buffer.size());
            res = f(std::move(arg));
        } else {
            args_type args = msgpack::unpack(buffer.data(), buffer.size())->convert();
            res = utils::expand_tuple_call(f, std::move(args));
        }
        if constexpr (concepts::ProtoType<return_type>) {
            res.SerializeToZeroCopyStream(&buf);
        } else {
            msgpack::pack(buf, res);
        }
    });
}


template<typename R, typename... Args>
asyncio::Task<void, const char*> call_func(Client& client, const std::string& name, R& res, Args&&... args) {
    std::string data;
    if constexpr (utils::is_proto_args<Args...>) {
        decltype(auto) arg = utils::get_first_arg(args...);
        arg.SerializeToString(&data);
    } else {
        std::stringstream s;
        std::tuple<std::decay_t<Args>...> args_ { std::forward<Args>(args)... };
        msgpack::pack(s, args_);
        s.seekg(0);
        data = std::move(s).str();
    }
    auto resp = co_await client.call(name, data);
    if (!resp) {
        co_return resp.error();
    }
    auto body = resp->body();
    if constexpr (concepts::ProtoType<R>) {
        res.ParseFromArray(body.data(), body.size());
    } else {
        res = msgpack::unpack(body.data(), body.size())->convert();
    }
    co_return nullptr;
}

TINYRPC_NS_END

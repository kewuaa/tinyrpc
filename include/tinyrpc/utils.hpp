#pragma once
#include <tuple>
#include <type_traits>

#include <asyncio.hpp>

#include "../tinyrpc_ns.hpp"
#include "./concepts.hpp"


TINYRPC_NS_BEGIN(utils)

template<typename T>
struct function_traits {};


template<typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using return_type = R;
    using args_type = std::tuple<std::decay_t<Args>...>;
};


template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>>:
    function_traits<R (*)(Args...)> {};


template<typename Arg, typename... Args>
struct first_arg {
    using type = Arg;
};

template<typename Arg, typename... Args>
constexpr decltype(auto) get_first_arg(Arg&& arg, Args&&... args) noexcept {
    return std::forward<Arg>(arg);
}


template<typename... Args>
struct proto_arg {
    static constexpr bool value = concepts::ProtoType<std::decay_t<typename first_arg<Args...>::type>>;
};


template<typename... Args>
struct proto_arg<std::tuple<Args...>> {
    static constexpr bool value = concepts::ProtoType<std::decay_t<typename first_arg<Args...>::type>>;
};


template<typename... Args>
constexpr auto is_proto_args = proto_arg<Args...>::value;


template<typename F, typename... Args, size_t... I>
inline decltype(auto) _expand_tuple_call(F&& f, std::tuple<Args...>&& args, std::index_sequence<I...>) {
    return f(std::get<I>(std::move(args))...);
}


template<typename F, typename... Args>
decltype(auto) expand_tuple_call(F&& f, std::tuple<Args...>&& args) noexcept {
    return _expand_tuple_call(
        std::forward<F>(f),
        std::move(args),
        std::index_sequence_for<Args...>()
    );
}


template<typename T>
void free_and_null(T*& ptr) noexcept {
    if (auto p = std::exchange(ptr, nullptr); p) {
        delete p;
    }
}

TINYRPC_NS_END

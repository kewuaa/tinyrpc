#pragma once
#include <string>
#include <concepts>

#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN(concepts)

template<typename T>
concept ProtoType = requires(T t) {
    requires requires (const std::string& data) {
        { t.ParseFromString(data) } -> std::same_as<bool>;
    };
    requires requires (const char* data, int size) {
        { t.ParseFromArray(data) } -> std::same_as<bool>;
    };
    requires requires (std::string* data) {
        { t.SerializeToString(data) } -> std::same_as<bool>;
    };
};

TINYRPC_NS_END

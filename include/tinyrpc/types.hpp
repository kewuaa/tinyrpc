#pragma once
#include <cstdint>
#include <cstddef>

#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN(types)

struct Header {
    uint64_t id;
    char func_name[32];
    size_t data_len;
} __attribute__((packed));

TINYRPC_NS_END

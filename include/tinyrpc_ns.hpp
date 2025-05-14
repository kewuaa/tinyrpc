#pragma once
#define TINYRPC_NS kwa::tinyrpc
#define TINYRPC_NS_BEGIN(...) namespace TINYRPC_NS __VA_OPT__(::) __VA_ARGS__ {
#define TINYRPC_NS_END }

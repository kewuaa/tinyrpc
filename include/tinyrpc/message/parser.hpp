#pragma once
#include <functional>

#include "tinyrpc_export.hpp"
#include "../message.hpp"
#include "../../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN(message)

using Handle = std::function<void(Message&&)>;

class TINYRPC_EXPORT Parser {
public:
    Parser() noexcept;
    ~Parser() noexcept;
    Parser(Parser&) = delete;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&) = delete;
    Parser& operator=(Parser&&) noexcept;
    void process(const char* data, size_t size, Handle&& handle) noexcept;
private:
    struct impl;
    impl* _pimpl;
};

TINYRPC_NS_END

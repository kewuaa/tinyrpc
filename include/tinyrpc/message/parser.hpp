#pragma once
#include <asyncio.hpp>

#include "tinyrpc_export.hpp"
#include "../message.hpp"
#include "../../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN(message)

class TINYRPC_EXPORT Parser {
public:
    Parser() noexcept;
    ~Parser() noexcept;
    Parser(Parser&) = delete;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&) = delete;
    Parser& operator=(Parser&&) noexcept;
    std::vector<Message> process(const char* data, size_t size) noexcept;
private:
    struct impl;
    impl* _pimpl;
};

TINYRPC_NS_END

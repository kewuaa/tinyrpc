#pragma once
#include <string>

#include "tinyrpc_config.hpp"
#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN()

class Message {
public:
    using ID = uint64_t;

    Message() noexcept = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&& msg) noexcept;
    bool fill_id(char c) noexcept;
    bool fill_func_name(char c) noexcept;
    bool fill_body_size(char c) noexcept;
    bool fill_body(char c) noexcept;

    inline auto id() const noexcept { return *(ID*)(_data.data()+_id_pos); }
    inline auto func_name() const noexcept { return (const char*)(_data.data()+_name_pos); }
    inline auto& body_size() const noexcept { return *(size_t*)(_data.data()+_size_pos); }
    inline auto body() const noexcept { return std::string_view(_data.begin()+_body_pos, _data.end()); }
    inline auto header() const noexcept { return std::string_view(_data.data(), _body_pos); }
    inline const std::string& to_string() const & noexcept { return _data; }
    inline std::string to_string() && noexcept { return std::move(_data); }
private:
    const int _id_pos { sizeof(VERIFY_FLAG) };
    int _name_pos { -1 };
    int _size_pos { -1 };
    int _body_pos { -1 };
    std::string _data { (char*)&VERIFY_FLAG, sizeof(VERIFY_FLAG) };
};

TINYRPC_NS_END

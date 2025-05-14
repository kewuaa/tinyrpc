#include <utility>

#include "tinyrpc/message.hpp"


TINYRPC_NS_BEGIN()

Message& Message::operator=(Message&& msg) noexcept {
    _name_pos = std::exchange(msg._name_pos, -1);
    _size_pos = std::exchange(msg._size_pos, -1);
    _body_pos = std::exchange(msg._body_pos, -1);
    _data = std::exchange(msg._data, {});
    return *this;
}

bool Message::fill_id(char c) noexcept {
    constexpr auto target_size = sizeof(ID);
    _data.push_back(c);
    if (_data.size()-_id_pos == target_size) {
        _name_pos = _data.size();
        return true;
    }
    return false;
}

bool Message::fill_func_name(char c) noexcept {
    _data.push_back(c);
    if (c == '\0') {
        _size_pos = _data.size();
        return true;
    }
    return false;
}

bool Message::fill_body_size(char c) noexcept {
    constexpr auto target_size = sizeof(size_t);
    _data.push_back(c);
    if (_data.size()-_size_pos == target_size) {
        _body_pos = _data.size();
        return true;
    }
    return false;
}

bool Message::fill_body(char c) noexcept {
    auto target_size = body_size();
    _data.push_back(c);
    return _data.size()-_body_pos == target_size;
}

TINYRPC_NS_END

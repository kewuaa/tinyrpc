#include <utility>

#include <spdlog/spdlog.h>

#include "tinyrpc/message/parser.hpp"
#include "tinyrpc/message.hpp"
#include "tinyrpc/utils.hpp"
#include "tinyrpc_config.hpp"


TINYRPC_NS_BEGIN(message)

struct Parser::impl {
    enum class State {
        Verify,
        ID,
        Name,
        Size,
        Body,
    };
    State state { State::Verify };
    std::string flag_buffer {};
    Message msg {};
    std::vector<Message> msgs {};

    inline int16_t flag() const noexcept {
        return *(int16_t*)flag_buffer.data();
    }

    inline bool fill_flag(char c) noexcept {
        flag_buffer.push_back(c);
        return flag_buffer.size() == sizeof(VERIFY_FLAG);
    }

    void trigger_handle() noexcept {
        msgs.push_back(std::exchange(msg, {}));
        SPDLOG_DEBUG("successfully handle message");
        flag_buffer.clear();
        state = State::Verify;
    }

    std::vector<Message> process(const char* data, size_t size) noexcept {
        int pos = 0;
        while (pos < size) {
            switch (state) {
                case State::Verify: {
                    for (; pos < size; ++pos) {
                        if (fill_flag(data[pos])) {
                            if (flag() == VERIFY_FLAG) {
                                state = State::ID;
                                SPDLOG_DEBUG("verify successfully");
                            } else {
                                flag_buffer.clear();
                                SPDLOG_DEBUG("failed to verify");
                            }
                            break;
                        }
                    }
                    break;
                }
                case State::ID: {
                    for (; pos < size; ++pos) {
                        if (msg.fill_id(data[pos])) {
                            state = State::Name;
                            SPDLOG_DEBUG("ID: {}", msg.id());
                            break;
                        }
                    }
                    break;
                }
                case State::Name: {
                    for (; pos < size; ++pos) {
                        if (msg.fill_func_name(data[pos])) {
                            if (msg.func_name().empty()) {
                                SPDLOG_DEBUG("empty function name");
                                trigger_handle();
                                break;
                            }
                            state = State::Size;
                            SPDLOG_DEBUG("function name: {}", msg.func_name());
                            break;
                        }
                    }
                    break;
                }
                case State::Size: {
                    for (; pos < size; ++pos) {
                        if (msg.fill_body_size(data[pos])) {
                            state = State::Body;
                            SPDLOG_DEBUG("body size: {}", msg.body_size());
                            if (msg.body_size() == 0) {
                                trigger_handle();
                            }
                            break;
                        }
                    }
                    break;
                }
                case State::Body: {
                    for (; pos < size; ++pos) {
                        if (msg.fill_body(data[pos])) {
                            SPDLOG_DEBUG("successfully load message body");
                            trigger_handle();
                            break;
                        }
                    }
                    break;
                }
            }
            ++pos;
        }
        return std::exchange(msgs, {});
    }
};


Parser::Parser() noexcept: _pimpl(new impl()) {}


Parser::~Parser() noexcept {
    utils::free_and_null(_pimpl);
}

Parser::Parser(Parser&& p) noexcept: _pimpl(std::exchange(p._pimpl, nullptr)) {
    //
}

Parser& Parser::operator=(Parser&& p) noexcept {
    utils::free_and_null(_pimpl);
    _pimpl = std::exchange(p._pimpl, nullptr);
    return *this;
}

std::vector<Message> Parser::process(const char* data, size_t size) noexcept {
    return _pimpl->process(data, size);
}

TINYRPC_NS_END

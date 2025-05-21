#pragma once
#if TINYRPC_ENABLE_PROTOBUF
#include <google/protobuf/io/zero_copy_stream_impl.h>
#endif

#include <growable_buffer.hpp>

#include "tinyrpc_config.hpp"
#include "../tinyrpc_ns.hpp"


TINYRPC_NS_BEGIN()

class WrappedBuffer
#ifdef TINYRPC_ENABLE_PROTOBUF
final: public google::protobuf::io::ZeroCopyOutputStream
#endif
{
public:
    inline WrappedBuffer(GrowableBuffer& buffer): _buffer(buffer) {}
    WrappedBuffer(WrappedBuffer&) = delete;
    WrappedBuffer(WrappedBuffer&&) = delete;
    WrappedBuffer& operator=(WrappedBuffer&) = delete;
    WrappedBuffer& operator=(WrappedBuffer&&) = delete;

#ifdef TINYRPC_ENABLE_PROTOBUF
    inline bool Next(void** data, int* size) override {
        auto view = _buffer.malloc(TINYRPC_DEFAULT_BUFFER_SIZE);
        *data = view.data();
        *size = TINYRPC_DEFAULT_BUFFER_SIZE;
        _count += view.size();
        return true;
    }

    inline void BackUp(int count) override {
        _count -= count;
        _buffer.backup(count);
    }

    inline int64_t ByteCount() const override {
        return _count;
    }
#endif

    inline void write(const char* data, size_t size) noexcept {
        _buffer.write({ data, size });
    }
private:
    GrowableBuffer& _buffer;
#ifdef TINYRPC_ENABLE_PROTOBUF
    int64_t _count = 0;
#endif
};

TINYRPC_NS_END

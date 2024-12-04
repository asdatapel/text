#pragma once

#include "gpu/metal/device.hpp"
#include "types.hpp"

namespace Gpu {

struct Buffer {
    MTL::Buffer *mtl_buffer = nullptr;
    void *data = nullptr;
    i32 size;
};

Buffer create_buffer(Device *device, i32 size)
{
    Buffer buffer;
    buffer.mtl_buffer = 
        device->metal_device->newBuffer(size, MTL::ResourceStorageModeShared);
    buffer.size = size;
    buffer.data = (u8 *)buffer.mtl_buffer->contents();

    return buffer;
}

Buffer create_buffer(Device *device, void *data, i32 size)
{
    Buffer buffer;
    buffer.mtl_buffer = 
        device->metal_device->newBuffer(data, size, MTL::ResourceStorageModeShared);
    buffer.size = size;
    buffer.data = (u8 *)buffer.mtl_buffer->contents();

    return buffer;
}

void destroy_buffer(Buffer buffer)
{
    buffer.mtl_buffer->release();
}

void upload_buffer(Buffer buffer, void *data, i32 size, i32 offset)
{
    assert(buffer.data);
    memcpy((u8 *)buffer.data + offset, data, size);
}

}

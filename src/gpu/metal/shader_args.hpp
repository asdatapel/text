#pragma once

#include "gpu/metal/buffer.hpp"
#include "gpu/metal/converters.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/pipeline.hpp"
#include "gpu/metal/texture.hpp"
#include "gpu/shader_args.hpp"
#include "metal_headers.hpp"
#include "types.hpp"

namespace Gpu {
struct ShaderArgBuffer {
    Buffer buffer;
    MTL::ArgumentEncoder *arg_encoder;
    Array<MTL::ArgumentDescriptor *, 16> arg_descriptors;
};

ShaderArgBuffer create_shader_arg_buffer(Device *device, Pipeline *pipeline)
{
    ShaderArgBuffer arg_buffer;

    i32 index = 0;
    for (i32 i = 0; i < pipeline->arg_defs.size; i++) {
        MTL::ArgumentDescriptor *descriptor = MTL::ArgumentDescriptor::alloc()->init();
        descriptor->setIndex(index);
        descriptor->setDataType(map_data_type(pipeline->arg_defs[i].type));
        descriptor->setAccess(map_access(pipeline->arg_defs[i].access));
        descriptor->setArrayLength(pipeline->arg_defs[i].count);
        arg_buffer.arg_descriptors.push_back(descriptor);

        index += 1 + pipeline->arg_defs[i].count;
    }
    
    NS::Array *d = NS::Array::alloc()->init((const NS::Object *const *) arg_buffer.arg_descriptors.data, arg_buffer.arg_descriptors.size);
    MTL::ArgumentEncoder *arg_encoder = device->metal_device->newArgumentEncoder(d);
    d->release();

    arg_buffer.arg_encoder = arg_encoder;
    arg_buffer.buffer = create_buffer(device, arg_encoder->encodedLength());
    return arg_buffer;
}

void destroy_shader_arg_buffer(ShaderArgBuffer arg_buffer) {
    for (i32 i = 0; i < arg_buffer.arg_descriptors.size; i++) {
        arg_buffer.arg_descriptors[i]->release();
    }

    arg_buffer.arg_encoder->release();
    destroy_buffer(arg_buffer.buffer);
}

void bind_shader_buffer_data(ShaderArgBuffer arg_buffer, Buffer data_buffer, i32 arg_i, i32 array_i)
{
    arg_buffer.arg_encoder->setArgumentBuffer(arg_buffer.buffer.mtl_buffer, 0);
    arg_buffer.arg_encoder->setBuffer(data_buffer.mtl_buffer, 0, arg_buffer.arg_descriptors[arg_i]->index() + array_i);
}

void bind_shader_buffer_texture(ShaderArgBuffer arg_buffer, Texture texture, i32 arg_i, i32 array_i)
{
    arg_buffer.arg_encoder->setArgumentBuffer(arg_buffer.buffer.mtl_buffer, 0);
    arg_buffer.arg_encoder->setTexture(texture.mtl_texture, arg_buffer.arg_descriptors[arg_i]->index() + array_i);
}

}
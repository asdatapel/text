#pragma once

#include <random>
#include "gpu/metal/buffer.hpp"
#include "gpu/shader_args.hpp"
#include "metal_headers.hpp"
#include "types.hpp"

namespace Gpu
{

void draw_indexed(Device *device,
                  Buffer index_buffer, i32 offset, i32 index_count)
{   
    device->render_command_encoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle, 
        index_count, MTL::IndexTypeUInt32, index_buffer.mtl_buffer, offset * 4,
        1, 0, 0);
}

void draw(Device *device,
          Buffer vertex_buffer, i32 offset, i32 vertex_count)
{
    device->render_command_encoder->setVertexBuffer(vertex_buffer.mtl_buffer, 0, 0);
    device->render_command_encoder->drawPrimitives(
        MTL::PrimitiveTypeTriangle, offset, vertex_count);
}
}

#pragma once

#include "gpu/metal/metal_headers.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/texture.hpp"

namespace Gpu
{

struct RenderTarget {
  MTL::RenderPassDescriptor *mtl_render_pass_descriptor;
  MTL::DepthStencilState *depth_stencil_state;
};

RenderTarget create_render_target(Gpu::Device *device, std::optional<Texture> texture,
                                  std::optional<Texture> depth_texture,
                                  std::optional<Color> clear_color)
{
  RenderTarget target;
  target.mtl_render_pass_descriptor = MTL::RenderPassDescriptor::alloc()->init();

  if (texture) {
    MTL::RenderPassColorAttachmentDescriptor *cd =
      target.mtl_render_pass_descriptor->colorAttachments()->object(0);
    cd->setTexture(texture->mtl_texture);
    cd->setStoreAction(MTL::StoreActionStore);

    if (clear_color) {
      cd->setLoadAction(MTL::LoadActionClear);
      cd->setClearColor(
        MTL::ClearColor(clear_color->r, clear_color->g, clear_color->b, clear_color->a));
    } else {
      cd->setLoadAction(MTL::LoadActionLoad);
    }
  }

  if (depth_texture) {
    MTL::DepthStencilDescriptor *depth_stencil_descriptor =
        MTL::DepthStencilDescriptor::alloc()->init();
    depth_stencil_descriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depth_stencil_descriptor->setDepthWriteEnabled(true);
    target.depth_stencil_state =
        device->metal_device->newDepthStencilState(depth_stencil_descriptor);
    depth_stencil_descriptor->release();

    MTL::RenderPassDepthAttachmentDescriptor *depth_attachment =
        target.mtl_render_pass_descriptor->depthAttachment();
    depth_attachment->setTexture(depth_texture->mtl_texture);
    depth_attachment->setLoadAction(MTL::LoadActionClear);
    depth_attachment->setStoreAction(MTL::StoreActionDontCare);
    depth_attachment->setClearDepth(1.0);
  }

  return target;
}
void destroy_render_target(RenderTarget target)
{
  target.mtl_render_pass_descriptor->release();
}

void start_render_pass(Device *device, RenderTarget target)
{
  device->render_command_encoder = device->metal_command_buffer->renderCommandEncoder(
      target.mtl_render_pass_descriptor);
}

void end_render_pass(Device *device, RenderTarget target)
{
  device->render_command_encoder->endEncoding();
  device->render_command_encoder = nullptr;
}

void set_render_target_color_attachment(RenderTarget target, i32 attachment_index,
                                        Gpu::Texture texture, i32 mip, i32 layer)
{
  MTL::RenderPassColorAttachmentDescriptor *cd =
      target.mtl_render_pass_descriptor->colorAttachments()->object(attachment_index);
  cd->setTexture(texture.mtl_texture);
  cd->setLevel(mip);
  cd->setSlice(layer);
}

}  // namespace Gpu

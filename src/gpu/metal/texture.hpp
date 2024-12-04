#pragma once

#include "Metal/MTLHeaderBridge.hpp"
#include "Metal/MTLPixelFormat.hpp"
#include "gpu/metal/converters.hpp"
#include "gpu/metal/pipeline.hpp"
#include "image.hpp"

namespace Gpu
{

struct Texture {
  MTL::Texture* mtl_texture;
};

Texture create_texture(Device* device, Image image)
{
  MTL::TextureDescriptor* texture_descriptor =
      MTL::TextureDescriptor::alloc()->init();
  texture_descriptor->setPixelFormat(MTL::PixelFormatR8Unorm);
  texture_descriptor->setWidth(image.width);
  texture_descriptor->setHeight(image.height);
  texture_descriptor->setStorageMode(MTL::StorageModeShared);

  Texture texture;
  texture.mtl_texture = device->metal_device->newTexture(texture_descriptor);

  MTL::Region region = MTL::Region(0, 0, 0, image.width, image.height, 1);
  NS::UInteger bytes_per_row = 1 * image.width;
  texture.mtl_texture->replaceRegion(region, 0, image.data(), bytes_per_row);

  texture_descriptor->release();

  return texture;
}

Texture create_render_target_texture(Device* device, u32 width, u32 height,
                                     PixelFormat format)
{
  MTL::TextureDescriptor* texture_descriptor =
      MTL::TextureDescriptor::alloc()->init();
  texture_descriptor->setPixelFormat(map_pixel_format(format));
  texture_descriptor->setWidth(width);
  texture_descriptor->setHeight(height);
  texture_descriptor->setUsage(MTL::TextureUsageRenderTarget |
                               MTL::TextureUsageShaderRead);
  texture_descriptor->setStorageMode(MTL::StorageModePrivate);

  Texture texture;
  texture.mtl_texture = device->metal_device->newTexture(texture_descriptor);

  texture_descriptor->release();

  return texture;
}

Texture create_render_target_cubemap(Device* device, u32 size,
                                     PixelFormat format, bool mipmaps)
{
  MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::textureCubeDescriptor(
                                               map_pixel_format(format), size, mipmaps);
  texture_descriptor->setUsage(MTL::TextureUsageRenderTarget |
                               MTL::TextureUsageShaderRead);
  texture_descriptor->setStorageMode(MTL::StorageModePrivate);

  Texture texture;
  texture.mtl_texture = device->metal_device->newTexture(texture_descriptor);

  return texture;
}

Texture create_cubemap(Device* device, u32 size, PixelFormat format)
{
  MTL::TextureDescriptor* texture_descriptor =
      MTL::TextureDescriptor::textureCubeDescriptor(map_pixel_format(format),
                                                    size, false);

  Texture texture;
  texture.mtl_texture = device->metal_device->newTexture(texture_descriptor);

  return texture;
}

}  // namespace Gpu

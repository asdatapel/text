#pragma once

#include "Metal/MTLPixelFormat.hpp"
#include "gpu/shader_args.hpp"
#include "metal_headers.hpp"
#include "platform.hpp"
#include "types.hpp"

namespace Gpu {

MTL::DataType map_data_type(ShaderArgumentDefinition::Type in)
{
    switch (in) {
    case ShaderArgumentDefinition::Type::DATA:
        return MTL::DataType::DataTypePointer;
        break;
    case ShaderArgumentDefinition::Type::SAMPLER:
        return MTL::DataType::DataTypeSampler;
        break;
    case ShaderArgumentDefinition::Type::TEXTURE:
        return MTL::DataType::DataTypeTexture;
        break;
    }
}

MTL::ArgumentAccess map_access(Access in)
{
    if (in & Access::READ & Access::WRITE) {
        return MTL::ArgumentAccess::ArgumentAccessReadWrite;
    }
    if (in & Access::READ) {
        return MTL::ArgumentAccess::ArgumentAccessReadOnly;
    }
    if (in & Access::WRITE) {
        return MTL::ArgumentAccess::ArgumentAccessWriteOnly;
    }

    return MTL::ArgumentAccess::ArgumentAccessReadOnly;
}

MTL::PixelFormat map_pixel_format(PixelFormat in)
{
    switch (in) {
        case PixelFormat::RGBA8U:
            return MTL::PixelFormat::PixelFormatRGBA8Unorm;
        case PixelFormat::RGBA16F:
            return MTL::PixelFormat::PixelFormatRGBA16Float;
        case PixelFormat::RG16F:
            return MTL::PixelFormatRG16Float;
        case PixelFormat::RG32F:
            return MTL::PixelFormat::PixelFormatRG32Float;
        case PixelFormat::RGBA32F:
            return MTL::PixelFormat::PixelFormatRGBA32Float;
        case PixelFormat::DEPTH16:
            return MTL::PixelFormat::PixelFormatDepth16Unorm;
        case PixelFormat::DEPTH32:
            return MTL::PixelFormat::PixelFormatDepth32Float;
        case PixelFormat::UNKNOWN:
            return MTL::PixelFormat::PixelFormatInvalid;
    }
}

PixelFormat map_pixel_format(MTL::PixelFormat in)
{
    switch (in) {
        case MTL::PixelFormat::PixelFormatRGBA8Uint:
            return PixelFormat::RGBA8U;
        case MTL::PixelFormat::PixelFormatRGBA16Float:
            return PixelFormat::RGBA16F;
        case MTL::PixelFormat::PixelFormatRG32Float:
            return PixelFormat::RG32F;
        case MTL::PixelFormat::PixelFormatRGBA32Float:
            return PixelFormat::RGBA32F;
        case MTL::PixelFormat::PixelFormatDepth16Unorm:
            return PixelFormat::DEPTH16;
        case MTL::PixelFormat::PixelFormatDepth32Float:
            return PixelFormat::DEPTH32;
        default:
            return PixelFormat::UNKNOWN;
    }
}

}

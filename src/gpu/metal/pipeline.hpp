#pragma once

#include "gpu/metal/metal_headers.hpp"
#include "gpu/metal/converters.hpp"
#include "gpu/pipeline.hpp"
#include "types.hpp"

namespace Gpu {

struct ShaderArgs {

};
struct Pipeline {
    MTL::RenderPipelineState* pso;

    Array<ShaderArgumentDefinition, 16> arg_defs;
};

Pipeline create_pipeline(Device *device, PipelineDefinition def)
{
    Pipeline pipeline;
    pipeline.arg_defs = def.arg_defs;

    NS::String *vert_shader_name = NS::String::alloc()->init(def.vert_shader.data, def.vert_shader.size, NS::StringEncoding::UTF8StringEncoding, false);
    NS::String *frag_shader_name = NS::String::alloc()->init(def.frag_shader.data, def.frag_shader.size, NS::StringEncoding::UTF8StringEncoding, false);
    
    MTL::Function* vertex_shader = device->shader_library->newFunction(vert_shader_name);
    assert(vertex_shader);
    MTL::Function* fragment_shader = device->shader_library->newFunction(frag_shader_name);
    assert(fragment_shader);

    MTL::RenderPipelineDescriptor* render_pipeline_descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    render_pipeline_descriptor->setLabel(NS::String::string("Triangle Rendering Pipeline", NS::ASCIIStringEncoding));
    render_pipeline_descriptor->setVertexFunction(vertex_shader);
    render_pipeline_descriptor->setFragmentFunction(fragment_shader);
    assert(render_pipeline_descriptor);

    MTL::PixelFormat pixel_format = (def.format == PixelFormat::UNKNOWN)
        ? (MTL::PixelFormat)device->metal_layer->pixelFormat()
        : map_pixel_format(def.format);
    
    MTL::RenderPipelineColorAttachmentDescriptor *color_attachment = render_pipeline_descriptor->colorAttachments()->object(0);
    color_attachment->setPixelFormat(pixel_format);
    color_attachment->setBlendingEnabled(true);
    color_attachment->setRgbBlendOperation(MTL::BlendOperationAdd);
    color_attachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
    color_attachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    color_attachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    color_attachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    color_attachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

    if (def.depth_buffer_enabled) {
        render_pipeline_descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth16Unorm);
    }

    NS::Error* error;
    pipeline.pso = device->metal_device->newRenderPipelineState(render_pipeline_descriptor, &error);
    if (error) {
        std::cerr << "Failed to build pipeline.\n" << error->debugDescription()->cString(NS::ASCIIStringEncoding) << '\n';
    }

    render_pipeline_descriptor->release();
    vert_shader_name->release();
    frag_shader_name->release();
    vertex_shader->release();
    fragment_shader->release();

    return pipeline;
}

void destroy_pipeline(Pipeline pipeline)
{
    pipeline.pso->release();
}

void bind_pipeline(Device *device, Pipeline pipeline)
{
    device->render_command_encoder->setRenderPipelineState(pipeline.pso);
}

}

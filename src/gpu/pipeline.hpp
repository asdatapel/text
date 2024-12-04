#pragma once

#include "containers/array.hpp"
#include "file.hpp"
#include "gpu/shader_args.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "platform.hpp"
#include "types.hpp"

namespace Gpu
{

struct VertexDescription {
  Array<PixelFormat, 10> attributes;
};

struct PipelineDefinition {
  String vert_shader;
  String frag_shader;

  bool depth_buffer_enabled = false;
  PixelFormat format;

  Array<ShaderArgumentDefinition, 16> arg_defs;
};
}  // namespace Gpu
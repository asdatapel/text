#pragma once

#include "types.hpp"

namespace Gpu
{

enum Access {
  READ  = 1,
  WRITE = 2,
};

struct ShaderArgumentDefinition {
  enum struct Type {
    DATA,
    TEXTURE,
    SAMPLER,
  };

  Type type;
  Access access;
  i32 count = 0;
};

}  // namespace Gpu
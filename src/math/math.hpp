#pragma once

#include "math/color.hpp"
#include "math/common.hpp"
#include "math/matrix.hpp"
#include "math/rect.hpp"
#include "math/vector.hpp"

struct AABB {
  Vec3f min = {100000, 100000, 100000};
  Vec3f max = {-100000, -100000, -100000};
};
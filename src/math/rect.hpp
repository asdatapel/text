#pragma once

#include "math/vector.hpp"

struct Rect4f {
  f32 x = 0, y = 0;
  f32 width = 0, height = 0;

  Rect4f() {}
  Rect4f(f32 x, f32 y, f32 width, f32 height)
  {
    this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
  }
  Rect4f(Vec2f pos, Vec2f size)
  {
    this->x      = pos.x;
    this->y      = pos.y;
    this->width  = size.x;
    this->height = size.y;
  }

  void set_bottom(f32 down) { height = down - y; }
  void set_right(f32 right) { width = right - x; }

  Vec2f xy() { return {x, y}; }
  Vec2f span() { return {width, height}; }
  Vec2f center() { return {x + (width / 2), y + (height / 2)}; }
  f32 left() { return x; }
  f32 right() { return x + width; }
  f32 top() { return y; }
  f32 bottom() { return y + height; }

  static Rect4f from_ends(Vec2f a, Vec2f b)
  {
    return Rect4f(Vec2f{fminf(a.x, b.x), fminf(a.y, b.y)},
                Vec2f{fabs(b.x - a.x), fabs(b.y - a.y)});
  }
};

inline b8 operator==(const Rect4f &lhs, const Rect4f &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width &&
         lhs.height == rhs.height;
}

inline b8 operator!=(const Rect4f &lhs, const Rect4f &rhs) { return !(lhs == rhs); }

b8 overlaps(Rect4f rect1, Rect4f rect2)
{
  return !(rect1.right() < rect2.left() || rect1.left() > rect2.right() ||
         rect1.bottom() < rect2.top() || rect1.top() > rect2.bottom());
}

b8 in_rect(Vec2f point, Rect4f rect)
{
  return point.x > rect.x && point.x < rect.x + rect.width &&
         point.y > rect.y && point.y < rect.y + rect.height;
}

b8 in_rect(Vec2f point, Rect4f rect, Rect4f mask)
{
  if (mask.width == 0 || mask.height == 0) {
    mask = rect;
  }

  return point.x > rect.x && point.x < rect.x + rect.width &&
         point.y > rect.y && point.y < rect.y + rect.height &&
         point.x > mask.x && point.x < mask.x + mask.width &&
         point.y > mask.y && point.y < mask.y + mask.height;
}

Rect4f inset(Rect4f rect, f32 inset)
{
  rect.x += inset;
  rect.y += inset;
  rect.width -= inset * 2;
  rect.height -= inset * 2;
  return rect;
}
Rect4f outset(Rect4f rect, f32 outset)
{
  rect.x -= outset;
  rect.y -= outset;
  rect.width += outset * 2;
  rect.height += outset * 2;
  return rect;
}
Vec2f clamp_to_rect(Vec2f val, Rect4f rect)
{
  return max(min(val, rect.xy() + rect.span()), Vec2f{0, 0});
}

template <typename T>
struct Rect4 {
  T x = 0, y = 0;
  T width = 0, height = 0;

  Rect4() {}
  Rect4(T x, T y, T width, T height)
  {
    this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
  }
  Rect4(Vec2f pos, Vec2f size)
  {
    this->x      = pos.x;
    this->y      = pos.y;
    this->width  = size.x;
    this->height = size.y;
  }

  void set_bottom(T down) { height = down - y; }
  void set_right(T right) { width = right - x; }

  Vec2f xy() { return {x, y}; }
  Vec2f span() { return {width, height}; }
  Vec2f center() { return {x + (width / 2), y + (height / 2)}; }
  T left() { return x; }
  T right() { return x + width; }
  T top() { return y; }
  T bottom() { return y + height; }

  static Rect4 from_ends(Vec2f a, Vec2f b)
  {
    return Rect4f(Vec2f{fminf(a.x, b.x), fminf(a.y, b.y)},
                Vec2f{fabs(b.x - a.x), fabs(b.y - a.y)});
  }
};

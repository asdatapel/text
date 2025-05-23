#pragma once

#include "containers/static_stack.hpp"
#include "font.hpp"
#include "gpu/gpu.hpp"
#include "math/math.hpp"
#include "types.hpp"

namespace Draw
{

const u32 CORNERS[] = {
    0x00000000,
    0x00010000,
    0x00020000,
    0x00030000,
};

enum struct PrimitiveIds : u32 {
  RECT         = 1 << 18,
  ROUNDED_RECT = 2 << 18,
  TEXTURE_RECT = 3 << 18,
  BITMAP_GLYPH = 4 << 18,
  VECTOR_GLYPH = 5 << 18,
  LINE         = 6 << 18,
};

struct RectPrimitive {
  Rect4f rect;
};

struct RoundedRectPrimitive {
  Rect4f dimensions;
  u32 clip_rect_idx;
  u32 color;
  f32 corner_radius;
  u32 corner_mask;
};

struct TextureRectPrimitive {
  Rect4f dimensions;
  Vec4f uv_bounds;
  i32 texture_idx;
  u32 clip_rect_idx;
  // Vec2f pad;
};

struct BitmapGlyphPrimitive {
  Rect4f dimensions;
  Vec4f uv_bounds;
  u32 clip_rect_idx;
  u32 color;
  u32 texture_idx;
};

struct ConicCurvePrimitive {
  Vec2f p0, p1, p2;
  // Vec2f pad;
};

struct VectorGlyphPrimitive {
  Rect4f dimensions;
  u32 curve_start_idx;
  u32 curve_count;
  u32 color;
  u32 clip_rect_idx;
};

struct LinePrimitive {
  Vec2f a;
  Vec2f b;
  u32 color;
  u32 clip_rect_idx;
  // Vec2f pad;
};

struct Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[50000];
  TextureRectPrimitive texture_rects[1024];
  VectorGlyphPrimitive vector_glyphs[1024];
  ConicCurvePrimitive conic_curves[1024];
  LinePrimitive lines[1024];
  Vec4f canvas_size;
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  i32 z;
};

struct DrawSettings {
  b8 force_scissor = false;
  u32 scissor_idx  = 0;
};

struct List {
  Font font;
  Gpu::Texture font_texture;

  u64 frame = 0;
  Vec2f canvas_size;

  Gpu::Pipeline pipeline;
  Gpu::Buffer primitive_buffer;
  Gpu::ShaderArgBuffer shader_args;

  Primitives primitives;
  i32 clip_rects_count    = 0;
  i32 rounded_rects_count = 0;
  i32 texture_rects_count = 0;
  i32 bitmap_glyphs_count = 0;
  i32 conic_curves_count  = 0;
  i32 vector_glyphs_count = 0;
  i32 lines_count         = 0;

  StaticStack<Rect4f, 1024> scissors;
  StaticStack<u32, 1024> scissor_idxs;

  i32 texture_count = 0;
  Gpu::Texture textures[128];

  StaticStack<DrawSettings, 32> settings;

  u32 *verts     = nullptr;
  i32 vert_count = 0;
  Gpu::Buffer index_buffer;

  Array<DrawCall, 128> draw_calls;
  i32 max_z = 0;
};

void push_draw_settings(List *dl, DrawSettings ds) { dl->settings.push_back(ds); }
void pop_draw_settings(List *dl) { dl->settings.pop(); }

u32 get_current_scissor_idx(List *dl)
{
  if (dl->settings.size > 0 && dl->settings.top().force_scissor) {
    return dl->settings.top().scissor_idx;
  }
  return dl->scissor_idxs.top();
}
Rect4f get_current_scissor(List *dl)
{
  if (dl->settings.size > 0 && dl->settings.top().force_scissor) {
    return dl->primitives.clip_rects[dl->settings.top().scissor_idx].rect;
  }
  return dl->scissors.top();
}
u32 push_scissor(List *dl, Rect4f rect)
{
  auto to_bounds = [](Rect4f r) {
    return Vec4f{r.x, r.y, r.x + r.width, r.y + r.height};
  };
  if (dl->scissors.size > 0) {
    Vec4f rect_bounds   = to_bounds(rect);
    Vec4f parent_bounds = to_bounds(dl->scissors.top());

    rect_bounds.x = fmaxf(rect_bounds.x, parent_bounds.x);
    rect_bounds.y = fmaxf(rect_bounds.y, parent_bounds.y);
    rect_bounds.z = fminf(rect_bounds.z, parent_bounds.z);
    rect_bounds.w = fminf(rect_bounds.w, parent_bounds.w);

    rect = {rect_bounds.x, rect_bounds.y, rect_bounds.z - rect_bounds.x,
            rect_bounds.w - rect_bounds.y};
  }

  dl->primitives.clip_rects[dl->clip_rects_count++] = {rect};
  dl->scissor_idxs.push_back(dl->clip_rects_count - 1);
  dl->scissors.push_back(rect);

  return dl->clip_rects_count - 1;
}
void pop_scissor(List *dl)
{
  dl->scissor_idxs.pop();
  dl->scissors.pop();
}
void push_base_scissor(List *dl)
{
  dl->scissor_idxs.push_back(0);
  dl->scissors.push_back(dl->scissors[0]);
}

u32 push_texture(List *dl, Gpu::Texture texture)
{
  dl->textures[dl->texture_count++] = texture;
  return dl->texture_count - 1;
}

void push_draw_call(List *dl, i32 tri_count, i32 z)
{
  if (dl->draw_calls.size > 0) {
    DrawCall *last_draw_call = &dl->draw_calls[dl->draw_calls.size - 1];
    if (last_draw_call->z == z) {
      last_draw_call->tri_count += tri_count;
      return;
    }
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count, z};
  dl->draw_calls.push_back(dc);
  dl->max_z = std::max(dl->max_z, z);
}

enum CornerMask : u32 {
  TOP_LEFT     = 0b0001,
  TOP_RIGHT    = 0b0010,
  BOTTOM_LEFT  = 0b0100,
  BOTTOM_RIGHT = 0b1000,
  TOP          = TOP_LEFT | TOP_RIGHT,
  BOTTOM       = BOTTOM_LEFT | BOTTOM_RIGHT,
  ALL          = TOP | BOTTOM,
};
u32 push_primitive_rounded_rect(List *dl, Rect4f rect, Color color, f32 corner_radius,
                                u32 corner_mask)
{
  dl->primitives.rounded_rects[dl->rounded_rects_count++] = {
      rect, get_current_scissor_idx(dl), color_to_int(color), corner_radius, corner_mask};

  return dl->rounded_rects_count - 1;
}

RoundedRectPrimitive *push_rounded_rect(List *dl, i32 z, Rect4f rect, f32 corner_radius,
                                        Color color, u32 corner_mask = CornerMask::ALL)
{
  auto push_rect_vert = [](List *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::ROUNDED_RECT | CORNERS[corner] |
                                   primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return nullptr;
  }

  u32 primitive_idx =
      push_primitive_rounded_rect(dl, rect, color, corner_radius, corner_mask);

  push_rect_vert(dl, primitive_idx, 0);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 2);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 3);
  push_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);

  return &dl->primitives.rounded_rects[primitive_idx];
}

RoundedRectPrimitive *push_rect(List *dl, i32 z, Rect4f rect, Color color)
{
  return push_rounded_rect(dl, z, rect, 0, color);
}

u32 push_primitive_bitmap_glyph(List *dl, Rect4f rect, Vec4f uv_bounds, Color color,
                                u32 texture_idx)
{
  dl->primitives.bitmap_glyphs[dl->bitmap_glyphs_count++] = {
      rect, uv_bounds, get_current_scissor_idx(dl), color_to_int(color), texture_idx};

  return dl->bitmap_glyphs_count - 1;
}

void push_bitmap_glyph(List *dl, i32 z, Rect4f rect, Vec4f uv_bounds, Color color,
                       u32 texture_idx)
{
  auto push_glyph_vert = [](List *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::BITMAP_GLYPH | CORNERS[corner] |
                                   primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx =
      push_primitive_bitmap_glyph(dl, rect, uv_bounds, color, texture_idx);

  push_glyph_vert(dl, primitive_idx, 0);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 2);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 3);
  push_glyph_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

// void push_text(List *dl, Font font, i32 z, String text, Vec2f pos, Color color,
//                f32 height)
// {
//   f32 baseline     = font.baseline;
//   f32 resize_ratio = height / font.font_size_px;
//   for (int i = 0; i < text.size; i++) {
//     Character c     = font.characters[text.data[i]];
//     Rect4f shape_rect = {
//         pos.x + c.shape.x * resize_ratio, pos.y + (c.shape.y) * resize_ratio,
//         c.shape.width * resize_ratio, c.shape.height * resize_ratio};
//     pos.x += c.advance * resize_ratio;

//     shape_rect.x = floorf(shape_rect.x);
//     shape_rect.y = floorf(shape_rect.y);

//     Vec4f uv_bounds = {c.uv.x, c.uv.y + c.uv.height, c.uv.x + c.uv.width,
//                        c.uv.y};
//     push_bitmap_glyph(dl, z, shape_rect, uv_bounds, color, 0);

//     error(height, ", ", font.font_size_px, ", ", resize_ratio, ", ", shape_rect.x,
//     shape_rect.y, shape_rect.width, shape_rect.height);
//   }
// };

// void push_text_centered(List *dl, i32 z, String text, Vec2f pos,
//                         Vec2b center, Color color, f32 height)
// {
//   f32 resize_ratio = height / dl->font.font_size_px;
//   f32 line_width   = get_text_width(dl->font, text, resize_ratio);

//   Vec2f centered_pos;
//   centered_pos.x = center.x ? pos.x - line_width / 2 : pos.x;
//   centered_pos.y =
//       center.y ? pos.y + (dl->font.ascent + dl->font.descent) / 2.f : pos.y;

//   push_text(dl, dl->font, z, text, centered_pos, color, height);
// }

void push_texture_rect(List *dl, i32 z, Rect4f rect, Vec4f uv_bounds,
                       Gpu::Texture texture)
{
  auto push_primitive_texture_rect = [](List *dl, Rect4f rect, Vec4f uv_bounds,
                                        i32 texture_idx) {
    dl->primitives.texture_rects[dl->texture_rects_count++] = {
        rect, uv_bounds, texture_idx, get_current_scissor_idx(dl)};

    return dl->texture_rects_count - 1;
  };
  auto push_texture_rect_vert = [](List *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::TEXTURE_RECT | CORNERS[corner] |
                                   primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return;
  }

  i32 texture_idx   = push_texture(dl, texture);
  u32 primitive_idx = push_primitive_texture_rect(dl, rect, uv_bounds, texture_idx);

  push_texture_rect_vert(dl, primitive_idx, 0);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 2);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 3);
  push_texture_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

u32 push_primitive_line(List *dl, Vec2f a, Vec2f b, Color color)
{
  dl->primitives.lines[dl->lines_count++] = {a, b, color_to_int(color),
                                             get_current_scissor_idx(dl)};

  return dl->lines_count - 1;
}

void push_line(List *dl, i32 z, Vec2f a, Vec2f b, Color color)
{
  auto push_vert = [](List *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::LINE | CORNERS[corner] |
                                   primitive_index};
  };

  Vec2f mins          = min(a, b);
  Vec2f maxs          = max(a, b);
  Rect4f bounding_box = {mins.x, mins.y, maxs.x - mins.x, maxs.y - mins.y};
  if (!overlaps(bounding_box, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx = push_primitive_line(dl, a, b, color);

  push_vert(dl, primitive_idx, 0);
  push_vert(dl, primitive_idx, 1);
  push_vert(dl, primitive_idx, 2);
  push_vert(dl, primitive_idx, 1);
  push_vert(dl, primitive_idx, 3);
  push_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

void push_cubic_spline(List *dl, i32 z, Vec2f p[4], Color color, i32 n_segments)
{
  auto evaluate_spline = [&](f32 t) {
    f32 t2  = t * t;
    f32 t3  = t2 * t;
    Vec2f a = (-p[0] + 3 * p[1] - 3 * p[2] + p[3]) * t3;
    Vec2f b = (3 * p[0] - 6 * p[1] + 3 * p[2]) * t2;
    Vec2f c = (-3 * p[0] + 3 * p[1]) * t;
    Vec2f d = p[0];

    return a + b + c + d;
  };

  for (i32 i = 0; i < n_segments; i++) {
    f32 t0      = (f32)i / n_segments;
    f32 t1      = (f32)(i + 1) / n_segments;
    Vec2f start = evaluate_spline(t0);
    Vec2f end   = evaluate_spline(t1);

    push_line(dl, z, start, end, color);
  }
}

void init_draw_system(List *dl, Gpu::Device *gpu)
{
  Gpu::ShaderArgumentDefinition shader_arg_def_primitives;
  shader_arg_def_primitives.type = Gpu::ShaderArgumentDefinition::Type::DATA;
  shader_arg_def_primitives.access =
      (Gpu::Access)(Gpu::Access::WRITE | Gpu::Access::READ);
  Gpu::ShaderArgumentDefinition shader_arg_def_textures;
  shader_arg_def_textures.type   = Gpu::ShaderArgumentDefinition::Type::TEXTURE;
  shader_arg_def_textures.access = (Gpu::Access)(Gpu::Access::READ);
  shader_arg_def_textures.count  = 128;
  Gpu::PipelineDefinition pipeline_def;
  pipeline_def.vert_shader = "vertex_shader";
  pipeline_def.frag_shader = "fragment_shader";
  pipeline_def.arg_defs    = {shader_arg_def_primitives, shader_arg_def_textures};
  pipeline_def.format      = PixelFormat::UNKNOWN;
  dl->pipeline             = Gpu::create_pipeline(gpu, pipeline_def);

  dl->shader_args      = Gpu::create_shader_arg_buffer(gpu, &dl->pipeline);
  dl->primitive_buffer = Gpu::create_buffer(gpu, 128 * MB);
  Gpu::bind_shader_buffer_data(dl->shader_args, dl->primitive_buffer, 0, 0);

  dl->verts        = (u32 *)malloc(sizeof(u32) * 1024 * 1024);
  dl->index_buffer = create_buffer(gpu, MB);

  dl->font         = load_font("resources/fonts/jetbrains/JetBrainsMono-Medium.ttf", 24);
  dl->font_texture = Gpu::create_texture(gpu, dl->font.bitmap);
}

void start_frame(List *dl, Vec2f canvas_size)
{
  dl->vert_count = 0;
  dl->draw_calls.clear();
  dl->max_z = -1;

  dl->clip_rects_count    = 0;
  dl->rounded_rects_count = 0;
  dl->texture_rects_count = 0;
  dl->bitmap_glyphs_count = 0;
  dl->vector_glyphs_count = 0;
  dl->lines_count         = 0;

  dl->texture_count = 0;

  dl->scissor_idxs.clear();
  dl->scissors.clear();

  push_scissor(dl, {0, 0, canvas_size.x, canvas_size.y});

  dl->canvas_size = canvas_size;

  Draw::push_texture(dl, dl->font_texture);
}

void end_frame(List *dl, Gpu::Device *gpu, u64 frame)
{
  dl->primitives.canvas_size = Vec4f{dl->canvas_size.x, dl->canvas_size.y, 0, 0};

  if (dl->frame != frame) {
    dl->frame = frame;
    Gpu::upload_buffer(dl->primitive_buffer, &dl->primitives, sizeof(dl->primitives), 0);
    Gpu::upload_buffer(dl->index_buffer, dl->verts, dl->vert_count * sizeof(u32), 0);
  }
  Gpu::bind_shader_buffer_data(dl->shader_args, dl->primitive_buffer, 0, 0);
  for (i32 i = 0; i < dl->texture_count; i++) {
    Gpu::bind_shader_buffer_texture(dl->shader_args, dl->textures[i], 1, i);
  }

  // bind_shader_args()
  {
    gpu->render_command_encoder->setVertexBuffer(dl->shader_args.buffer.mtl_buffer, 0, 0);
    gpu->render_command_encoder->setFragmentBuffer(dl->shader_args.buffer.mtl_buffer, 0,
                                                   0);
    gpu->render_command_encoder->useResource(
        dl->primitive_buffer.mtl_buffer, MTL::ResourceUsageRead,
        MTL::RenderStageFragment | MTL::RenderStageVertex);

    if (dl->texture_count > 0) {
      gpu->render_command_encoder->useResource(
          dl->textures[0].mtl_texture, MTL::ResourceUsageRead, MTL::RenderStageFragment);
    }
  }

  Gpu::bind_pipeline(gpu, dl->pipeline);
  for (i32 z = dl->max_z; z >= 0; z--) {
    for (i32 i = 0; i < dl->draw_calls.size; i++) {
      DrawCall call = dl->draw_calls[i];
      if (call.z == z) {
        Gpu::draw_indexed(gpu, dl->index_buffer, call.vert_offset, call.tri_count * 3);
      }
    }
  }
}

// returns next position
Vec2f draw_char(Draw::List *dl, Font &font, Color color, u32 character, Vec2f pos)
{
  if (character >= font.glyphs_zero.size) {
    character = 0;
  }

  Glyph glyph = font.glyphs_zero[character];

  Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height - glyph.bearing.y,
                       glyph.size.x, glyph.size.y};
  Vec4f uv_bounds   = {
      font.glyphs_zero[character].uv.x, font.glyphs_zero[character].uv.y,
      font.glyphs_zero[character].uv.x + font.glyphs_zero[character].uv.width,
      font.glyphs_zero[character].uv.y + font.glyphs_zero[character].uv.height};
  push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);
  pos.x += glyph.advance.x;
  return pos;
}

// returns next position
Vec2f draw_string(Draw::List *dl, Font &font, Color color, String string, Vec2f pos)
{
  for (i32 i = 0; i < string.size; i++) {
    pos = draw_char(dl, font, color, string[i], pos);
  }
  return pos;
}

}  // namespace Draw

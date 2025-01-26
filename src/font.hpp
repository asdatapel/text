#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>
#include "freetype/ftimage.h"
#include "freetype/fttypes.h"
#include "memory.hpp"
#include FT_FREETYPE_H

#include "file.hpp"
#include "image.hpp"
#include "logging.hpp"
#include "platform.hpp"
#include "types.hpp"

const i32 NUM_CHARS_IN_FONT = 128;

FT_Library library;

struct Glyph {
  Rect4f uv;
  Vec2f size;
  Vec2f bearing;
  Vec2f advance;
};

struct Font {
  Array<Glyph, 256> glyphs_zero;
  Array<Glyph, 256> glyphs_one;
  Array<Glyph, 256> glyphs_two;
  Image bitmap;
  Vec2i bitmap_cursor = {1, 1};
  i32 bitmap_next_y   = 0;

  f32 size;
  f32 ascent;
  f32 descent;
  f32 height;

  i32 char_buffer_offset = 0;

  f32 get_text_width(String text, f32 scale = 1.f)
  {
    f32 width = 0;
    for (i32 i = 0; i < text.size; i++) {
      Glyph g = glyphs_zero[text[i]];
      width += g.advance.x;
    }

    return width * scale;
  }

  i32 char_index_at_pos(String text, Vec2f text_pos, Vec2f pos, f32 scale = 1.f)
  {
    f32 cursor_x = text_pos.x;
    for (i32 i = 0; i < text.size; i++) {
      Glyph g = glyphs_zero[text[i]];

      if (cursor_x + (g.advance.x * scale / 2.f) > pos.x) return i;

      cursor_x += g.advance.x * scale;
    }

    return text.size;
  }
};

void rasterize_glyph(Font *font, FT_Face face, u32 character, Glyph *glyph)
{
  u32 usable_width = font->bitmap.width - 2;
  u32 usable_height = font->bitmap.height - 2;

  Rect4<i32> target = {
      font->bitmap_cursor.x,
      font->bitmap_cursor.y,
      (i32)face->glyph->bitmap.width,
      (i32)face->glyph->bitmap.rows,
  };
  if (target.x + target.width >= usable_width) {
    target.x = 0;
    target.y = font->bitmap_next_y;
  }

  font->bitmap_cursor = {
      target.x + target.width + 2,
      target.y,
  };
  font->bitmap_next_y = std::max(font->bitmap_next_y, target.y + target.height + 2);

  for (i32 y = 0; y < face->glyph->bitmap.rows; y++) {
    for (i32 x = 0; x < face->glyph->bitmap.width; x++) {
      u8 value = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
      font->bitmap.data()[(target.y + y) * font->bitmap.width + (target.x + x)] = value;
    }
  }

  glyph->uv = {
      (f32)target.x / font->bitmap.width,
      (f32)target.y / font->bitmap.height,
      (f32)target.width / font->bitmap.width,
      (f32)target.height / font->bitmap.height,
  };
}

Array<Glyph, 3> extract_glyph(Font *font, FT_Face face, u32 character)
{
  FT_Vector offset_zero = {0, 0};
  FT_Vector offset_one  = {64 / 3, 0};
  FT_Vector offset_two  = {2 * 64 / 3, 0};

  u32 glyph_index = FT_Get_Char_Index(face, character);

  FT_Set_Transform(face, NULL, &offset_zero);
  FT_Error err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (err) {
    fatal("failed to load glyph");
  }

  FT_Outline outline = face->glyph->outline;

  Glyph glyph;
  glyph.size = Vec2f(
      (f32)face->glyph->metrics.width  / 64.f,
      (f32)face->glyph->metrics.height / 64.f
  );
  glyph.bearing = Vec2f(
      (f32)face->glyph->metrics.horiBearingX / 64.f,
      (f32)face->glyph->metrics.horiBearingY / 64.f
  );
  glyph.advance.x = (f32)face->glyph->advance.x / 64.f;
  glyph.advance.y = (f32)face->glyph->advance.y / 64.f;

  err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (err) {
    fatal("failed to render glyph");
  }
  rasterize_glyph(font, face, character, &glyph);

  Glyph glyph_one = glyph;
  FT_Set_Transform(face, NULL, &offset_one);
  err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (err) {
    fatal("failed to load glyph");
  }
  err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (err) {
    fatal("failed to render glyph");
  }
  rasterize_glyph(font, face, character, &glyph_one);

  Glyph glyph_two = glyph;
  FT_Set_Transform(face, NULL, &offset_two);
  err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (err) {
    fatal("failed to load glyph");
  }
  err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (err) {
    fatal("failed to render glyph");
  }
  rasterize_glyph(font, face, character, &glyph_two);

  return {glyph, glyph_one, glyph_two};
}

Font load_font(String filename, f32 size)
{
  FT_Error err = FT_Init_FreeType(&library);
  if (err) {
    fatal("failed to init freetype");
  }

  Temp temp;
  File file;
  if (!read_file(filename, &temp, &file)) {
    fatal("failed to read font file?");
  }

  FT_Face face;
  err = FT_New_Memory_Face(library, file.data.data, file.data.size, 0, &face);

  err = FT_Set_Pixel_Sizes(face, 0, size);
  if (err) {
    fatal("failed to set pixel size?");
  }

  if (err) {
    fatal("failed to load font");
  }

  Font font;
  font.size    = size;
  font.ascent  = (f32)face->size->metrics.ascender / 64.f;
  font.descent = (f32)face->size->metrics.descender / 64.f;
  font.height  = (f32)face->size->metrics.height / 64.f;
  font.bitmap  = Image(1024, 1024, 1, &system_allocator);
  for (i32 i = 0; i < 128; i++) {
    Array<Glyph, 3> glyphs = extract_glyph(&font, face, i);
    font.glyphs_zero.push_back(glyphs[0]);
    font.glyphs_one.push_back(glyphs[1]);
    font.glyphs_two.push_back(glyphs[2]);
  }
  return font;
}

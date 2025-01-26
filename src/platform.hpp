#pragma once

#include <cstring>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "containers/dynamic_array.hpp"
#include "input.hpp"
#include "memory.hpp"
#include "string.hpp"
#include "types.hpp"

namespace Platform
{

enum struct CursorShape {
  NORMAL,
  HORIZ_RESIZE,
  VERT_RESIZE,
  NWSE_RESIZE,
  SWNE_RESIZE,
  IBEAM,
  POINTING_HAND,
  COUNT,
};

void init()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

struct VulkanExtensions {
  u32 count = 0;
  const char **extensions;
};

struct GlfwWindow {
  GLFWwindow *ref = nullptr;

  Vec2f size;

  GLFWcursor *cursors[(i32)CursorShape::COUNT] = {};

  void init()
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    ref = glfwCreateWindow(1920, 1080, "DUI Demo", nullptr, nullptr);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

    cursors[(i32)CursorShape::NORMAL] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    cursors[(i32)CursorShape::HORIZ_RESIZE] =
        glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    cursors[(i32)CursorShape::VERT_RESIZE] =
        glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    cursors[(i32)CursorShape::NWSE_RESIZE] =
        glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    cursors[(i32)CursorShape::SWNE_RESIZE] =
        glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    cursors[(i32)CursorShape::IBEAM] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    cursors[(i32)CursorShape::POINTING_HAND] =
        glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
  }

  VulkanExtensions get_vulkan_extensions()
  {
    VulkanExtensions vulkan_extensions;
    vulkan_extensions.extensions =
        glfwGetRequiredInstanceExtensions(&vulkan_extensions.count);
    return vulkan_extensions;
  }

  Vec2f get_size()
  {
    Vec2i new_size;
    glfwGetFramebufferSize(ref, &new_size.x, &new_size.y);

    // glfw says windows size is 0 when its minimized
    if (new_size.x != 0 && new_size.y != 0) {
      Vec2f scale;
      glfwGetWindowContentScale(ref, &scale.x, &scale.y);
      size = new_size;
    }

    return size;
  }

  b8 should_close() { return glfwWindowShouldClose(ref); }

  void destroy()
  {
    glfwDestroyWindow(ref);

    glfwTerminate();
  }

  void set_cursor_shape(CursorShape shape) { glfwSetCursor(ref, cursors[(i32)shape]); }
};

void fill_input(GlfwWindow *window, Input *state)
{
  // reset per frame data
  for (int i = 0; i < (int)Key::COUNT; i++) {
    state->key_down_events[i] = false;
    state->key_up_events[i]   = false;
  }
  for (int i = 0; i < (int)MouseButton::COUNT; i++) {
    state->mouse_button_down_events[i] = false;
    state->mouse_button_up_events[i]   = false;
  }
  state->text_inputs       = {};
  state->key_inputs        = {};
  state->scrollwheel_count = 0;

  f64 mouse_x;
  f64 mouse_y;
  glfwGetCursorPos(window->ref, &mouse_x, &mouse_y);
  state->mouse_pos_prev = state->mouse_pos;
  state->mouse_pos = {(f32)mouse_x * 2, (f32)mouse_y * 2};  // TODO use scaling factor
  state->mouse_pos_delta = state->mouse_pos - state->mouse_pos_prev;

  glfwPollEvents();
}

void character_input_callback(GLFWwindow *window, u32 codepoint)
{
  Input *input = static_cast<Input *>(glfwGetWindowUserPointer(window));

  if (codepoint < 256)  // only doing ASCII
  {
    input->text_inputs.push_back(codepoint);
  }
}

void key_input_callback(GLFWwindow *window, i32 key, i32 scancode, i32 action, i32 mods)
{
  Input *input = static_cast<Input *>(glfwGetWindowUserPointer(window));

  Modifiers modifiers = {
      static_cast<bool>(mods & (GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK)),
      static_cast<bool>(mods & GLFW_MOD_CONTROL),
      static_cast<bool>(mods & GLFW_MOD_ALT),
      static_cast<bool>(mods & GLFW_MOD_SUPER),
  };

  auto append_if_press = [&](Key k, bool text_key = false) {
    input->keys[(i32)k] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      input->key_inputs.push_back({k, modifiers, text_key});
      input->key_down_events[(i32)k] = true;
    } else {
      input->key_up_events[(i32)k] = true;
    }
  };

  auto append_if_press_without_modifiers = [&](Key k) {
    modifiers = {};
    append_if_press(k);
  };

  auto append_if_press_text_key = [&](Key unshifted, Key shifted) {
    if (mods & (GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK)) {
      modifiers.set_shift(false);
      append_if_press(shifted, true);
    } else {
      append_if_press(unshifted, true);
    }
  };

  switch (key) {
    case GLFW_KEY_TAB:
      append_if_press(Key::TAB);
      break;
    case GLFW_KEY_SPACE:
      append_if_press_text_key(Key::SPACE, Key::SPACE);
      break;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
      append_if_press(Key::ENTER);
      break;
    case GLFW_KEY_BACKSPACE:
      append_if_press(Key::BACKSPACE);
      break;
    case GLFW_KEY_DELETE:
      append_if_press(Key::DEL);
      break;

    case GLFW_KEY_0:
    case GLFW_KEY_KP_0:
      append_if_press_text_key(Key::NUM_0, Key::RIGHT_PARENTHESIS);
      break;
    case GLFW_KEY_1:
    case GLFW_KEY_KP_1:
      append_if_press_text_key(Key::NUM_1, Key::EXCLAMATION_MARK);
      break;
    case GLFW_KEY_2:
    case GLFW_KEY_KP_2:
      append_if_press_text_key(Key::NUM_2, Key::AT);
      break;
    case GLFW_KEY_3:
    case GLFW_KEY_KP_3:
      append_if_press_text_key(Key::NUM_3, Key::HASH);
      break;
    case GLFW_KEY_4:
    case GLFW_KEY_KP_4:
      append_if_press_text_key(Key::NUM_4, Key::DOLLAR_SIGN);
      break;
    case GLFW_KEY_5:
    case GLFW_KEY_KP_5:
      append_if_press_text_key(Key::NUM_5, Key::PERCENT);
      break;
    case GLFW_KEY_6:
    case GLFW_KEY_KP_6:
      append_if_press_text_key(Key::NUM_6, Key::CARET);
      break;
    case GLFW_KEY_7:
    case GLFW_KEY_KP_7:
      append_if_press_text_key(Key::NUM_7, Key::AMPERSAND);
      break;
    case GLFW_KEY_8:
    case GLFW_KEY_KP_8:
      append_if_press_text_key(Key::NUM_8, Key::STAR);
      break;
    case GLFW_KEY_9:
    case GLFW_KEY_KP_9:
      append_if_press_text_key(Key::NUM_9, Key::LEFT_PARENTHSIS);
      break;

    case GLFW_KEY_A:
      append_if_press(Key::A, true);
      break;
    case GLFW_KEY_B:
      append_if_press(Key::B, true);
      break;
    case GLFW_KEY_C:
      append_if_press(Key::C, true);
      break;
    case GLFW_KEY_D:
      append_if_press(Key::D, true);
      break;
    case GLFW_KEY_E:
      append_if_press(Key::E, true);
      break;
    case GLFW_KEY_F:
      append_if_press(Key::F, true);
      break;
    case GLFW_KEY_G:
      append_if_press(Key::G, true);
      break;
    case GLFW_KEY_H:
      append_if_press(Key::H, true);
      break;
    case GLFW_KEY_I:
      append_if_press(Key::I, true);
      break;
    case GLFW_KEY_J:
      append_if_press(Key::J, true);
      break;
    case GLFW_KEY_K:
      append_if_press(Key::K, true);
      break;
    case GLFW_KEY_L:
      append_if_press(Key::L, true);
      break;
    case GLFW_KEY_M:
      append_if_press(Key::M, true);
      break;
    case GLFW_KEY_N:
      append_if_press(Key::N, true);
      break;
    case GLFW_KEY_O:
      append_if_press(Key::O, true);
      break;
    case GLFW_KEY_P:
      append_if_press(Key::P, true);
      break;
    case GLFW_KEY_Q:
      append_if_press(Key::Q, true);
      break;
    case GLFW_KEY_R:
      append_if_press(Key::R, true);
      break;
    case GLFW_KEY_S:
      append_if_press(Key::S, true);
      break;
    case GLFW_KEY_T:
      append_if_press(Key::T, true);
      break;
    case GLFW_KEY_U:
      append_if_press(Key::U, true);
      break;
    case GLFW_KEY_V:
      append_if_press(Key::V, true);
      break;
    case GLFW_KEY_W:
      append_if_press(Key::W, true);
      break;
    case GLFW_KEY_X:
      append_if_press(Key::X, true);
      break;
    case GLFW_KEY_Y:
      append_if_press(Key::Y, true);
      break;
    case GLFW_KEY_Z:
      append_if_press(Key::Z, true);
      break;

    case GLFW_KEY_GRAVE_ACCENT:
      append_if_press_text_key(Key::BACKTICK, Key::TILDE);
      break;
    case GLFW_KEY_MINUS:
      append_if_press_text_key(Key::DASH, Key::UNDERSCORE);
      break;
    case GLFW_KEY_EQUAL:
      append_if_press_text_key(Key::EQUAL, Key::PLUS);
      break;
    case GLFW_KEY_LEFT_BRACKET:
      append_if_press_text_key(Key::LEFT_BRACKET, Key::LEFT_CURLY_BRACE);
      break;
    case GLFW_KEY_RIGHT_BRACKET:
      append_if_press_text_key(Key::RIGHT_BRACKET, Key::RIGHT_CURLY_BRACE);
      break;
    case GLFW_KEY_SLASH:
      append_if_press_text_key(Key::SLASH, Key::QUESTION_MARK);
      break;
    case GLFW_KEY_COMMA:
      append_if_press_text_key(Key::COMMA, Key::LESS_THAN);
      break;
    case GLFW_KEY_PERIOD:
      append_if_press_text_key(Key::PERIOD, Key::GREATER_THAN);
      break;
    case GLFW_KEY_BACKSLASH:
      append_if_press_text_key(Key::BACKSLASH, Key::PIPE);
      break;

    case GLFW_KEY_UP:
      append_if_press(Key::UP);
      break;
    case GLFW_KEY_DOWN:
      append_if_press(Key::DOWN);
      break;
    case GLFW_KEY_LEFT:
      append_if_press(Key::LEFT);
      break;
    case GLFW_KEY_RIGHT:
      append_if_press(Key::RIGHT);
      break;

    case GLFW_KEY_F1:
      append_if_press(Key::F1);
      break;
    case GLFW_KEY_F2:
      append_if_press(Key::F2);
      break;
    case GLFW_KEY_F3:
      append_if_press(Key::F3);
      break;
    case GLFW_KEY_F4:
      append_if_press(Key::F4);
      break;
    case GLFW_KEY_F5:
      append_if_press(Key::F5);
      break;
    case GLFW_KEY_F6:
      append_if_press(Key::F6);
      break;
    case GLFW_KEY_F7:
      append_if_press(Key::F7);
      break;
    case GLFW_KEY_F8:
      append_if_press(Key::F8);
      break;
    case GLFW_KEY_F9:
      append_if_press(Key::F9);
      break;
    case GLFW_KEY_F10:
      append_if_press(Key::F10);
      break;
    case GLFW_KEY_F11:
      append_if_press(Key::F11);
      break;
    case GLFW_KEY_F12:
      append_if_press(Key::F12);
      break;

    case GLFW_KEY_CAPS_LOCK:
      append_if_press_without_modifiers(Key::CAPS_LOCK);
      break;
    case GLFW_KEY_LEFT_SHIFT:
      append_if_press_without_modifiers(Key::LSHIFT);
      break;
    case GLFW_KEY_RIGHT_SHIFT:
      append_if_press_without_modifiers(Key::RSHIFT);
      break;
    case GLFW_KEY_LEFT_CONTROL:
      append_if_press_without_modifiers(Key::LCTRL);
      break;
    case GLFW_KEY_RIGHT_CONTROL:
      append_if_press_without_modifiers(Key::RCTRL);
      break;
    case GLFW_KEY_LEFT_SUPER:
    case GLFW_KEY_RIGHT_SUPER:
      append_if_press_without_modifiers(Key::COMMAND);
      break;
    case GLFW_KEY_LEFT_ALT:
      append_if_press_without_modifiers(Key::LALT);
      break;
    case GLFW_KEY_RIGHT_ALT:
      append_if_press_without_modifiers(Key::RALT);
      break;

    case GLFW_KEY_ESCAPE:
      append_if_press(Key::ESCAPE);
      break;
    case GLFW_KEY_INSERT:
      append_if_press(Key::INS);
      break;
    case GLFW_KEY_HOME:
      append_if_press(Key::HOME);
      break;
    case GLFW_KEY_END:
      append_if_press(Key::END);
      break;
    case GLFW_KEY_PAGE_UP:
      append_if_press(Key::PAGEUP);
      break;
    case GLFW_KEY_PAGE_DOWN:
      append_if_press(Key::PAGEDOWN);
      break;
  }
}

void mouse_button_callback(GLFWwindow *window, i32 button, i32 action, i32 mods)
{
  Input *input = static_cast<Input *>(glfwGetWindowUserPointer(window));

  MouseButton mouse_button;
  switch (button) {
    case (GLFW_MOUSE_BUTTON_LEFT): {
      mouse_button = MouseButton::LEFT;
    } break;
    case (GLFW_MOUSE_BUTTON_RIGHT): {
      mouse_button = MouseButton::RIGHT;
    } break;
    case (GLFW_MOUSE_BUTTON_MIDDLE): {
      mouse_button = MouseButton::MIDDLE;
    } break;
    default:
      return;
  }

  input->mouse_buttons[(i32)mouse_button] = action == GLFW_PRESS;

  if (action == GLFW_PRESS)
    input->mouse_button_down_events[(i32)mouse_button] = true;
  else
    input->mouse_button_up_events[(i32)mouse_button] = true;
}

void scroll_callback(GLFWwindow *window, f64 x_offset, f64 y_offset)
{
  Input *input = static_cast<Input *>(glfwGetWindowUserPointer(window));

  input->scrollwheel_count += y_offset;
}

void setup_input_callbacks(GlfwWindow *window, Input *input)
{
  glfwSetWindowUserPointer(window->ref, input);
  glfwSetCharCallback(window->ref, character_input_callback);
  glfwSetKeyCallback(window->ref, key_input_callback);
  glfwSetMouseButtonCallback(window->ref, mouse_button_callback);
  glfwSetScrollCallback(window->ref, scroll_callback);
}

DynamicArray<String> list_files(String root, StackAllocator *alloc)
{
  DynamicArray<String> files(alloc);
  files.set_capacity(128);

  std::filesystem::path root_path = std::string((char *)root.data, root.size);
  for (const auto &entry : std::filesystem::recursive_directory_iterator(root_path)) {
    if (entry.is_directory()) {
      continue;
    }

    String filename;
    filename.size = entry.path().lexically_normal().string().size();
    filename.data = alloc->alloc(filename.size).data;
    memcpy(filename.data, entry.path().lexically_normal().string().data(), filename.size);

    files.push_back(filename);
  }

  return files;
}

GlfwWindow *global_window_for_clipboard_access = nullptr;
void set_clipboard(String str)
{
  glfwSetClipboardString(global_window_for_clipboard_access->ref,
                         str.c_str(&tmp_allocator));
}
String get_clipboard()
{
  const char *str = glfwGetClipboardString(global_window_for_clipboard_access->ref);
  

  return {(u8 *)str, str ? (u32)strlen(str) : 0};
}
}  // namespace Platform

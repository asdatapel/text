#include "actions.hpp"
#include "buffer.hpp"
#include "debug_window.hpp"
#include "draw.hpp"
#include "gpu/gpu.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/render_target.hpp"
#include "gpu/metal/texture.hpp"
#include "math/math.hpp"
#include "menu.hpp"
#include "platform.hpp"
#include "status_bar.hpp"
#include "tester.hpp"
#include "types.hpp"
#include "window.hpp"

constexpr bool ENABLE_METAL_CAPTURE = false;

namespace Five
{

struct WindowManager {
  Array<Window, 32> windows;
  Menu menu;

  i32 focused = 0;
  Mode mode   = Mode::INSERT;

  Mode get_focused_mode()
  {
    if (menu.open) {
      return Mode::INSERT;
    }
    return mode;
  }

  Window *get_focused_window() { return &windows[focused]; }

  void handle_action(Action action, Draw::List *dl)
  {
    if (menu.open) {
      ::Five::handle_action(&menu, action);
      return;
    }

    if (action == Command::BUFFER_CHANGE_MODE) {
      static i32 i = 0;
      i++;
      if (mode == Mode::INSERT) {
        mode = Mode::NORMAL;
      } else if (mode == Mode::NORMAL) {
        mode = Mode::INSERT;
      }

      return;
    }
    if (mode != Mode::INSERT) {
      if (action == Command::INPUT_NEWLINE || action == Command::INPUT_TAB ||
          action == Command::INPUT_BACKSPACE || action == Command::INPUT_TEXT) {
        return;
      }
    }

    if (action == Command::WINDOW_SWITCH_LEFT) {
      focused = std::max(focused - 1, 0);
    }
    if (action == Command::WINDOW_SWITCH_RIGHT) {
      focused = std::min(focused + 1, (i32)windows.size - 1);
    }

    if (action == Command::MOUSE_LEFT_CLICK || action == Command::MOUSE_RIGHT_CLICK) {
      for (i32 i = 0; i < windows.size; i++) {
        if (in_rect(action.mouse_position, windows[i].rect)) {
          focused = i;
          break;
        }
      }
    }

    ::Five::handle_action(&windows[focused], action, dl);
    return;
  }

  void add_window(Window window) { windows.push_back(window); }
};

int mymain()
{
  test_rope();
  rope_buffer_tests();

  Input input;
  Chord chord;
  Actions actions;
  WindowManager window_manager;

  Platform::init();
  Platform::GlfwWindow sys_window;
  sys_window.init();
  Platform::setup_input_callbacks(&sys_window, &input);
  Platform::global_window_for_clipboard_access = &sys_window;

  Gpu::Device *device = Gpu::init(&sys_window);

  Draw::List dl;
  Draw::init_draw_system(&dl, device);

  Vec2f canvas_size = sys_window.get_size();
  Vec2f window_size = {canvas_size.x / 2, canvas_size.y - 40};
  window_manager.add_window(init_window({0, 0, window_size.x, window_size.y}));
  window_manager.add_window(
      init_window({window_size.x, 0, window_size.x, window_size.y}));

  Gpu::Texture target_tex = Gpu::create_render_target_texture(
      device, window_size.x, window_size.y, PixelFormat::RGBA8U);
  Gpu::RenderTarget target =
      Gpu::create_render_target(device, target_tex, std::nullopt, Color(34, 36, 43, 1));

  RopeBuffer *buffer = buffer_manager.get_or_open_buffer("test.txt");
  open_editor(&window_manager.windows[0], buffer);
  Tester tester = create_tester("resources/test/tiny.txt");

  i64 frame = 0;
  while (!sys_window.should_close()) {
    Platform::fill_input(&sys_window, &input);
    process_input(&input, &actions, &chord, window_manager.get_focused_mode());
    add_actions(&tester, window_manager.windows[0].active_editor, &actions);

    // i32 asd = 0;
    // while (actions.size > 0 && asd < 100000) {
    //   for (i32 i = 0; i < actions.size; i++) {
    //     if (actions[i] == Command::MENU_QUICK_OPEN) {
    //       window_manager.menu.open = true;
    //       continue;
    //     }

    //     window_manager.handle_action(actions[i], &dl);
    //   }

    //   actions.clear();
    //   add_actions(&tester, window_manager.windows[0].active_editor, &actions);
    //   asd++;
    // }

    for (i32 i = 0; i < actions.size; i++) {
      if (actions[i] == Command::MENU_QUICK_OPEN) {
        window_manager.menu.open = true;
        continue;
      }

      window_manager.handle_action(actions[i], &dl);
    }

    // if constexpr (ENABLE_METAL_CAPTURE) {
    //   if (capture == 0) {
    //     MTL::CaptureManager::sharedCaptureManager()->startCapture(device->metal_device);
    //   }
    // }

    Gpu::start_frame(device);
    Gpu::start_backbuffer(device, Color(40, 44, 52));

    Draw::start_frame(&dl, sys_window.get_size());

    for (i32 i = 0; i < window_manager.windows.size; i++) {
      bool is_focused = window_manager.focused == i;
      draw_window(window_manager.windows[i], &dl, is_focused);
      draw_status_bar(&dl, &chord, window_manager.get_focused_mode());
    }

    if (window_manager.windows[0].active_editor) {
      static DebugWindow debug_window =
          init_debug_window(&window_manager.windows[0].active_editor->buffer);
      debug_window.scroll += input.scrollwheel_count;
      debug_window.mouse_position = input.mouse_pos;
      draw(&debug_window, &dl, window_manager.windows[1].content_rect);
    }

    if (window_manager.menu.open) {
      Five::draw_filemenu(&window_manager.menu, &dl, window_manager.get_focused_window());
    }

    Draw::end_frame(&dl, device, frame);

    Gpu::end_backbuffer(device);

    Gpu::end_frame(device);

    // if constexpr (ENABLE_METAL_CAPTURE) {
    //   if (capture == 3) {
    //     MTL::CaptureManager::sharedCaptureManager()->stopCapture();
    //   }
    // }
    frame++;

    tmp_allocator.reset();
  }

  // Dui::destroy()
  // Gpu::destroy_device()

  sys_window.destroy();

  return 0;
}
}  // namespace Five

int main() { Five::mymain(); }
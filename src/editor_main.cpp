#include "actions.hpp"
#include "buffer.hpp"
#include "debug_window.hpp"
#include "draw.hpp"
#include "font_manager.hpp"
#include "gpu/gpu.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/render_target.hpp"
#include "gpu/metal/texture.hpp"
#include "math/math.hpp"
#include "menu.hpp"
#include "panes/pane_manager.hpp"
#include "platform.hpp"
#include "status_bar.hpp"
#include "tester.hpp"
#include "types.hpp"
#include "window.hpp"

constexpr bool ENABLE_METAL_CAPTURE = !true;

// struct WindowManager {
//   Array<Window, 32> windows;
//   Menu menu;

//   i32 focused = 0;
//   Mode mode   = Mode::INSERT;

//   Mode get_focused_mode()
//   {
//     if (menu.open) {
//       return Mode::INSERT;
//     }
//     return mode;
//   }

//   Window *get_focused_window() { return &windows[focused]; }

//   // void handle_action(Action action, Draw::List *dl)
//   // {
//   //   if (menu.open) {
//   //     ::handle_action(&menu, action);
//   //     return;
//   //   }

//   //   if (action == Command::BUFFER_CHANGE_MODE) {
//   //     if (mode == Mode::INSERT) {
//   //       mode = Mode::NORMAL;
//   //     } else if (mode == Mode::NORMAL) {
//   //       mode = Mode::INSERT;
//   //     }

//   //     return;
//   //   }
//   //   if (mode != Mode::INSERT) {
//   //     if (action == Command::INPUT_NEWLINE || action == Command::INPUT_TAB ||
//   //         action == Command::INPUT_BACKSPACE || action == Command::INPUT_TEXT) {
//   //       return;
//   //     }
//   //   }

//   //   if (action == Command::WINDOW_SWITCH_LEFT) {
//   //     focused = std::max(focused - 1, 0);
//   //   }
//   //   if (action == Command::WINDOW_SWITCH_RIGHT) {
//   //     focused = std::min(focused + 1, (i32)windows.size - 1);
//   //   }

//   //   if (action == Command::MOUSE_LEFT_CLICK || action == Command::MOUSE_RIGHT_CLICK)
//   {
//   //     for (i32 i = 0; i < windows.size; i++) {
//   //       if (in_rect(action.mouse_position, windows[i].rect)) {
//   //         focused = i;
//   //         break;
//   //       }
//   //     }
//   //   }

//   //   ::handle_action(&windows[focused], action, dl);
//   //   return;
//   // }

//   void add_pane(Window window) { windows.push_back(window); }
// };

// void process(WindowManager *wm, Actions *actions)
// {
//   for (i32 i = 0; i < actions->size; i++) {
//     Action *action = &actions->operator[](i);

//     Menu *menu = &wm->menu;
//     if (menu->open) {
//       ::handle_action(menu, *action);
//       return;
//     }

//     if (eat(action, Command::BUFFER_CHANGE_MODE)) {
//       if (wm->mode == Mode::INSERT) {
//         wm->mode = Mode::NORMAL;
//       } else if (wm->mode == Mode::NORMAL) {
//         wm->mode = Mode::INSERT;
//       }

//       continue;
//     }
//     if (wm->mode != Mode::INSERT) {
//       if (eat(action, Command::INPUT_NEWLINE) || eat(action, Command::INPUT_TAB) ||
//           eat(action, Command::INPUT_BACKSPACE) || eat(action, Command::INPUT_TEXT)) {
//         continue;
//       }
//     }

//     if (eat(action, Command::WINDOW_SWITCH_LEFT)) {
//       wm->focused = std::max(wm->focused - 1, 0);
//     }
//     if (eat(action, Command::WINDOW_SWITCH_RIGHT)) {
//       wm->focused = std::min(wm->focused + 1, (i32)wm->windows.size - 1);
//     }

//     if (dont_eat(action, Command::MOUSE_LEFT_CLICK) ||
//         dont_eat(action, Command::MOUSE_RIGHT_CLICK)) {
//       for (i32 i = 0; i < wm->windows.size; i++) {
//         if (in_rect(action->mouse_position, wm->windows[i].rect)) {
//           wm->focused = i;
//           break;
//         }
//       }
//     }
//   }

//   for (i32 i = 0; i < wm->windows.size; i++) {
//     ::process(&wm->windows[i], actions, wm->focused == i);
//   }
// }

int mymain()
{
  test_rope();
  rope_buffer_tests();

  Input input;
  Chord chord;
  Actions actions;

  Platform::init();
  Platform::GlfwWindow sys_window;
  sys_window.init();
  Platform::setup_input_callbacks(&sys_window, &input);
  Platform::global_window_for_clipboard_access = &sys_window;

  Gpu::Device *device = Gpu::init(&sys_window);

  init_fonts();

  Draw::List dl;
  Draw::init_draw_system(&dl, device);

  Vec2f canvas_size = sys_window.get_size();
  Vec2f window_size = {canvas_size.x / 2, canvas_size.y - 40};
  pm.add_pane(init_pane({0, 0, window_size.x, window_size.y}));
  pm.add_pane(init_pane({window_size.x, 0, window_size.x, window_size.y}));

  Gpu::Texture target_tex = Gpu::create_render_target_texture(
      device, window_size.x, window_size.y, PixelFormat::RGBA8U);
  Gpu::RenderTarget target = Gpu::create_render_target(device, target_tex, std::nullopt,
                                                       settings.background_color);

  RopeBuffer *buffer = buffer_manager.get_or_open_buffer("test.txt");
  create_or_open_editor_tab(&pm.panes[0], buffer);
  Tester tester = create_tester("resources/test/tiny.txt");

  i64 frame = 0;
  while (!sys_window.should_close()) {
    Platform::fill_input(&sys_window, &input);
    process_input(&input, &actions, &chord);
    // add_actions(&tester, pm.windows[0].active_editor, &actions);

    // i32 asd = 0;
    // while (actions.size > 0 && asd < 100000) {
    //   for (i32 i = 0; i < actions.size; i++) {
    //     if (actions[i] == Command::MENU_QUICK_OPEN) {
    //       pm.menu.open = true;
    //       continue;
    //     }

    //     pm.handle_action(actions[i], &dl);
    //   }

    //   actions.clear();
    //   add_actions(&tester, pm.windows[0].active_editor, &actions);
    //   asd++;
    // }

    for (i32 i = 0; i < actions.size; i++) {
      Action *action = &actions[i];
      if (eat(action, Command::MENU_QUICK_OPEN)) {
        menu.open = true;
        continue;
      }
    }
    process(&menu, &actions);
    process(&pm, &actions);

    // if constexpr (ENABLE_METAL_CAPTURE) {
    //   if (capture == 0) {
    //     MTL::CaptureManager::sharedCaptureManager()->startCapture(device->metal_device);
    //   }
    // }

    Gpu::start_frame(device);
    Gpu::start_backbuffer(device, settings.background_color);

    Draw::start_frame(&dl, sys_window.get_size());

    draw_panes(&pm, &dl);
    draw_status_bar(&dl);

    // if (pm.panes[0].active_editor) {
    //   static DebugWindow debug_window =
    //       init_debug_window(&pm.panes[0].active_editor->buffer);
    //   debug_window.scroll += input.scrollwheel_count;
    //   debug_window.mouse_position = input.mouse_pos;
    //   for (i32 i = 0; i < actions.size; i++) {
    //     handle_action(&debug_window, actions[i]);
    //   }
    //   draw(&debug_window, &dl, pm.panes[1].rect);
    // }

    draw_filemenu(&menu, &dl);

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

int main() { mymain(); }
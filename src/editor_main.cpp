#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "gpu/gpu.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/render_target.hpp"
#include "gpu/metal/texture.hpp"
#include "math/math.hpp"
#include "menu.hpp"
#include "platform.hpp"
#include "status_bar.hpp"
#include "types.hpp"
#include "window.hpp"

namespace Five
{
struct WindowManager {
  Window window;
  Menu menu;

  i32 focused = 0;

  Mode get_focused_mode()
  {
    if (menu.open) {
      return menu.mode;
    }

    return window.mode;
  }

  void handle_action(Action action)
  {
    if (menu.open) {
      return ::Five::handle_action(&menu, action);
    }

    return ::Five::handle_action(&window, action);
  }

  void add_window(Window window) { this->window = window; }
};

int mymain()
{
  Input input;
  Chord chord;
  Actions actions;
  BasicBuffer buffer = load_buffer("./blah.txt");
  WindowManager window_manager;

  Platform::init();
  Platform::GlfwWindow sys_window;
  sys_window.init();
  Platform::setup_input_callbacks(&sys_window, &input);

  Gpu::Device *device = Gpu::init(&sys_window);

  Draw::List dl;
  Draw::init_draw_system(&dl, device);

  Vec2f window_size = sys_window.get_size();
  {
    Window window = init_window(device, &dl, &buffer, window_size);
    set_window_buffer(&window, &buffer);
    window_manager.add_window(window);
  }

  Gpu::Texture target_tex = Gpu::create_render_target_texture(
      device, window_size.x, window_size.y, PixelFormat::RGBA8U);
  Gpu::RenderTarget target =
      Gpu::create_render_target(device, target_tex, std::nullopt, Color(34, 36, 43, 1));

  i32 capture = 0;
  while (!sys_window.should_close()) {
    Platform::fill_input(&sys_window, &input);
    process_input(&input, &actions, &chord, window_manager.get_focused_mode());

    for (i32 i = 0; i < actions.size; i++) {
      if (actions[i] == Command::MENU_QUICK_OPEN) {
        window_manager.menu.open = true;
        continue;
      }

      window_manager.handle_action(actions[i]);
    }

    if (capture == 0) {
      // MTL::CaptureManager::sharedCaptureManager()->startCapture(device->metal_device);
    }

    Gpu::start_frame(device);
    Gpu::start_backbuffer(device, Color(40, 44, 52));

    Draw::start_frame(&dl, sys_window.get_size());
    draw_window(window_manager.window, device, &dl);
    draw_status_bar(&dl, window_manager.get_focused_mode());
    if (window_manager.menu.open) {
      Five::draw_filemenu(&window_manager.menu, &dl, &window_manager.window);
    }
    Draw::end_frame(&dl, device, capture);

    Gpu::end_backbuffer(device);

    Gpu::end_frame(device);

    if (capture == 3) {
      // MTL::CaptureManager::sharedCaptureManager()->stopCapture();
    }
    capture++;

    tmp_allocator.reset();
  }

  // Dui::destroy()
  // Gpu::destroy_device()

  sys_window.destroy();

  return 0;
}
}  // namespace Five

int main() { Five::mymain(); }
#include <iostream>

#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "gpu/gpu.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/render_target.hpp"
#include "gpu/metal/texture.hpp"
#include "math/math.hpp"
#include "platform.hpp"
#include "status_bar.hpp"
#include "types.hpp"
#include "window.hpp"

int main()
{
  Input input;
  Commander commander;
  Commands commands;

  Platform::init();
  Platform::GlfwWindow sys_window;
  sys_window.init();
  Platform::setup_input_callbacks(&sys_window, &input);

  Gpu::Device *device = Gpu::init(&sys_window);

  i32 capture = 0;

  Draw::DrawList dl;
  Draw::init_draw_system(&dl, device);
  BasicBuffer buffer = load_buffer("./src/editor_main.cpp");

  Vec2f window_size = sys_window.get_size();
  Window window = init_window(device, &dl, &buffer, window_size.x, window_size.y);
  set_window_buffer(&window, &buffer);

  Gpu::Texture target_tex = Gpu::create_render_target_texture(device, window_size.x, window_size.y, PixelFormat::RGBA8U);
  Gpu::RenderTarget target = Gpu::create_render_target(device, target_tex, std::nullopt, Color(34, 36, 43, 1));

  while (!sys_window.should_close()) {
    Platform::fill_input(&sys_window, &input);
    process_input(&commander, &input, &commands);
    handle_input(&window, &sys_window, &input, &commands, &commander);

    if (capture == 0) {
      // MTL::CaptureManager::sharedCaptureManager()->startCapture(device->metal_device);
    }

    Gpu::start_frame(device);
    Gpu::start_backbuffer(device, Color(40, 44, 52));

    Draw::start_frame(&dl);
    draw_window(window, device, &dl);
    draw_status_bar(&dl);
    Draw::end_frame(&dl, device, sys_window.get_size(), capture);

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

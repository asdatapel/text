#pragma once

#include "containers/array.hpp"
#include "input.hpp"
#include "logging.hpp"

enum struct Mode {
  NORMAL,
  INSERT,
};
enum struct Command {
  BUFFER_COPY,
  BUFFER_PASTE,
  BUFFER_PLACE_ANCHOR,

  NAV_CHAR_LEFT,
  NAV_CHAR_RIGHT,
  NAV_LINE_UP,
  NAV_LINE_DOWN,
  NAV_PARAGRAPH_UP,
  NAV_PARAGRAPH_DOWN,
  NAV_WORD_LEFT,
  NAV_WORD_RIGHT,
  NAV_JUMP_TO_ANCHOR,

  WINDOW_SCROLL_UP,
  WINDOW_SCROLL_DOWN,
};
typedef Array<Command, 128> Commands;

struct Commander {
    Mode mode = Mode::NORMAL;
    Array<Key, 16> chord;
};

void process_input(Commander *commander, Input *input, Commands *commands)
{
  commands->clear();

  for (i32 i = 0; i < input->key_input.size; i++) {
    Key key   = input->key_input[i];
    bool ctrl = input->keys[(i32)Key::LCTRL] || input->keys[(i32)Key::RCTRL];

    if (key == Key::LALT || key == Key::RALT) {
        switch(commander->mode) {
            case Mode::NORMAL:
                commander->mode = Mode::INSERT;
                break;
            case Mode::INSERT:
                commander->mode = Mode::NORMAL;
                break;
        }
    }

    if (commander->mode == Mode::NORMAL) {
        if (key == Key::A) {
            commands->push_back(Command::BUFFER_PLACE_ANCHOR);
        }
        if (key == Key::Y) {
            commands->push_back(Command::BUFFER_COPY);
        }
        if (key == Key::P) {
            commands->push_back(Command::BUFFER_PASTE);
        }
        if (key == Key::LEFT || key == Key::H) {
            commands->push_back(Command::NAV_CHAR_LEFT);
        }
        if (key == Key::RIGHT || key == Key::L) {
            commands->push_back(Command::NAV_CHAR_RIGHT);
        }
        if (key == Key::DOWN || key == Key::J) {
            commands->push_back(Command::NAV_LINE_DOWN);
        }
        if (key == Key::UP || key == Key::K) {
            commands->push_back(Command::NAV_LINE_UP);
        }
        if (key == Key::B) {
            commands->push_back(Command::NAV_WORD_LEFT);
        }
        if (key == Key::E) {
            commands->push_back(Command::NAV_WORD_RIGHT);
        }
    }
  }
}

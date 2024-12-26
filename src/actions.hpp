#pragma once

#include "containers/array.hpp"
#include "containers/dynamic_array.hpp"
#include "input.hpp"
#include "logging.hpp"

enum struct Mode {
  NORMAL,
  INSERT,
};
struct Focusable {
  Mode mode = Mode::INSERT;
};

enum struct Command {
  INPUT_TEXT,
  INPUT_NEWLINE,
  INPUT_TAB,
  INPUT_BACKSPACE,
  INPUT_DELETE,

  BUFFER_CHANGE_MODE,
  BUFFER_SAVE,
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

  MENU_QUICK_OPEN,
};
struct Action {
  Command command;
  u32 character;

  Action() {}
  Action(Command command) { this->command = command; }
  Action(u32 character)
  {
    this->command   = Command::INPUT_TEXT;
    this->character = character;
  }

  bool operator==(Command command) { return this->command == command; }
};
typedef Array<Action, 128> Actions;

typedef Array<KeyInput, 8> Chord;
Array<std::pair<Chord, Command>, 1024> bindings = {
    {Chord{{Key::H}}, Command::NAV_CHAR_LEFT},
    {Chord{{Key::LEFT}}, Command::NAV_CHAR_LEFT},
    {Chord{{Key::L}}, Command::NAV_CHAR_RIGHT},
    {Chord{{Key::RIGHT}}, Command::NAV_CHAR_RIGHT},
    {Chord{{Key::J}}, Command::NAV_LINE_DOWN},
    {Chord{{Key::DOWN}}, Command::NAV_LINE_DOWN},
    {Chord{{Key::K}}, Command::NAV_LINE_UP},
    {Chord{{Key::UP}}, Command::NAV_LINE_UP},
    {Chord{{Key::B}}, Command::NAV_WORD_LEFT},
    {Chord{{Key::E}}, Command::NAV_WORD_RIGHT},

    {Chord{{Key::ENTER}}, Command::INPUT_NEWLINE},
    {Chord{{Key::TAB}}, Command::INPUT_TAB},
    {Chord{{Key::BACKSPACE}}, Command::INPUT_BACKSPACE},
    {Chord{{Key::DEL}}, Command::INPUT_DELETE},

    {Chord{{Key::SPACE}, {Key::SPACE}}, Command::MENU_QUICK_OPEN},

    {Chord{{Key::S, Modifiers{.super = true}}}, Command::BUFFER_SAVE},
    {Chord{{Key::A}}, Command::BUFFER_PLACE_ANCHOR},
    {Chord{{Key::Y}}, Command::BUFFER_COPY},
    {Chord{{Key::P}}, Command::BUFFER_PASTE},

    {Chord{{Key::LALT}}, Command::BUFFER_CHANGE_MODE},
    {Chord{{Key::RALT}}, Command::BUFFER_CHANGE_MODE},
};

bool match_chord(Chord *chord, Chord *binding, bool *any_partial_matches)
{
  bool matching = true;

  for (i32 i = 0; i < chord->size && i < binding->size; i++) {
    if ((*chord)[i] != (*binding)[i]) {
      return false;
    }
  }

  if (chord->size < binding->size) {
    *any_partial_matches |= matching;
  }

  return chord->size == binding->size && matching;
}

void process_input(Input *input, Actions *actions, Chord *chord, Mode mode)
{
  actions->clear();

  if (mode == Mode::INSERT && chord->size == 0) {
    for (i32 i = 0; i < input->text_inputs.size; i++) {
      actions->push_back(input->text_inputs[i]);
    }
  }

  for (i32 i = 0; i < input->key_inputs.size; i++) {
    KeyInput key = input->key_inputs[i];

    error("ASDASD, ", chord->size, ", ", key.text_key);
    if (key.text_key && mode == Mode::INSERT && chord->size == 0) {
      break;
    }
    error("QWEE, ", chord->size, ", ", key.text_key);

    chord->push_back(key);
  }

  bool any_partial_matches = false;
  if (chord->size > 0) {
    error("POIU, ", chord->size);
    for (i32 i = 0; i < bindings.size; i++) {
      if (match_chord(chord, &bindings[i].first, &any_partial_matches)) {
        chord->clear();
        actions->push_back(bindings[i].second);
        error("BINDING ", i);
      }
    }

    if (!any_partial_matches) {
      error("ZXC, ", chord->size);
      chord->clear();
    }
  }
}

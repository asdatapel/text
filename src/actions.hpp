#pragma once

#include "containers/array.hpp"
#include "containers/dynamic_array.hpp"
#include "global_state.hpp"
#include "input.hpp"
#include "logging.hpp"

enum struct Command {
  NONE,

  MOUSE_LEFT_CLICK,
  MOUSE_RIGHT_CLICK,
  MOUSE_SCROLL,

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
  NAV_BLOCK_UP,
  NAV_BLOCK_DOWN,

  WINDOW_SCROLL_UP,
  WINDOW_SCROLL_DOWN,
  WINDOW_SWITCH_LEFT,
  WINDOW_SWITCH_RIGHT,

  MENU_QUICK_OPEN,
  TOGGLE_FIND,
  JUMP_TO_NEXT,

  ESCAPE,
};
String command_strings[] = {
  "NONE",

  "MOUSE_LEFT_CLICK",
  "MOUSE_RIGHT_CLICK",
  "MOUSE_SCROLL",

  "INPUT_TEXT",
  "INPUT_NEWLINE",
  "INPUT_TAB",
  "INPUT_BACKSPACE",
  "INPUT_DELETE",

  "BUFFER_CHANGE_MODE",
  "BUFFER_SAVE",
  "BUFFER_COPY",
  "BUFFER_PASTE",
  "BUFFER_PLACE_ANCHOR",

  "NAV_CHAR_LEFT",
  "NAV_CHAR_RIGHT",
  "NAV_LINE_UP",
  "NAV_LINE_DOWN",
  "NAV_PARAGRAPH_UP",
  "NAV_PARAGRAPH_DOWN",
  "NAV_WORD_LEFT",
  "NAV_WORD_RIGHT",
  "NAV_JUMP_TO_ANCHOR",
  "NAV_BLOCK_UP",
  "NAV_BLOCK_DOWN",

  "WINDOW_SCROLL_UP",
  "WINDOW_SCROLL_DOWN",
  "WINDOW_SWITCH_LEFT",
  "WINDOW_SWITCH_RIGHT",

  "MENU_QUICK_OPEN",
  "TOGGLE_FIND",
  "JUMP_TO_NEXT",

  "ESCAPE",
};

struct Action {
  Command command;

  u32 character;
  Vec2f mouse_position;
  f64 scrollwheel_delta;

  bool eaten = false;

  Action() {}
  Action(Command command) { this->command = command; }
  Action(u32 character)
  {
    this->command   = Command::INPUT_TEXT;
    this->character = character;
  }

  bool operator==(Command command) { return this->command == command; }
};
typedef Array<Action, 64> Actions;

typedef Array<KeyInput, 8> Chord;
Array<std::pair<Chord, Command>, 1024> bindings = {
    {Chord{{Key::ENTER}}, Command::INPUT_NEWLINE},
    {Chord{{Key::TAB}}, Command::INPUT_TAB},
    {Chord{{Key::BACKSPACE}}, Command::INPUT_BACKSPACE},
    {Chord{{Key::DEL}}, Command::INPUT_DELETE},

    {Chord{{Key::S, Modifiers::with_super()}}, Command::BUFFER_SAVE},
    {Chord{{Key::SPACE}, {Key::B}, {Key::S}}, Command::BUFFER_SAVE},
    {Chord{{Key::A}}, Command::BUFFER_PLACE_ANCHOR},
    {Chord{{Key::Y}}, Command::BUFFER_COPY},
    {Chord{{Key::P}}, Command::BUFFER_PASTE},
    {Chord{{Key::LALT}}, Command::BUFFER_CHANGE_MODE},
    {Chord{{Key::RALT}}, Command::BUFFER_CHANGE_MODE},

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
    {Chord{{Key::LEFT_CURLY_BRACE}}, Command::NAV_BLOCK_UP},
    {Chord{{Key::RIGHT_CURLY_BRACE}}, Command::NAV_BLOCK_DOWN},

    {Chord{{Key::SPACE}, {Key::W}, {Key::H}}, Command::WINDOW_SWITCH_LEFT},
    {Chord{{Key::SPACE}, {Key::W}, {Key::L}}, Command::WINDOW_SWITCH_RIGHT},

    {Chord{{Key::SPACE}, {Key::SPACE}}, Command::MENU_QUICK_OPEN},
    {Chord{{Key::F}}, Command::TOGGLE_FIND},
    {Chord{{Key::E}}, Command::JUMP_TO_NEXT},

    {Chord{{Key::G}}, Command::ESCAPE},
    {Chord{{Key::G, Modifiers::with_ctrl()}}, Command::ESCAPE},
    {Chord{{Key::ESCAPE}}, Command::ESCAPE},
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


void show_partial_chord(Chord *chord);
void show_completed_chord(Chord *chord, Command command);
void process_input(Input *input, Actions *actions, Chord *chord)
{
  actions->clear();

  if (mode == Mode::INSERT && chord->size == 0) {
    for (i32 i = 0; i < input->text_inputs.size; i++) {
      actions->push_back(input->text_inputs[i]);
    }
  }

  if (input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
    Action action;
    action.command        = Command::MOUSE_LEFT_CLICK;
    action.mouse_position = input->mouse_pos;
    actions->push_back(action);
  }
  if (input->mouse_button_up_events[(i32)MouseButton::RIGHT]) {
    Action action;
    action.command        = Command::MOUSE_RIGHT_CLICK;
    action.mouse_position = input->mouse_pos;
    actions->push_back(action);
  }
  {
    Action action;
    action.command           = Command::MOUSE_SCROLL;
    action.mouse_position    = input->mouse_pos;
    action.scrollwheel_delta = input->scrollwheel_count;
    actions->push_back(action);
  }

  for (i32 i = 0; i < input->key_inputs.size; i++) {
    KeyInput key = input->key_inputs[i];
    if (key.text_key && mode == Mode::INSERT && key.modifiers == Modifiers{} &&
        chord->size == 0) {
      break;
    }

    chord->push_back(key);
    show_partial_chord(chord);

    bool any_partial_matches = false;
    if (chord->size > 0) {
      for (i32 i = 0; i < bindings.size; i++) {
        if (match_chord(chord, &bindings[i].first, &any_partial_matches)) {
          Command command = bindings[i].second;
          actions->push_back(command);

          show_completed_chord(chord, command);
          chord->clear();
        }
      }

      if (!any_partial_matches) {
        chord->clear();
      }
    }
  }
}

bool eat(Action *action, Command command)
{
  if (action->eaten || action->command != command) {
    return false;
  }

  action->eaten = true;
  return true;
}

bool dont_eat(Action *action, Command command)
{
  return (!action->eaten && action->command == command);
}
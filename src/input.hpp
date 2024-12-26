#pragma once

#include "containers/array.hpp"
#include "math/math.hpp"

enum struct Key : u8 {
  SPACE,
  TAB,
  ENTER,
  BACKSPACE,
  DEL,

  NUM_0,
  NUM_1,
  NUM_2,
  NUM_3,
  NUM_4,
  NUM_5,
  NUM_6,
  NUM_7,
  NUM_8,
  NUM_9,

  EXCLAMATION_MARK,
  AT,
  HASH,
  DOLLAR_SIGN,
  PERCENT,
  CARET,
  AMPERSAND,
  STAR,
  LEFT_PARENTHSIS,
  RIGHT_PARENTHESIS,

  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,

  BACKTICK,
  TILDE,
  DASH,
  UNDERSCORE,
  EQUAL,
  PLUS,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  LEFT_CURLY_BRACE,
  RIGHT_CURLY_BRACE,
  SLASH,
  QUESTION_MARK,
  COMMA,
  LESS_THAN,
  PERIOD,
  GREATER_THAN,
  BACKSLASH,
  PIPE,

  UP,
  DOWN,
  LEFT,
  RIGHT,

  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,

  CAPS_LOCK,
  LSHIFT,
  RSHIFT,
  LCTRL,
  RCTRL,
  COMMAND,
  LALT,
  RALT,
  ESCAPE,
  INS,
  HOME,
  END,
  PAGEUP,
  PAGEDOWN,

  COUNT
};

enum struct MouseButton { LEFT, RIGHT, MIDDLE, COUNT };

struct Modifiers {
  bool shift : 1;
  bool ctrl : 1;
  bool alt : 1;
  bool super : 1;
};

struct KeyInput {
  Key key;
  Modifiers modifiers;
  bool text_key;

  bool operator==(const Key &other) { return key == other; }

  bool operator==(const KeyInput &other)
  {
    return key == other.key && modifiers.alt == other.modifiers.alt &&
           modifiers.ctrl == other.modifiers.ctrl &&
           modifiers.shift == other.modifiers.shift &&
           modifiers.super == other.modifiers.super;
  }
  bool operator!=(const KeyInput &other)
  {
    return key != other.key || modifiers.alt != other.modifiers.alt ||
           modifiers.ctrl != other.modifiers.ctrl ||
           modifiers.shift != other.modifiers.shift ||
           modifiers.super != other.modifiers.super;
  }
};

struct Input {
  bool keys[(int)Key::COUNT]            = {};
  bool key_up_events[(int)Key::COUNT]   = {};
  bool key_down_events[(int)Key::COUNT] = {};

  bool mouse_buttons[(int)MouseButton::COUNT]            = {};
  bool mouse_button_down_events[(int)MouseButton::COUNT] = {};
  bool mouse_button_up_events[(int)MouseButton::COUNT]   = {};

  Array<char, 128> text_inputs    = {};
  Array<KeyInput, 128> key_inputs = {};

  double scrollwheel_count = 0;

  Vec2f mouse_pos       = {};
  Vec2f mouse_pos_prev  = {};
  Vec2f mouse_pos_delta = {};
};

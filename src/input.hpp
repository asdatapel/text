#pragma once

#include <bitset>

#include "containers/array.hpp"
#include "math/math.hpp"
#include "string.hpp"

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

String KEY_STRINGS[] = {
    "SPACE",
    "TAB",
    "ENTER",
    "BACKSPACE",
    "DEL",

    "NUM_0",
    "NUM_1",
    "NUM_2",
    "NUM_3",
    "NUM_4",
    "NUM_5",
    "NUM_6",
    "NUM_7",
    "NUM_8",
    "NUM_9",

    "EXCLAMATION_MARK",
    "AT",
    "HASH",
    "DOLLAR_SIGN",
    "PERCENT",
    "CARET",
    "AMPERSAND",
    "STAR",
    "LEFT_PARENTHSIS",
    "RIGHT_PARENTHESIS",

    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",

    "BACKTICK",
    "TILDE",
    "DASH",
    "UNDERSCORE",
    "EQUAL",
    "PLUS",
    "LEFT_BRACKET",
    "RIGHT_BRACKET",
    "LEFT_CURLY_BRACE",
    "RIGHT_CURLY_BRACE",
    "SLASH",
    "QUESTION_MARK",
    "COMMA",
    "LESS_THAN",
    "PERIOD",
    "GREATER_THAN",
    "BACKSLASH",
    "PIPE",

    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",

    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",

    "CAPS_LOCK",
    "LSHIFT",
    "RSHIFT",
    "LCTRL",
    "RCTRL",
    "COMMAND",
    "LALT",
    "RALT",
    "ESCAPE",
    "INS",
    "HOME",
    "END",
    "PAGEUP",
    "PAGEDOWN",

    "COUNT",
};

enum struct MouseButton { LEFT, RIGHT, MIDDLE, COUNT };

struct Modifiers {
  std::bitset<4> values;

  Modifiers() {};
  Modifiers(bool shift, bool ctrl, bool alt, bool super)
  {
    values[0] = shift;
    values[1] = ctrl;
    values[2] = alt;
    values[3] = super;
  }
  static Modifiers with_super()
  {
    Modifiers m;
    m.values[3] = true;
    return m;
  }
  static Modifiers with_ctrl()
  {
    Modifiers m;
    m.values[1] = true;
    return m;
  }

  bool shift() const { return values[0]; }
  bool ctrl() const { return values[1]; }
  bool alt() const { return values[2]; }
  bool super() const { return values[3]; }
  void set_shift(bool val) { values.set(0, val); }
  void set_ctrl(bool val) { values.set(1, val); }
  void set_alt(bool val) { values.set(2, val); }
  void set_super(bool val) { values.set(3, val); }

  bool operator==(const Modifiers &other)
  {
    return values.to_ulong() == other.values.to_ulong();
  }
  bool operator!=(const Modifiers &other)
  {
    return values.to_ulong() != other.values.to_ulong();
  }
};

struct KeyInput {
  Key key;
  Modifiers modifiers;
  bool text_key;

  bool operator==(const Key &other) { return key == other; }

  bool operator==(const KeyInput &other)
  {
    return key == other.key && modifiers == other.modifiers;
  }
  bool operator!=(const KeyInput &other)
  {
    return key != other.key || modifiers != other.modifiers;
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

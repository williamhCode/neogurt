#pragma once
#include <string>

// ui related options sent by "option_set" event.
// events will be sent at startup and when options are changed.
// some events can be ignored (not very significant).
// others are needed when an ext_* is activated.
struct UiOptions {
  // std::string ambiwidth;
  // bool arabicshape;
  bool emoji;
  std::string guifont;
  std::string guifontwide;
  int linespace;
  bool mousefocus;
  bool mousehide;
  bool mousemoveevent;
  // int pumblend;
  // int showtabline;
  // int termguicolors;
  // bool termsync;
  // bool ttimeout;
  // int ttimeoutlen;
  // int verbose;
  // bool ext_linegrid;
  // bool ext_multigrid;
  // bool ext_hlstate;
  // bool ext_termcolors;
  // bool ext_cmdline;
  // bool ext_popupmenu;
  // bool ext_tabline;
  // bool ext_wildmenu;
  // bool ext_messages;
};

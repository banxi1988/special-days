#include <ncurses.h>

typedef enum TextStyle {
  TextStyleTitle = 20,
  TextStyleHeadline = 17,
  TextStyleBody = 17,
  // TextStyleCallout = 16,
  TextStyleSubhead = 15,
  TextStyleBirth = 31,
  TextStyleMemo = 32,
  TextStyleCountdown = 33,
  // TextStyleFootnote = 13,
  // TextStyleCaption = 12,
  TextStyleBorder = 11,
} TextStyle;

void cui_init_colors() {
  init_color(COLOR_BLUE, 10, 132, 255);
  init_color(COLOR_GREEN, 48, 209, 88);
  init_color(COLOR_CYAN, 100, 210, 255);
  init_color(COLOR_RED, 255, 69, 58);
  init_color(COLOR_YELLOW, 255, 214, 10);
  init_color(COLOR_MAGENTA, 191, 90, 242);

  init_pair(TextStyleTitle, COLOR_BLUE, COLOR_BLACK);
  init_pair(TextStyleBody, COLOR_WHITE, COLOR_BLACK);
  init_pair(TextStyleHeadline, COLOR_BLUE, COLOR_BLACK);
  init_pair(TextStyleSubhead, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(TextStyleMemo, COLOR_RED, COLOR_BLACK);
  init_pair(TextStyleBirth, COLOR_GREEN, COLOR_BLACK);
  init_pair(TextStyleCountdown, COLOR_YELLOW, COLOR_BLACK);
  init_pair(TextStyleBorder, COLOR_CYAN, COLOR_BLACK);
}

static int text_style_to_attr(TextStyle style) {
  switch (style) {
    case TextStyleHeadline:
    case TextStyleSubhead:
      return A_BOLD;
    default:
      break;
  }
  return 0;
}

void cui_set_text_style(TextStyle style) {
  attron(COLOR_PAIR(style));
  int attr = text_style_to_attr(style);
  if (attr) {
    attron(attr);
  }
}
void cui_unset_text_style(TextStyle style) {
  attroff(COLOR_PAIR(style));
  int attr = text_style_to_attr(style);
  if (attr) {
    attroff(attr);
  }
}

static int cur_y = 0;
static int cur_x = 0;
void cui_print_line(const char* str, TextStyle style) {
  cui_set_text_style(style);
  mvaddstr(cur_y, cur_x, str);
  cur_y += 1;
  cui_unset_text_style(style);
}
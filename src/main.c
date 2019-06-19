#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "codetalks.h"
#include "cui.h"

// 特殊日子类型,
typedef enum DayKind {
  DayKindBirth,     // 生日
  DayKindMemo,      // 纪念日
  DayKindCountdown  // 倒数日
} DayKind;

char *day_kind_to_text(DayKind kind) {
  switch (kind) {
    case DayKindBirth:
      return "生日";
    case DayKindMemo:
      return "纪念日";
    case DayKindCountdown:
      return "倒数日";
    default:
      return "未知";
  }
}

typedef struct SpecialDay {
  char date[20];  // 日期 2018-19-30 [20:[36:[23]] 时,分,秒为可选值
  char name[36];  // 名称
  DayKind kind;

} SpecialDay;

const char *encode_special_day(SpecialDay *day) {
  static char buf[64];
  snprintf(buf, sizeof(buf), "%20s %36s %d\n", day->date, day->name, day->kind);
  return buf;
}

void decode_special_day(char *buf, SpecialDay *day) {
  sscanf(buf, " %20s %36s %d", day->date, day->name, &day->kind);
}

bool parse_date(const char *date, struct tm *timeptr) {
  const size_t len = strlen(date);
  switch (len) {
    case 10:  // 2018-11-30
      strptime(date, "%F", timeptr);
      break;
    case 13:  // 2018-11-30 16
      strptime(date, "%F %H", timeptr);
      break;
    case 16:  // 2018-11-30 16:27
      strptime(date, "%F %H:%M", timeptr);
      break;
    case 19:  // 2018-11-30 16:27:43
      strptime(date, "%F %T", timeptr);
      break;
    default:
      return false;
  }
  return true;
}

typedef enum TimeUnit { SECONDS_OF_DAY = 24 * 60 * 60 } TimeUnit;

int days_since_date(const char *date) {
  struct tm date_tm;
  memset(&date_tm, 0, sizeof(struct tm));
  const bool ok = parse_date(date, &date_tm);
  if (!ok) {
    return INT32_MAX;
  }
  const time_t t = mktime(&date_tm);
  const time_t now = time(NULL);
  const int diff_seconds = now - t;
  return (int)ceil(diff_seconds * 1.0 / SECONDS_OF_DAY);
}

#define MAX_SPECIAL_DAYS (32)
typedef struct SpecialDayService {
  SpecialDay specialDays[MAX_SPECIAL_DAYS];
  int8_t count;
  int fd;

} SpecialDayService;

static SpecialDayService service;

bool add_special_day(SpecialDay day) {
  if (service.count >= MAX_SPECIAL_DAYS) {
    fprintf(stderr, "添加失败,最多只能添加 %d 个特殊日子\n", MAX_SPECIAL_DAYS);
    return false;
  }
  service.specialDays[service.count] = day;
  service.count += 1;
  return true;
}
void print_divider() {
  cui_print_line(
      "+-------------------------------------------------------------------"
      "------------+\n",
      TextStyleBorder);
}
void print_header() {
  print_divider();
  char buf[120];
  snprintf(buf, sizeof(buf), "%-8s%-12s%-24s%-24s%12s\n", "序号", "记数(天)",
           "日期", "名称", "类型");
  cui_print_line(buf, TextStyleSubhead);
  print_divider();
}

TextStyle day_kind_to_text_style(DayKind kind) {
  switch (kind) {
    case DayKindMemo:
      return TextStyleMemo;
    case DayKindBirth:
      return TextStyleBirth;
    case DayKindCountdown:
      return TextStyleCountdown;
    default:
      return TextStyleBody;
  }
}

void print_special_day(SpecialDay *day, int no) {
  int days = days_since_date(day->date);
  char buf[120];
  snprintf(buf, sizeof(buf), "%-8d%-12d%-24s%-24s%12s\n", no, days, day->date,
           day->name, day_kind_to_text(day->kind));

  cui_print_line(buf, day_kind_to_text_style(day->kind));
  print_divider();
}

void list_special_days() {
  print_header();
  for (int i = 0; i < service.count; i++) {
    print_special_day(&service.specialDays[i], i + 1);
  }
}

void persist_special_day(int index, SpecialDay *day) {
  const char *day_str = encode_special_day(day);
  int size = strlen(day_str);
  printf("记录长: %d bytes\n", size);
  int offset = index * size;
  int write_cnt = pwrite(service.fd, day_str, size, offset);
  assert(size == write_cnt);
}

void persist_all() {
  for (int i = 0; i < service.count; i++) {
    persist_special_day(i, &service.specialDays[i]);
  }
}

void load_all() {
  // 先将文件偏移指针移到去文件开头
  const int file_size = lseek(service.fd, 0, SEEK_END);
  printf("data file size:%d bytes\n", file_size);
  const int offset = lseek(service.fd, 0, SEEK_SET);
  assert(offset == 0);
  SpecialDay ref_day = {"", "", 0};
  const char *ref_day_str = encode_special_day(&ref_day);
  int size = strlen(ref_day_str);
  printf("示例记录长: %d bytes\n", size);
  char buf[size + 1];
  int read_cnt = 0;
  while ((read_cnt = read(service.fd, buf, size)) == size) {
    SpecialDay day;
    decode_special_day(buf, &day);
    add_special_day(day);
  };
  if (read_cnt == -1) {
    ct_exit("读取日子信息出错");
  } else if (read_cnt != 0) {
    ct_log_error("读取异常,没有读取到结尾 记录长:%d 读取数:%d", size, read_cnt);
  }
}

int main(int argc, char const *argv[]) {
  setlocale(LC_ALL, "");  // 使用环境变量的设置
  service.fd = open("days.data", O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
  if (service.fd == -1) {
    ct_exit("days.data 打开出错.");
    return -1;
  }
  load_all();
  WINDOW *win = initscr();  // 初始化当前窗口
  start_color();            // 使用颜色
  cui_init_colors();
  cbreak();  // 禁用输入缓冲
  noecho();  // 禁用回显
  // add_special_day(
  //     (SpecialDay){.date = "2010-07-22", .name = "相遇", .kind = MEMO});
  // add_special_day(
  //     (SpecialDay){.date = "2013-09-30", .name = "领证", .kind = MEMO});
  // add_special_day(
  //     (SpecialDay){.date = "2016-11-27", .name = "小泥巴", .kind = BIRTH});
  // add_special_day(
  //     (SpecialDay){.date = "2018-09-18", .name = "小月儿", .kind = BIRTH});
  // persist_all();
  list_special_days();
  int ch;
  nodelay(stdscr, TRUE);
  for (;;) {
    ch = getch();
    if (ch == ERR) {
    } else {
      // printf("User input:%c\n", ch);
      switch (ch) {
        case 'q':
          printw("Bye Bye!");
          break;
      }
    }
  }

  endwin();  // 恢复终端设置
  return 0;
}

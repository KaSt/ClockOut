#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <json-c/json.h>
#include <pwd.h>
 
#define DEFAULT_WORK_MINUTES 480
#define DEFAULT_LUNCH_MINUTES 45
#define TIMEBANK_FILENAME ".work_timer_timebank.json"
 
int work_minutes = DEFAULT_WORK_MINUTES;
int lunch_minutes = DEFAULT_LUNCH_MINUTES;
int break_minutes = 0;
int screen_rows, screen_cols;
int timebank_enabled = 0;
int discrete_mode = 0;
int quit_counter = 0;
double timebank_value = 0.0;
struct tm start_tm;
 
void handle_resize(int sig) {
    endwin();
    refresh();
    clear();
    getmaxyx(stdscr, screen_rows, screen_cols);
}
 
char* get_timebank_path() {
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s", homedir, TIMEBANK_FILENAME);
    return path;
}
 
void load_timebank() {
    char *path = get_timebank_path();
    FILE *file = fopen(path, "r");
    if (file) {
        char buffer[256];
        fread(buffer, 1, sizeof(buffer), file);
        fclose(file);
        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(buffer);
        struct json_object *tb;
        if (json_object_object_get_ex(parsed_json, "timebank", &tb)) {
            timebank_value = json_object_get_double(tb);
        }
    }
}
 
void save_timebank(double value) {
    char *path = get_timebank_path();
    FILE *file = fopen(path, "w");
    if (file) {
        struct json_object *tb_json = json_object_new_object();
        json_object_object_add(tb_json, "timebank", json_object_new_double(value));
        const char *json_str = json_object_to_json_string(tb_json);
        fprintf(file, "%s", json_str);
        fclose(file);
    }
}
 
int parse_time(const char *str, struct tm *tm_out) {
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    *tm_out = *now_tm;
    int hour, minute;
    char suffix[3] = "";
 
    if (sscanf(str, "%2dh%2d%2s", &hour, &minute, suffix) >= 2 ||
        sscanf(str, "%2d:%2d%2s", &hour, &minute, suffix) >= 2) {
        if (strlen(suffix) > 0 && (strcmp(suffix, "pm") == 0 || strcmp(suffix, "PM") == 0)) {
            if (hour < 12) hour += 12;
        }
        tm_out->tm_hour = hour;
        tm_out->tm_min = minute;
        tm_out->tm_sec = 0;
        return 1;
    }
    return 0;
}
 
void draw_centered_text(const char *text, int color_pair, int row_offset) {
    int len = strlen(text);
    int row = screen_rows / 2 + row_offset;
    int col = (screen_cols - len) / 2;

    if (row < 0) row = 0;
    if (row >= screen_rows) row = screen_rows - 1;
    if (col < 0) col = 0;

    attron(COLOR_PAIR(color_pair));
    mvprintw(row, col, "%s", text);
    attroff(COLOR_PAIR(color_pair));
}
 
void draw_discrete_blocks(int remaining) {
    int hours = remaining / 60;
    int minutes = remaining % 60;
    int hour_row = screen_rows / 2 - 1;
    int minute_row = screen_rows / 2;
    int hour_col = (screen_cols - hours) / 2;
    int minute_blocks = minutes / 10;
    int minute_col = (screen_cols - minute_blocks) / 2;
    int minute_extra = minutes % 10;
    int minute_extra_col = (screen_cols - minute_extra) / 2;
 
    attron(COLOR_PAIR(1));
    /* Draw hour blocks using full block */
    move(hour_row, hour_col);
    for (int i = 0; i < hours; i++) {
        addch(ACS_DIAMOND);
    }
    /* Draw minute blocks (5‑minute increments) using checkerboard */
    move(minute_row, minute_col);
    for (int i = 0; i < minute_blocks; i++) {
        addch(ACS_BLOCK);
    }
    move(minute_row+1, minute_extra_col);
    for (int i = 0; i < minute_extra; i++) {
        addch(ACS_BULLET);
    }
 
    attroff(COLOR_PAIR(1));
}

void draw_scaled_pattern(const char **pattern, int base_width, int top, int left, int scale) {
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < base_width; col++) {
            if (pattern[row][col] != ' ') {
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        int target_row = top + row * scale + dy;
                        int target_col = left + col * scale + dx;
                        if (target_row < 0 || target_row >= screen_rows) continue;
                        if (target_col < 0 || target_col >= screen_cols) continue;
                        mvaddch(target_row, target_col, ACS_BLOCK);
                    }
                }
            }
        }
    }
}

int draw_big_time(int remaining_seconds) {
    #define DIGIT_HEIGHT 7
    #define DIGIT_WIDTH 5
    #define COLON_WIDTH 3

    static const char *digits[10][DIGIT_HEIGHT] = {
        {" XXX ", "X   X", "X   X", "X   X", "X   X", "X   X", " XXX "},
        {"  X  ", " XX  ", "  X  ", "  X  ", "  X  ", "  X  ", " XXX "},
        {" XXX ", "X   X", "    X", "   X ", "  X  ", " X   ", "XXXXX"},
        {" XXX ", "    X", "    X", " XXX ", "    X", "    X", " XXX "},
        {"X   X", "X   X", "X   X", "XXXXX", "    X", "    X", "    X"},
        {"XXXXX", "X    ", "X    ", "XXXX ", "    X", "    X", "XXXX "},
        {" XXX ", "X    ", "X    ", "XXXX ", "X   X", "X   X", " XXX "},
        {"XXXXX", "    X", "   X ", "  X  ", "  X  ", "  X  ", "  X  "},
        {" XXX ", "X   X", "X   X", " XXX ", "X   X", "X   X", " XXX "},
        {" XXX ", "X   X", "X   X", " XXXX", "    X", "    X", " XXX "}
    };

    static const char *colon[DIGIT_HEIGHT] = {
        "   ",
        " X ",
        " X ",
        "   ",
        " X ",
        " X ",
        "   "
    };

    int hours = remaining_seconds / 3600;
    int minutes = (remaining_seconds % 3600) / 60;
    int seconds = remaining_seconds % 60;

    char time_str[16];
    if (remaining_seconds >= 3600) {
        if (hours > 99) {
            snprintf(time_str, sizeof(time_str), "%d:%02d", hours, minutes);
        } else {
            snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
        }
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
    }

    int base_total_width = 0;
    int len = strlen(time_str);
    for (int i = 0; i < len; i++) {
        base_total_width += (time_str[i] == ':') ? COLON_WIDTH : DIGIT_WIDTH;
        if (i != len - 1) {
            base_total_width += 1;
        }
    }

    int max_scale_w = screen_cols / (base_total_width > 0 ? base_total_width : 1);
    if (max_scale_w < 1) max_scale_w = 1;
    int max_scale_h = screen_rows / DIGIT_HEIGHT;
    if (max_scale_h < 1) max_scale_h = 1;
    int scale = max_scale_w < max_scale_h ? max_scale_w : max_scale_h;

    int actual_width = base_total_width * scale;
    int actual_height = DIGIT_HEIGHT * scale;
    int top = (screen_rows - actual_height) / 2;
    int left = (screen_cols - actual_width) / 2;
    if (top < 0) top = 0;
    if (left < 0) left = 0;

    int color_pair = remaining_seconds <= 60 ? 2 : 1;
    attron(COLOR_PAIR(color_pair));

    int x = left;
    for (int i = 0; i < len; i++) {
        if (time_str[i] == ':') {
            draw_scaled_pattern(colon, COLON_WIDTH, top, x, scale);
            x += COLON_WIDTH * scale;
        } else {
            int digit = time_str[i] - '0';
            if (digit >= 0 && digit <= 9) {
                draw_scaled_pattern(digits[digit], DIGIT_WIDTH, top, x, scale);
            }
            x += DIGIT_WIDTH * scale;
        }
        if (i != len - 1) {
            x += scale;
        }
    }

    attroff(COLOR_PAIR(color_pair));
    return actual_height;
}
#undef DIGIT_HEIGHT
#undef DIGIT_WIDTH
#undef COLON_WIDTH

#ifndef CLOCKOUT_NO_MAIN
int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    start_tm = *now_tm;
    start_tm.tm_sec = 0;
 
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--time-bank") == 0 || strcmp(argv[i], "-tb") == 0) && i + 1 < argc) {
            timebank_enabled = 1;
            timebank_value = atof(argv[++i]);
            save_timebank(timebank_value);
        } else if (strcmp(argv[i], "--discrete") == 0 || strcmp(argv[i], "-d") == 0) {
            discrete_mode = 1;
        } else if (strstr(argv[i], "h") || strstr(argv[i], ":")) {
            parse_time(argv[i], &start_tm);
        } else if (strstr(argv[i], "m")) {
            sscanf(argv[i], "%d", &lunch_minutes);
        } else if (strstr(argv[i], "h")) {
            int h;
            sscanf(argv[i], "%dh", &h);
            work_minutes = h * 60;
        }
    }
 
    if (timebank_enabled) {
        load_timebank();
    }
 
    signal(SIGWINCH, handle_resize);
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_GREEN, -1);
    getmaxyx(stdscr, screen_rows, screen_cols);
    timeout(1000);

    while (1) {
        time_t current = time(NULL);
        double elapsed = difftime(current, mktime(&start_tm));
        int elapsed_seconds = (int)elapsed;
        int total_minutes = work_minutes + lunch_minutes + break_minutes;
        int remaining_minutes = total_minutes - (int)(elapsed_seconds / 60);
        int remaining_seconds = total_minutes * 60 - elapsed_seconds;

        clear();
        if (discrete_mode) {
            draw_discrete_blocks(remaining_minutes);
        } else {
            if (remaining_seconds > 0) {
                int digit_height = draw_big_time(remaining_seconds);
                if (timebank_enabled) {
                    char tb_text[64];
                    snprintf(tb_text, sizeof(tb_text), "Time Bank: %.2f hours", timebank_value);
                    draw_centered_text(tb_text, 2, digit_height / 2 + 2);
                }
            } else {
                draw_centered_text("Work time completed!", 2, 0);
                if (timebank_enabled) {
                    char tb_text[64];
                    snprintf(tb_text, sizeof(tb_text), "Time Bank: %.2f hours", timebank_value);
                    draw_centered_text(tb_text, 2, 2);
                }
            }
        }

        refresh();
        int ch = getch();
        if (ch != ERR) {
            if (ch == 'q' || ch == 'Q') {
                quit_counter++;
                if (quit_counter >= 2) break;
            } else if ((ch == 'd' || ch == 'D') && !discrete_mode) {
                discrete_mode = 1;
                quit_counter = 0;
            } else if (ch == 'b' || ch == 'B') {
                quit_counter = 0;
                echo();
                curs_set(TRUE);
                timeout(-1);
                int prompt_row = screen_rows - 2;
                if (prompt_row < 0) prompt_row = 0;
                move(prompt_row, 0);
                clrtoeol();
                mvprintw(prompt_row, 0, "Break minutes: ");
                refresh();
                char buffer[16];
                if (getnstr(buffer, sizeof(buffer) - 1) != ERR) {
                    int bmin = atoi(buffer);
                    if (bmin > 0) {
                        break_minutes += bmin;
                    }
                }
                move(prompt_row, 0);
                clrtoeol();
                refresh();
                curs_set(FALSE);
                noecho();
                timeout(1000);
            } else {
                quit_counter = 0;
            }
        }

        if (remaining_minutes < 0 && timebank_enabled) {
            int extra = -remaining_minutes;
            timebank_value += extra / 60.0;
            save_timebank(timebank_value);
            break;
        }
    }
 
    endwin();
    return 0;
}
#endif


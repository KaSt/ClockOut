#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
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
    curs_set(FALSE);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_GREEN, -1);
    getmaxyx(stdscr, screen_rows, screen_cols);
 
    while (1) {
        time_t current = time(NULL);
        double elapsed = difftime(current, mktime(&start_tm));
        int remaining = work_minutes + lunch_minutes + break_minutes - (int)(elapsed / 60);
 
        clear();
        if (discrete_mode) {
            draw_discrete_blocks(remaining);
        } else {
            if (remaining > 60) {
                int hrs = remaining / 60;
                int mins = remaining % 60;
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%02d hours %02d minutes remaining", hrs, mins);
                draw_centered_text(buffer, 1, 0);
            } else if (remaining > 1) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%d minutes remaining", remaining);
                draw_centered_text(buffer, 1, 0);
            } else if (remaining == 1) {
                int seconds = 60 - (int)(elapsed) % 60;
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%d seconds remaining", seconds);
                draw_centered_text(buffer, 2, 0);
            } else {
                draw_centered_text("Work time completed!", 2, 0);
            }
        }
 
        if (timebank_enabled && !discrete_mode) {
            char tb_text[64];
            snprintf(tb_text, sizeof(tb_text), "Time Bank: %.2f hours", timebank_value);
            draw_centered_text(tb_text, 2, 2);
        }
 
        refresh();
        timeout(1000);
        char input[16];
        int ch = getnstr(input, sizeof(input) - 1);
        if (ch != ERR) {
            if (strcmp(input, "q") == 0) {
                quit_counter++;
                if (quit_counter >= 2) break;
            } else if (tolower(input[0]) == 'b' && isdigit(input[1])) {
                int bmin = atoi(&input[1]);
                break_minutes += bmin;
            } else {
                quit_counter = 0;
            }
        }
 
        if (remaining < 0 && timebank_enabled) {
            int extra = -remaining;
            timebank_value += extra / 60.0;
            save_timebank(timebank_value);
            break;
        }
    }
 
    endwin();
    return 0;
}
 

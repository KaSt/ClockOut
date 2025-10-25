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
#include <ctype.h>
#include <stdarg.h>
 
#define DEFAULT_WORK_MINUTES 480
#define DEFAULT_LUNCH_MINUTES 45
#define TIMEBANK_FILENAME ".work_timer_timebank.json"

/*
 * Update this value to bump the application's version. The build system reads
 * the constant directly when creating release artifacts, so no additional
 * configuration is necessary.
 */
static const char CLOCKOUT_VERSION[] = "0.1.0";
 
int work_minutes = DEFAULT_WORK_MINUTES;
int lunch_minutes = DEFAULT_LUNCH_MINUTES;
int break_minutes = 0;
int screen_rows, screen_cols;
int timebank_enabled = 0;
int discrete_mode = 0;
int maven_mode = 0;
int windows_mode = 0;
int quit_counter = 0;
int crypto_mode = 0;
int botnet_mode = 0;
double timebank_value = 0.0;
struct tm start_tm;

static char status_message[128] = "";

#define MAX_CRYPTO_LOGS 256
#define CRYPTO_LOG_WIDTH 160

static char crypto_logs[MAX_CRYPTO_LOGS][CRYPTO_LOG_WIDTH];
static int crypto_log_head = 0;
static int crypto_log_size = 0;

static const char *crypto_header[] = {
    "cgminer version 3.6.6 - Started: [2013-10-30 20:31:03]",
    "--------------------------------------------------------------------------------",
    " ST: 2  SS: 0  NB: 1  AW: 1.84Mh/s  LW: 0  GF: 0  RF: 0  WU: 1114.9/m",
    " Connected to coinotron.com diff 255 with stratum as user Haze.1",
    " Block: 330632ea508d. Diff: 161K  Started: [20:31:05]  Best share: 4.1K",
    "",
    " [P]ool management  [G]PU management  [S]ettings  [D]isplay options  [Q]uit",
    " GPU 0: 80.0C 2000RPM 1.24Mh/s | A:12495 R:100 HW:0 U:1.11/m"
};

typedef struct {
    const char *country;
    int infections;
} BotnetInfection;

static const BotnetInfection initial_botnet_infections[] = {
    {"US", 12840},
    {"CN", 12220},
    {"RU", 9960},
    {"DE", 7420},
    {"IN", 6905},
    {"BR", 5120},
    {"GB", 4980},
    {"JP", 4312},
    {"KR", 4155},
    {"FR", 3840},
    {"IT", 3410},
    {"CA", 2755},
    {"ES", 2430},
    {"AU", 2285},
    {"NL", 2100},
    {"SE", 1985},
    {"ZA", 1980},
    {"MX", 1830},
    {"PL", 1744},
    {"TR", 1635},
    {"AR", 1525},
    {"SG", 1408},
    {"NO", 1302},
    {"FI", 1258},
    {"CH", 1189},
    {"IL", 1104},
    {"IE", 1036},
    {"PT", 970},
    {"MY", 905},
    {"NZ", 860},
    {"AE", 795},
    {"DK", 750},
    {"HK", 720},
    {"TW", 688},
    {"CL", 655},
    {"PH", 640},
    {"TH", 622},
    {"CO", 604},
    {"RO", 590},
    {"UA", 575},
    {"CZ", 560},
    {"HU", 542},
    {"GR", 528},
    {"EG", 512},
    {"SA", 498},
    {"VN", 482},
    {"BE", 470},
    {"AT", 456},
    {"KW", 444},
    {"BG", 430},
    {"SK", 418},
    {"HR", 405},
    {"SI", 392},
    {"LT", 380},
    {"LV", 368},
    {"EE", 355},
    {"IS", 342},
    {"LU", 330},
    {"MT", 318},
    {"CY", 306},
    {"ME", 294},
    {"BA", 282}
};

#define BOTNET_COUNTRIES (int)(sizeof(initial_botnet_infections) / sizeof(initial_botnet_infections[0]))
static BotnetInfection botnet_infections[BOTNET_COUNTRIES];

static const char *botnet_process_prefixes[] = {
    "relay", "socket", "drone", "payload", "mirror", "quantum", "cipher", "ghost",
    "matrix", "orbital", "mesh", "cache", "proxy", "signal", "vector", "daemon",
    "channel", "kernel", "beacon", "sentinel", "crawler", "servo", "glyph", "cycle"
};

static const char *botnet_process_suffixes[] = {
    "scheduler", "binder", "orchestrator", "smelter", "sentinel", "spider", "router",
    "synthesizer", "uplink", "overseer", "compiler", "stalker", "aggregator", "locator",
    "sequencer", "observer", "switch", "vector", "breacher", "handler", "diffuser",
    "analyzer", "engine", "weaver"
};

static int botnet_process_counter = 0;

#define BOTNET_LOG_CAPACITY 256
#define BOTNET_LOG_WIDTH 160
static char botnet_logs[BOTNET_LOG_CAPACITY][BOTNET_LOG_WIDTH];
static int botnet_log_head = 0;
static int botnet_log_size = 0;

static const char *botnet_operations[] = {
    "sweeping cloud nodes",
    "weaponizing edge caches",
    "synchronizing drone swarm",
    "forging credential payload",
    "deploying Tonka drones",
    "priming silicon overclock",
    "binding lattice mesh",
    "ghosting upstream mirrors"
};

static const char *botnet_targets[] = {
    "finance-grid", "municipal-scada", "deepsea-comms", "exonet-relay",
    "orbital-uplink", "satfarm", "supply-chain", "autonomous-fleet"
};

static void reset_botnet_state() {
    for (int i = 0; i < BOTNET_COUNTRIES; i++) {
        botnet_infections[i] = initial_botnet_infections[i];
    }
    botnet_log_head = 0;
    botnet_log_size = 0;
}

static void append_botnet_log(const char *line) {
    if (botnet_log_size < BOTNET_LOG_CAPACITY) {
        int idx = (botnet_log_head + botnet_log_size) % BOTNET_LOG_CAPACITY;
        strncpy(botnet_logs[idx], line, BOTNET_LOG_WIDTH - 1);
        botnet_logs[idx][BOTNET_LOG_WIDTH - 1] = '\0';
        botnet_log_size++;
    } else {
        strncpy(botnet_logs[botnet_log_head], line, BOTNET_LOG_WIDTH - 1);
        botnet_logs[botnet_log_head][BOTNET_LOG_WIDTH - 1] = '\0';
        botnet_log_head = (botnet_log_head + 1) % BOTNET_LOG_CAPACITY;
    }
}

static void update_botnet_infections() {
    for (int i = 0; i < BOTNET_COUNTRIES; i++) {
        int spike = rand() % 250;
        botnet_infections[i].infections += 20 + spike;
    }
}

static void add_botnet_log_entry() {
    char buffer[BOTNET_LOG_WIDTH];
    const char *operation = botnet_operations[rand() % (sizeof(botnet_operations) / sizeof(botnet_operations[0]))];
    const char *target = botnet_targets[rand() % (sizeof(botnet_targets) / sizeof(botnet_targets[0]))];

    int wave = 50 + rand() % 950;
    int batch = 1 + rand() % 64;
    snprintf(buffer, sizeof(buffer), "[TonkaBotnet] wave:%03d batch:%02d :: %s -> %s", wave, batch, operation, target);
    append_botnet_log(buffer);
}

static int compare_infections_desc(const void *a, const void *b) {
    const BotnetInfection *ia = *(const BotnetInfection * const *)a;
    const BotnetInfection *ib = *(const BotnetInfection * const *)b;
    return ib->infections - ia->infections;
}

static void initialize_botnet_dashboard() {
    reset_botnet_state();
    for (int i = 0; i < 6; i++) {
        add_botnet_log_entry();
    }
}

static void draw_botnet_border() {
    if (screen_rows < 3 || screen_cols < 2) {
        return;
    }

    attron(COLOR_PAIR(7));
    for (int col = 0; col < screen_cols; col++) {
        mvaddch(0, col, '=');
        mvaddch(screen_rows - 2, col, '=');
    }
    for (int row = 1; row < screen_rows - 2; row++) {
        mvaddch(row, 0, '|');
        mvaddch(row, screen_cols - 1, '|');
    }
    mvaddch(0, 0, '+');
    mvaddch(0, screen_cols - 1, '+');
    mvaddch(screen_rows - 2, 0, '+');
    mvaddch(screen_rows - 2, screen_cols - 1, '+');
    attroff(COLOR_PAIR(7));

    const char *title = "*** TONKABOTNET OPS CENTER ***";
    int title_col = (screen_cols - (int)strlen(title)) / 2;
    if (title_col < 2) title_col = 2;
    attron(COLOR_PAIR(8));
    mvprintw(0, title_col, "%s", title);
    attroff(COLOR_PAIR(8));
}

static void draw_botnet_infection_panel(int top, int left, int height, int width) {
    attron(COLOR_PAIR(8));
    mvprintw(top, left, "INFECTED NODES");
    attroff(COLOR_PAIR(8));

    if (height <= 2) {
        return;
    }

    BotnetInfection *sorted[BOTNET_COUNTRIES];
    for (int i = 0; i < BOTNET_COUNTRIES; i++) {
        sorted[i] = &botnet_infections[i];
    }
    qsort(sorted, BOTNET_COUNTRIES, sizeof(sorted[0]), compare_infections_desc);

    int rows_available = height - 2;
    if (rows_available > BOTNET_COUNTRIES) {
        rows_available = BOTNET_COUNTRIES;
    }

    for (int i = 0; i < rows_available; i++) {
        int row = top + 1 + i;
        attron(COLOR_PAIR(9));
        mvprintw(row, left, "%2s ", sorted[i]->country);
        attroff(COLOR_PAIR(9));
        int value_col = left + 4;
        if (value_col < left + width) {
            mvprintw(row, value_col, "%7d infections", sorted[i]->infections);
        }
    }
}

static void draw_botnet_logs_panel(int top, int left, int height, int width) {
    attron(COLOR_PAIR(8));
    mvprintw(top, left, "COORDINATED STRIKES");
    attroff(COLOR_PAIR(8));

    int usable = height - 2;
    if (usable <= 0) return;

    int to_show = botnet_log_size < usable ? botnet_log_size : usable;
    int start = botnet_log_size - to_show;
    for (int i = 0; i < to_show; i++) {
        int idx = (botnet_log_head + start + i) % BOTNET_LOG_CAPACITY;
        int row = top + 1 + i;
        mvprintw(row, left, "%-*.*s", width, width, botnet_logs[idx]);
    }
}

static void draw_botnet_process_panel(int top, int left, int height, int width, int remaining_seconds) {
    attron(COLOR_PAIR(8));
    mvprintw(top, left, "PROCESS GRID");
    attroff(COLOR_PAIR(8));

    if (height <= 2) {
        return;
    }

    const char *headers = "PID    CPU   TASK";
    mvprintw(top + 1, left, "%s", headers);

    int hours = remaining_seconds / 3600;
    if (hours < 0) hours = 0;
    int minutes = (remaining_seconds % 3600) / 60;
    if (minutes < 0) minutes = 0;

    char cpu_buffer[16];
    snprintf(cpu_buffer, sizeof(cpu_buffer), "%02d.%02d", hours, minutes);

    const char *core_task = "tonkacore-scheduler";
    int task_width = width - 13;
    if (task_width > 0) {
        mvprintw(top + 2, left, "4211   %s  %-*.*s", cpu_buffer, task_width, task_width, core_task);
    } else {
        mvprintw(top + 2, left, "4211   %s", cpu_buffer);
    }

    int rows_available = height - 3;
    if (rows_available <= 0) {
        return;
    }

    int prefix_count = (int)(sizeof(botnet_process_prefixes) / sizeof(botnet_process_prefixes[0]));
    int suffix_count = (int)(sizeof(botnet_process_suffixes) / sizeof(botnet_process_suffixes[0]));

    if (prefix_count == 0 || suffix_count == 0) {
        return;
    }

    for (int i = 0; i < rows_available; i++) {
        double load = 5.0 + (rand() % 950) / 10.0;
        int pid = 5100 + ((botnet_process_counter + i * 17) % 3890);
        const char *prefix = botnet_process_prefixes[(botnet_process_counter + i) % prefix_count];
        const char *suffix = botnet_process_suffixes[(botnet_process_counter / prefix_count + i) % suffix_count];

        char task_buffer[64];
        snprintf(task_buffer, sizeof(task_buffer), "%s-%s", prefix, suffix);

        if (task_width > 0) {
            mvprintw(top + 3 + i, left, "%4d   %05.1f  %-*.*s", pid, load, task_width, task_width, task_buffer);
        } else {
            mvprintw(top + 3 + i, left, "%4d   %05.1f", pid, load);
        }
    }

    botnet_process_counter = (botnet_process_counter + 1) % (prefix_count * suffix_count);
    mvprintw(top + 2, left, "4211   %s  %s", cpu_buffer, core_task);

    const char *extra_tasks[] = {
        "relay-mapper", "socket-binder", "drone-orchestrator", "payload-smelter",
        "mirror-sentinel", "quantum-spider"
    };

    int rows_available = height - 3;
    for (int i = 0; i < rows_available && i < (int)(sizeof(extra_tasks) / sizeof(extra_tasks[0])); i++) {
        double load = (rand() % 900) / 10.0;
        int pid = 5300 + rand() % 400;
        mvprintw(top + 3 + i, left, "%4d   %05.1f  %s", pid, load, extra_tasks[i]);
    }
}

static void draw_botnet_mode(int remaining_seconds) {
    wbkgd(stdscr, COLOR_PAIR(0));
    draw_botnet_border();

    int inner_top = 1;
    int inner_left = 2;
    int inner_height = screen_rows - 4;
    if (inner_height < 1) return;
    int inner_width = screen_cols - 4;
    if (inner_width < 3) return;

    int panel_width = inner_width / 3;
    int remainder = inner_width - panel_width * 3;

    int infection_width = panel_width;
    int logs_width = panel_width;
    int process_width = panel_width + remainder;

    draw_botnet_infection_panel(inner_top, inner_left, inner_height, infection_width);
    draw_botnet_logs_panel(inner_top, inner_left + infection_width + 1, inner_height, logs_width);
    draw_botnet_process_panel(inner_top, inner_left + infection_width + logs_width + 2, inner_height, process_width, remaining_seconds);

    if (status_message[0] != '\0') {
        attron(COLOR_PAIR(9));
        mvprintw(screen_rows - 1, 2, "%-*s", screen_cols - 4, status_message);
        attroff(COLOR_PAIR(9));
    } else {
        mvprintw(screen_rows - 1, 2, "%-*s", screen_cols - 4, "");
    }
}

static void trim_whitespace(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

static void handle_command_prompt() {
    echo();
    curs_set(TRUE);
    timeout(-1);

    int prompt_row = screen_rows - 1;
    if (prompt_row < 0) prompt_row = 0;
    move(prompt_row, 0);
    clrtoeol();
    mvprintw(prompt_row, 0, ":");
    refresh();

    char buffer[64];
    if (getnstr(buffer, sizeof(buffer) - 1) != ERR) {
        trim_whitespace(buffer);
        if (buffer[0] != '\0') {
            char command[64];
            strncpy(command, buffer, sizeof(command) - 1);
            command[sizeof(command) - 1] = '\0';

            char *space = strchr(command, ' ');
            char *argument = NULL;
            if (space) {
                *space = '\0';
                argument = space + 1;
                trim_whitespace(argument);
            }

            for (char *p = command; *p; ++p) {
                *p = (char)tolower((unsigned char)*p);
            }

            if (strcmp(command, "break") == 0 && argument) {
                int minutes = atoi(argument);
                if (minutes > 0) {
                    break_minutes += minutes;
                    set_status_message("Recorded %d minute break.", minutes);
                } else {
                    set_status_message("Invalid break duration: %s", argument);
                }
            } else if (strcmp(command, "break") == 0) {
                set_status_message("Usage: break <minutes>");
            } else {
                set_status_message("Unknown command: %s", buffer);
            }
            quit_counter = 0;
        }
    }

    move(prompt_row, 0);
    clrtoeol();
    refresh();

    curs_set(FALSE);
    noecho();
    timeout(1000);
}

static void draw_status_line() {
    if (botnet_mode) {
        return;
    }

    if (screen_rows <= 0) {
        return;
    }

    int row = screen_rows - 1;
    move(row, 0);
    clrtoeol();
    if (status_message[0] != '\0') {
        attron(COLOR_PAIR(8));
        mvprintw(row, 1, "%s", status_message);
        attroff(COLOR_PAIR(8));
    }
}

void set_status_message(const char *fmt, ...) {
    if (!fmt) {
        status_message[0] = '\0';
        return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(status_message, sizeof(status_message), fmt, args);
    va_end(args);
}

void reset_crypto_logs() {
    crypto_log_head = 0;
    crypto_log_size = 0;
}

void append_crypto_log(const char *line) {
    if (crypto_log_size < MAX_CRYPTO_LOGS) {
        int idx = (crypto_log_head + crypto_log_size) % MAX_CRYPTO_LOGS;
        strncpy(crypto_logs[idx], line, CRYPTO_LOG_WIDTH - 1);
        crypto_logs[idx][CRYPTO_LOG_WIDTH - 1] = '\0';
        crypto_log_size++;
    } else {
        strncpy(crypto_logs[crypto_log_head], line, CRYPTO_LOG_WIDTH - 1);
        crypto_logs[crypto_log_head][CRYPTO_LOG_WIDTH - 1] = '\0';
        crypto_log_head = (crypto_log_head + 1) % MAX_CRYPTO_LOGS;
    }
}

void generate_crypto_log_line(char *buffer, size_t size, int remaining_seconds) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_now);

    char share[9];
    for (int i = 0; i < 8; i++) {
        int v = rand() % 16;
        share[i] = v < 10 ? '0' + v : 'a' + (v - 10);
    }
    share[8] = '\0';

    if (remaining_seconds < 0) {
        remaining_seconds = 0;
    }

    int eff_hours = remaining_seconds / 3600;
    int eff_minutes = (remaining_seconds % 3600) / 60;

    if (rand() % 12 == 0) {
        int pool = rand() % 3;
        int diff_whole = 10 + rand() % 500;
        int diff_frac = rand() % 100;
        snprintf(buffer, size, "%s Found block for pool %d Diff %d.%02dK BLOCK! Eff: %d.%02dh",
                 timestamp, pool, diff_whole, diff_frac, eff_hours, eff_minutes);
    } else {
        int diff_value = 50 + rand() % 400;
        int gpu = 1 + rand() % 3;
        const char *notes[] = {"yay!!!", "nice!", "accepted"};
        const char *note = notes[rand() % (sizeof(notes) / sizeof(notes[0]))];
        snprintf(buffer, size, "%s Accepted %s Diff %d GPU %d (%s) Eff: %d.%02dh",
                 timestamp, share, diff_value, gpu, note, eff_hours, eff_minutes);
    }
}

void add_crypto_log_entries(int remaining_seconds) {
    int lines = 1 + rand() % 3;
    for (int i = 0; i < lines; i++) {
        char line[CRYPTO_LOG_WIDTH];
        generate_crypto_log_line(line, sizeof(line), remaining_seconds);
        append_crypto_log(line);
    }
}

void draw_crypto_screen() {
    int header_lines = sizeof(crypto_header) / sizeof(crypto_header[0]);
    int max_rows = screen_rows < header_lines ? screen_rows : header_lines;
    for (int i = 0; i < max_rows; i++) {
        mvprintw(i, 0, "%s", crypto_header[i]);
    }

    if (screen_rows <= header_lines) {
        return;
    }

    int available = screen_rows - header_lines;
    int to_show = crypto_log_size < available ? crypto_log_size : available;
    int start_offset = crypto_log_size > available ? crypto_log_size - available : 0;

    for (int i = 0; i < to_show; i++) {
        int idx = (crypto_log_head + start_offset + i) % MAX_CRYPTO_LOGS;
        mvprintw(header_lines + i, 0, "%s", crypto_logs[idx]);
    }
}
 
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
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
        fclose(file);
        if (bytes_read == 0) {
            return;
        }
        buffer[bytes_read] = '\0';
        struct json_object *parsed_json = json_tokener_parse(buffer);
        if (parsed_json != NULL) {
            struct json_object *tb;
            if (json_object_object_get_ex(parsed_json, "timebank", &tb)) {
                timebank_value = json_object_get_double(tb);
            }
            json_object_put(parsed_json);
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
        json_object_put(tb_json);
    }
}

#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))

void random_maven_file(char *buffer, size_t size) {
    if (size == 0) return;
    static const char *folders[] = {
        "src/main/java",
        "src/main/resources",
        "src/test/java",
        "src/test/resources"
    };
    static const char *packages[] = {
        "com/example/clockout",
        "org/clockout/app",
        "io/github/clockout",
        "net/company/clockout"
    };
    static const char *class_names[] = {
        "ClockOutApplication",
        "TimeBankService",
        "BreakTracker",
        "DisplayManager",
        "FocusMode",
        "MavenIntegration"
    };
    static const char *resource_files[] = {
        "application.yml",
        "banner.txt",
        "logback.xml",
        "messages.properties",
        "schema.sql",
        "build-info.properties"
    };
    static const char *code_extensions[] = {"java", "kt"};

    const char *folder = folders[rand() % ARRAY_SIZE(folders)];
    if (strstr(folder, "java") != NULL) {
        const char *pkg = packages[rand() % ARRAY_SIZE(packages)];
        const char *class_name = class_names[rand() % ARRAY_SIZE(class_names)];
        const char *ext = code_extensions[rand() % ARRAY_SIZE(code_extensions)];
        snprintf(buffer, size, "%s/%s/%s.%s", folder, pkg, class_name, ext);
    } else {
        const char *resource = resource_files[rand() % ARRAY_SIZE(resource_files)];
        snprintf(buffer, size, "%s/%s", folder, resource);
    }
}

void draw_footer_instructions() {
    if (screen_rows <= 0) return;
    int row = screen_rows - 1;
    move(row, 0);
    clrtoeol();
    mvprintw(row, 0, "[s] Standard  [d] Discrete  [m] Maven  [b] Break  [q] Quit");
}

void draw_windows_mode(int remaining_seconds, int total_seconds) {
    if (total_seconds <= 0) {
        total_seconds = 1;
    }
    if (remaining_seconds < 0) {
        remaining_seconds = 0;
    }

    wbkgd(stdscr, COLOR_PAIR(3));
    erase();

    int title_left = 2;
    if (screen_cols > 36) {
        title_left = (screen_cols - 36) / 2;
    }
    if (title_left < 0) title_left = 0;
    if (title_left >= screen_cols) title_left = screen_cols > 0 ? screen_cols - 1 : 0;

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(1, title_left, "Windows XP Professional Setup");
    attroff(COLOR_PAIR(4) | A_BOLD);

    attron(COLOR_PAIR(3));
    int info_left = 4;
    if (screen_cols > 70) {
        info_left = (screen_cols - 70) / 2;
        if (info_left < 2) info_left = 2;
    }
    if (info_left < 0) info_left = 0;
    if (info_left >= screen_cols) info_left = screen_cols > 0 ? screen_cols - 1 : 0;
    mvprintw(3, info_left, "Please wait while Setup formats the partition");
    mvprintw(5, info_left, "   C: Partition1 [New (Raw)]        16370 MB ( 16370 MB free)");
    mvprintw(6, info_left, "   on 16379 MB Disk 0 at Id 0 on bus 0 on atapi [MBR].");

    int box_width = screen_cols - info_left * 2;
    if (box_width > 74) box_width = 74;
    if (box_width < 20) box_width = screen_cols - 4;
    if (box_width < 16) box_width = screen_cols;
    int box_left = (screen_cols - box_width) / 2;
    if (box_left < 0) box_left = 0;
    int box_top = screen_rows / 2 - 2;
    if (box_top < 8) box_top = 8;
    if (box_top + 4 >= screen_rows) {
        box_top = screen_rows > 5 ? screen_rows - 5 : 0;
    }

    int hours_left = remaining_seconds / 3600;
    int minutes_left = (remaining_seconds % 3600) / 60;
    char status_line[64];
    snprintf(status_line, sizeof(status_line), "Setup is formatting... %d.%02d", hours_left, minutes_left);

    attron(COLOR_PAIR(3));
    mvaddch(box_top, box_left, ACS_ULCORNER);
    mvhline(box_top, box_left + 1, ACS_HLINE, box_width - 2);
    mvaddch(box_top, box_left + box_width - 1, ACS_URCORNER);
    for (int i = 1; i < 4; i++) {
        mvaddch(box_top + i, box_left, ACS_VLINE);
        mvaddch(box_top + i, box_left + box_width - 1, ACS_VLINE);
    }
    mvaddch(box_top + 4, box_left, ACS_LLCORNER);
    mvhline(box_top + 4, box_left + 1, ACS_HLINE, box_width - 2);
    mvaddch(box_top + 4, box_left + box_width - 1, ACS_LRCORNER);

    mvprintw(box_top + 1, box_left + 2, "%s", status_line);

    int bar_width = box_width - 4;
    if (bar_width < 4) bar_width = box_width;
    int bar_left = box_left + (box_width - bar_width) / 2;
    int bar_top = box_top + 3;

    double progress_ratio = 1.0 - (double)remaining_seconds / (double)total_seconds;
    if (progress_ratio < 0.0) progress_ratio = 0.0;
    if (progress_ratio > 1.0) progress_ratio = 1.0;
    int filled = (int)(progress_ratio * bar_width + 0.5);
    if (filled > bar_width) filled = bar_width;
    if (filled < 0) filled = 0;

    attron(COLOR_PAIR(5));
    for (int i = 0; i < bar_width; i++) {
        mvaddch(bar_top, bar_left + i, ' ');
    }
    attroff(COLOR_PAIR(5));

    attron(COLOR_PAIR(6));
    for (int i = 0; i < filled; i++) {
        mvaddch(bar_top, bar_left + i, ' ');
    }
    attroff(COLOR_PAIR(6));

    if (remaining_seconds <= 0) {
        int dialog_width = 24;
        int dialog_height = 5;
        if (dialog_width > screen_cols - 4) dialog_width = screen_cols - 4;
        if (dialog_width < 16) dialog_width = screen_cols - 2;
        if (dialog_width < 10) dialog_width = screen_cols;
        if (dialog_height > screen_rows - 4) dialog_height = screen_rows - 4;
        if (dialog_height < 3) dialog_height = screen_rows;
        int dialog_left = (screen_cols - dialog_width) / 2;
        if (dialog_left < 0) dialog_left = 0;
        int dialog_top = box_top;
        if (dialog_top + dialog_height >= screen_rows) {
            dialog_top = screen_rows > dialog_height ? screen_rows - dialog_height : 0;
        }

        attron(COLOR_PAIR(5));
        for (int r = 0; r < dialog_height; r++) {
            for (int c = 0; c < dialog_width; c++) {
                mvaddch(dialog_top + r, dialog_left + c, ' ');
            }
        }
        attroff(COLOR_PAIR(5));

        attron(COLOR_PAIR(3));
        mvaddch(dialog_top, dialog_left, ACS_ULCORNER);
        mvhline(dialog_top, dialog_left + 1, ACS_HLINE, dialog_width - 2);
        mvaddch(dialog_top, dialog_left + dialog_width - 1, ACS_URCORNER);
        for (int r = 1; r < dialog_height - 1; r++) {
            mvaddch(dialog_top + r, dialog_left, ACS_VLINE);
            mvaddch(dialog_top + r, dialog_left + dialog_width - 1, ACS_VLINE);
        }
        mvaddch(dialog_top + dialog_height - 1, dialog_left, ACS_LLCORNER);
        mvhline(dialog_top + dialog_height - 1, dialog_left + 1, ACS_HLINE, dialog_width - 2);
        mvaddch(dialog_top + dialog_height - 1, dialog_left + dialog_width - 1, ACS_LRCORNER);

        const char *message = "you can eject";
        int msg_left = dialog_left + (dialog_width - (int)strlen(message)) / 2;
        int msg_row = dialog_top + dialog_height / 2;
        if (msg_row >= dialog_top + dialog_height) msg_row = dialog_top + dialog_height - 2;
        mvprintw(msg_row, msg_left, "%s", message);
        attroff(COLOR_PAIR(3));
    }

    attroff(COLOR_PAIR(3));
}

void draw_maven_mode(int remaining_seconds) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char version[16];
    snprintf(version, sizeof(version), "%d.%02d", tm_now->tm_hour, tm_now->tm_min);

    int remaining = remaining_seconds;
    if (remaining < 0) remaining = 0;
    int rem_hours = remaining / 3600;
    int rem_minutes = (remaining % 3600) / 60;
    char remaining_label[32];
    snprintf(remaining_label, sizeof(remaining_label), "%02d:%02d", rem_hours, rem_minutes);

    static const char *phases[] = {"clean", "resources", "compile", "test", "package", "install"};
    static const char *goals[] = {
        "maven-resources-plugin",
        "maven-compiler-plugin",
        "maven-surefire-plugin",
        "maven-jar-plugin",
        "maven-install-plugin"
    };
    static const char *modules[] = {
        "clockout-core",
        "clockout-api",
        "clockout-cli",
        "clockout-ui",
        "clockout-integration",
        "clockout-database"
    };
    static const char *actions[] = {
        "Compiling",
        "Processing",
        "Copying",
        "Generating",
        "Analyzing",
        "Running"
    };

    int row = 2;
    mvprintw(row++, 0, "[INFO] Scanning for projects...");
    mvprintw(row++, 0, "[INFO] Building ClockOut %s", version);
    mvprintw(row++, 0, "[INFO] Remaining focus time: %s", remaining_label);
    if (timebank_enabled) {
        mvprintw(row++, 0, "[INFO] Time bank balance: %.2f h", timebank_value);
    }

    int lines_remaining = screen_rows - row - 1;
    if (lines_remaining < 0) lines_remaining = 0;

    for (int i = 0; i < lines_remaining && row < screen_rows - 1; i++) {
        const char *module = modules[rand() % ARRAY_SIZE(modules)];
        if (i % 3 == 0) {
            const char *goal = goals[rand() % ARRAY_SIZE(goals)];
            const char *phase = phases[rand() % ARRAY_SIZE(phases)];
            mvprintw(row++, 0, "[INFO] --- %s:%s:%s (%s) @ %s ---", goal, phase, version, module, module);
        } else {
            const char *action = actions[rand() % ARRAY_SIZE(actions)];
            char file[256];
            random_maven_file(file, sizeof(file));
            mvprintw(row++, 0, "[INFO] %s %s", action, file);
        }
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
 
    srand((unsigned)time(NULL));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0) {
            printf("ClockOut %s\n", CLOCKOUT_VERSION);
            return 0;
        } else if ((strcmp(argv[i], "--time-bank") == 0 || strcmp(argv[i], "-tb") == 0) && i + 1 < argc) {
            timebank_enabled = 1;
            timebank_value = atof(argv[++i]);
            save_timebank(timebank_value);
        } else if (strcmp(argv[i], "--discrete") == 0 || strcmp(argv[i], "-d") == 0) {
            discrete_mode = 1;
            windows_mode = 0;
            botnet_mode = 0;
        } else if (strcmp(argv[i], "--cryptomining") == 0 || strcmp(argv[i], "-c") == 0) {
            crypto_mode = 1;
            windows_mode = 0;
            botnet_mode = 0;
        } else if (strcmp(argv[i], "--windows") == 0 || strcmp(argv[i], "-w") == 0) {
            windows_mode = 1;
            discrete_mode = 0;
            maven_mode = 0;
            botnet_mode = 0;
        } else if (strcmp(argv[i], "--maven") == 0 || strcmp(argv[i], "-m") == 0) {
            maven_mode = 1;
            discrete_mode = 0;
            crypto_mode = 0;
            botnet_mode = 0;
        } else if (strcmp(argv[i], "--botnet") == 0 || strcmp(argv[i], "-b") == 0) {
            botnet_mode = 1;
            discrete_mode = 0;
            maven_mode = 0;
            windows_mode = 0;
            crypto_mode = 0;
        } else if (strstr(argv[i], "h") || strstr(argv[i], ":")) {
            parse_time(argv[i], &start_tm);
        } else if (strstr(argv[i], "m")) {
            sscanf(argv[i], "%d", &lunch_minutes);
        } else if (strstr(argv[i], "h")) {
            int h;
            sscanf(argv[i], "%dh", &h);
            work_minutes = h * 60;
        } else {
            size_t arg_len = strlen(argv[i]);
            int has_am_pm = 0;
            if (arg_len >= 2) {
                char second_last = (char)tolower((unsigned char)argv[i][arg_len - 2]);
                char last = (char)tolower((unsigned char)argv[i][arg_len - 1]);
                if ((second_last == 'a' || second_last == 'p') && last == 'm') {
                    has_am_pm = 1;
                }
            }

            if (strchr(argv[i], ':') != NULL ||
                (strchr(argv[i], 'h') != NULL && arg_len > 0 && argv[i][arg_len - 1] != 'h') ||
                has_am_pm) {
                parse_time(argv[i], &start_tm);
            } else if (arg_len > 1 && argv[i][arg_len - 1] == 'm') {
                int numeric = 1;
                for (size_t j = 0; j < arg_len - 1; j++) {
                    if (!isdigit((unsigned char)argv[i][j])) {
                        numeric = 0;
                        break;
                    }
                }
                if (numeric) {
                    int minutes;
                    if (sscanf(argv[i], "%d", &minutes) == 1) {
                        lunch_minutes = minutes;
                    }
                }
            } else if (arg_len > 1 && argv[i][arg_len - 1] == 'h') {
                int numeric = 1;
                for (size_t j = 0; j < arg_len - 1; j++) {
                    if (!isdigit((unsigned char)argv[i][j])) {
                        numeric = 0;
                        break;
                    }
                }
                if (numeric) {
                    int h;
                    if (sscanf(argv[i], "%d", &h) == 1) {
                        work_minutes = h * 60;
                    }
                }
            }
        }
    }

    if (botnet_mode) {
        initialize_botnet_dashboard();
    }

    if (crypto_mode) {
        reset_crypto_logs();
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
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);
    init_pair(5, COLOR_BLACK, COLOR_WHITE);
    init_pair(6, COLOR_BLACK, COLOR_YELLOW);
    init_pair(7, COLOR_GREEN, -1);
    init_pair(8, COLOR_CYAN, -1);
    init_pair(9, COLOR_MAGENTA, -1);
    getmaxyx(stdscr, screen_rows, screen_cols);
    timeout(1000);

    while (1) {
        time_t current = time(NULL);
        double elapsed = difftime(current, mktime(&start_tm));
        int elapsed_seconds = (int)elapsed;
        int total_minutes = work_minutes + lunch_minutes + break_minutes;
        int remaining_minutes = total_minutes - (int)(elapsed_seconds / 60);
        int total_seconds = total_minutes * 60;
        int remaining_seconds = total_seconds - elapsed_seconds;
        
        clear();

        if (botnet_mode) {
            update_botnet_infections();
            add_botnet_log_entry();
            draw_botnet_mode(remaining_seconds);
        } else if (windows_mode) {
            draw_windows_mode(remaining_seconds, total_seconds);
        } else if (crypto_mode) {
            add_crypto_log_entries(remaining_seconds);
            draw_crypto_screen();
        } else if (maven_mode) {
            draw_maven_mode(remaining_seconds);
        } else if (discrete_mode) {
            draw_discrete_blocks(remaining_minutes);
        } else {
            wbkgd(stdscr, COLOR_PAIR(0));
            clear();

            if (crypto_mode) {
                add_crypto_log_entries(remaining_seconds);
                draw_crypto_screen();
            } else if (maven_mode) {
                draw_maven_mode(remaining_seconds);
            } else if (discrete_mode) {
                draw_discrete_blocks(remaining_minutes);
                //draw_footer_instructions();
            } else {
                char version_label[64];
                snprintf(version_label, sizeof(version_label), "ClockOut v%s", CLOCKOUT_VERSION);
                mvprintw(0, 0, "%s", version_label);

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
            }
        }

        if (!botnet_mode) {
            draw_status_line();
        }

        refresh();
        int ch = getch();
        if (ch != ERR) {
            if (ch == 'q' || ch == 'Q') {
                quit_counter++;
                if (quit_counter >= 2) break;
            } else if (ch == 'd' || ch == 'D') {
                discrete_mode = !discrete_mode;
                maven_mode = 0;
                windows_mode = 0;
                quit_counter = 0;
            } else if (ch == 's' || ch == 'S') {
                discrete_mode = 0;
                maven_mode = 0;
                windows_mode = 0;
                crypto_mode = 0;
                botnet_mode = 0;
                quit_counter = 0;
            } else if (ch == 'm' || ch == 'M') {
                maven_mode = 1;
                discrete_mode = 0;
                windows_mode = 0;
                crypto_mode = 0;
                botnet_mode = 0;
                quit_counter = 0;
            } else if (ch == 'c' || ch == 'C') {
                crypto_mode = !crypto_mode;
                if (crypto_mode) {
                    reset_crypto_logs();
                }
                if (crypto_mode) {
                    windows_mode = 0;
                }
                if (crypto_mode) {
                    botnet_mode = 0;
                }
                quit_counter = 0;
            } else if (ch == 'w' || ch == 'W') {
                windows_mode = 1;
                maven_mode = 0;
                discrete_mode = 0;
                crypto_mode = 0;
                botnet_mode = 0;
                quit_counter = 0;
            } else if (ch == 'b' || ch == 'B') {
                botnet_mode = !botnet_mode;
                if (botnet_mode) {
                    crypto_mode = 0;
                    windows_mode = 0;
                    discrete_mode = 0;
                    maven_mode = 0;
                    initialize_botnet_dashboard();
                }
                quit_counter = 0;
            } else if (ch == ':') {
                handle_command_prompt();
                quit_counter = 0;
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


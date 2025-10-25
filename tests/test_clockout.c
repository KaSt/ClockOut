#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int parse_time(const char *str, struct tm *tm_out);
void save_timebank(double value);
void load_timebank(void);
char *get_timebank_path(void);

extern double timebank_value;

static void test_parse_time_formats(void) {
    struct tm tm = {0};
    assert(parse_time("08:30", &tm));
    assert(tm.tm_hour == 8);
    assert(tm.tm_min == 30);

    tm.tm_hour = tm.tm_min = -1;
    assert(parse_time("8:30pm", &tm));
    assert(tm.tm_hour == 20);
    assert(tm.tm_min == 30);

    tm.tm_hour = tm.tm_min = -1;
    assert(parse_time("14h15", &tm));
    assert(tm.tm_hour == 14);
    assert(tm.tm_min == 15);
}

static void test_parse_time_invalid(void) {
    struct tm tm;
    assert(parse_time("invalid", &tm) == 0);
}

static void test_timebank_roundtrip(void) {
    char backup_path[512];
    char *path = get_timebank_path();
    int had_existing = access(path, F_OK) == 0;
    if (had_existing) {
        snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
        if (rename(path, backup_path) != 0) {
            perror("rename");
            exit(EXIT_FAILURE);
        }
    }

    double original_value = timebank_value;
    double expected = 1.75;
    save_timebank(expected);
    timebank_value = 0.0;
    load_timebank();
    assert(fabs(timebank_value - expected) < 1e-6);

    unlink(path);
    if (had_existing) {
        if (rename(backup_path, path) != 0) {
            perror("restore rename");
            exit(EXIT_FAILURE);
        }
    }
    timebank_value = original_value;
}

int main(void) {
    test_parse_time_formats();
    test_parse_time_invalid();
    test_timebank_roundtrip();
    printf("All tests passed.\n");
    return 0;
}

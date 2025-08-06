//
// Created by jiaxv on 23-8-17.
//

#include "ld_log.h"

#include <linux/limits.h>
/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#define MAX_CALLBACKS 32
#define MAX_LOG_BUFFER_SIZE 16384

typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
} Callback;

static struct {
    void *udata;
    log_LockFn lock;
    int level;
    bool quiet;
    Callback callbacks[MAX_CALLBACKS];
    struct timespec g_start;
} L;


static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif

static size_t get_current_time(char *buf, size_t max_len) {
	struct timespec curr;
	int secs, nsecs;
	if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1)
		log_error("clock_gettime");

	secs = curr.tv_sec - L.g_start.tv_sec;
	nsecs = curr.tv_nsec - L.g_start.tv_nsec;
	if (nsecs < 0) {
		secs--;
		nsecs += 1000000000;
	}

    snprintf(buf, max_len, "%d.%09d", secs, nsecs);

    return strlen(buf);
}


static void stdout_callback(log_Event *ev) {
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ev->time)] = '\0';
    // buf[get_current_time(buf, sizeof(buf))] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(
        ev->udata, "[%s.%06ld] %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
        buf, ev->tv.tv_usec, level_colors[ev->level], level_strings[ev->level],
        ev->file, ev->line);
    // fprintf(
    //     ev->udata, "[%s] %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    //     buf, level_colors[ev->level], level_strings[ev->level],
    //     ev->file, ev->line);
#else
    fprintf(
            ev->udata, "%s %-5s %s:%d: ",
            buf, level_strings[ev->level], ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}


static void file_callback(log_Event *ev) {
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ev->time)] = '\0';
    fprintf(
        ev->udata, "[%s.%06ld] %-5s %s:%d: ",
        buf, ev->tv.tv_usec, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}


static void lock(void) {
    if (L.lock) { L.lock(TRUE, L.udata); }
}


static void unlock(void) {
    if (L.lock) { L.lock(FALSE, L.udata); }
}


const char *log_level_string(int level) {
    return level_strings[level];
}


void log_set_lock(log_LockFn fn, void *udata) {
    L.lock = fn;
    L.udata = udata;
}


void log_set_level(int level) {
    L.level = level;
}


void log_set_quiet(bool enable) {
    L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!L.callbacks[i].fn) {
            L.callbacks[i] = (Callback){fn, udata, level};
            return 0;
        }
    }
    return -1;
}


int log_add_fp(FILE *fp, int level) {
    return log_add_callback(file_callback, fp, level);
}


static void init_event(log_Event *ev, void *udata) {
    // if (!ev->time) {
    //     time_t t = time(NULL);
    //     ev->time = localtime(&t);
    // }
    gettimeofday(&ev->tv, NULL);
    localtime_r(&ev->tv.tv_sec, &ev->time);

    ev->udata = udata;
}

void log_va(int level, const char *file, int line, const char *fmt, va_list *va) {
    log_Event ev = {
        .fmt = fmt,
        .file = file,
        .line = line,
        .level = level,
    };
    va_copy(ev.ap, *va);

    lock();

    if (!L.quiet && level >= L.level) {
        init_event(&ev, stderr);
        stdout_callback(&ev);
    }


    for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Callback *cb = &L.callbacks[i];
        if (level >= cb->level) {
            init_event(&ev, cb->udata);
            cb->fn(&ev);
        }
    }

    unlock();
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {

    // log_Event ev = {
    //     .fmt = fmt,
    //     .file = file,
    //     .line = line,
    //     .level = level,
    // };
    //
    // lock();
    //
    // if (!L.quiet && level >= L.level) {
    //     init_event(&ev, stderr);
    //     va_start(ev.ap, fmt);
    //     stdout_callback(&ev);
    //     va_end(ev.ap);
    // }
    //
    //
    // for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    //     Callback *cb = &L.callbacks[i];
    //     if (level >= cb->level) {
    //         init_event(&ev, cb->udata);
    //         va_start(ev.ap, fmt);
    //         cb->fn(&ev);
    //         va_end(ev.ap);
    //     }
    // }
    //
    // unlock();

    va_list args;
    va_start(args, fmt);
    log_va(level, file, line, fmt, &args);
    va_end(args);
}


void log_print_buffer(int level, const char *file, int line, const char *msg_head, const uint8_t *buf, size_t len) {
    char *out_log = NULL;
    size_t sz = 0;
    FILE *log_stream = open_memstream(&out_log, &sz);
    fprintf(log_stream, "%s%s:", level_colors[level], msg_head);

    for (int i = 0; i < len; i++)
        fprintf(log_stream, "%02x ", buf[i]);

    fprintf(log_stream, "\033[0m");
    fclose(log_stream);

    // char out_log[MAX_LOG_BUFFER_SIZE * 2] = {0};
    // sprintf(out_log, "%s%s: ", level_colors[level], msg_head);
    // for (int i = 0; i < len; i++)
    //     sprintf(out_log, "%s%02x ", out_log, buf[i]);
    //
    // sprintf(out_log, "%s\033[0m", out_log);
    log_log(level, file, line, out_log);
    free(out_log);
}

l_err log_init(int level, const char *log_dir, const char *role_str) {
    if (!log_dir)   return LD_ERR_NULL;

    char time_str[32] = {0};
    char *log_path = NULL;
    char *log_file = NULL;
    size_t p_sz, f_sz = 0;
    FILE *path_stream = NULL;
    FILE *file_stream = NULL;

    if (log_dir[0] == '~') {
        const char *home_dir = getenv("HOME");
        char command[PATH_MAX] = {0};
        memcpy(command, home_dir, strlen(home_dir));
        memcpy(command + strlen(home_dir), log_dir + 1, strlen(log_dir) -1);
        log_dir = command;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &L.g_start) == -1)
        log_error("clock_gettime");

    /* check the basic log directory */
    if (check_path(log_dir)) {
        return LD_ERR_INTERNAL;
    }

    path_stream = open_memstream(&log_path, &p_sz);

    /* get current date as log folder name of today */
    get_time(time_str, LD_TIME_UNDERLINE, LD_TIME_DAY);
    fprintf(path_stream, "%s/%s", log_dir, time_str);
    fclose(path_stream);
    if (check_path(log_path)) {
        log_warn("Wrong Log path");
        free(log_path);
        return LD_ERR_INTERNAL;
    }

    file_stream = open_memstream(&log_file, &f_sz);
    /* get current second with role as log name */
    get_time(time_str, LD_TIME_UNDERLINE,LD_TIME_MICRO);
    fprintf(file_stream, "%s/%s_%s.log", log_path, time_str, role_str);
    fclose(file_stream);

    FILE *log_p = fopen(log_file, "w");
    log_set_level(level);
    log_add_fp(log_p, level);

    free(log_path);
    free(log_file);
    return LD_OK;
}

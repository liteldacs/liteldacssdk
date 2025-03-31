//
// Created by 邹嘉旭 on 2025/4/1.
//

/* Sanitize character string in situ: turns dangerous characters into \OOO.
 * With a bit of work, we could use simpler reps for \\, \r, etc.,
 * but this is only to protect against something that shouldn't be used.
 * Truncate resulting string to what fits in buffer.
 */

#include <ld_log.h>

#include "passert.h"

openswan_passert_fail_t openswan_passert_fail = (openswan_passert_fail_t) openswanlib_passert_fail;
bool log_to_stderr = TRUE;
bool log_to_syslog = FALSE;

const char *progname;

size_t sanitize_string(char *buf, size_t size) {
#   define UGLY_WIDTH	4	/* width for ugly character: \OOO */
    size_t len;
    size_t added = 0;
    char *p;

    //passert(size >= UGLY_WIDTH);	/* need room to swing cat */

    /* find right side of string to be sanitized and count
     * number of columns to be added.  Stop on end of string
     * or lack of room for more result.
     */
    for (p = buf; *p != '\0' && &p[added] < &buf[size - UGLY_WIDTH]; p++) {
        unsigned char c = *p;

        /* exception is that all veritical space just becomes white space */
        if (c == '\n' || c == '\r') {
            *p = ' ';
            continue;
        }

        if (c == '\\' || !isprint(c))
            added += UGLY_WIDTH - 1;
    }

    /* at this point, p points after last original character to be
     * included.  added is how many characters are added to sanitize.
     * so p[added] will point after last sanitized character.
     */

    p[added] = '\0';
    len = &p[added] - buf;

    /* scan backwards, copying characters to their new home
     * and inserting the expansions for ugly characters.
     *
     * vertical space is changed to horizontal.
     *
     * It is finished when no more shifting is required.
     * This is a predecrement loop.
     */
    while (added != 0) {
        char fmtd[UGLY_WIDTH + 1];
        unsigned char c;

        while ((c = *--p) != '\\' && isprint(c))
            p[added] = c;

        added -= UGLY_WIDTH - 1;
        snprintf(fmtd, sizeof(fmtd), "\\%03o", c);
        memcpy(p + added, fmtd, UGLY_WIDTH);
    }
    return len;
#   undef UGLY_WIDTH
}

int DBG_log(const char *message, ...) {
    va_list args;
    char m[LOG_WIDTH]; /* longer messages will be truncated */

    va_start(args, message);
    vsnprintf(m, sizeof(m), message, args);
    va_end(args);

    /* then sanitize anything else that is left. */
    (void) sanitize_string(m, sizeof(m));

    if (log_to_stderr)
        fprintf(stderr, "| %s\n", m);
    if (log_to_syslog)
        log_debug("| %s", m);

    return 0;
}


static void fmt_log(char *buf, size_t buf_len, const char *fmt, va_list ap) {
    fprintf(stderr, "%p", fmt);
    bool reproc = *fmt == '~';
    size_t ps = 0;

    buf[0] = '\0';
    if (reproc)
        fmt++; /* ~ at start of format suppresses this prefix */
    else if (progname != NULL && (strlen(progname) + 1 + 1) < buf_len) {
        /* start with name of connection_s */
        strncat(buf, progname, buf_len - 1);
        strncat(buf, " ", buf_len - 1);
    }

    ps = strlen(buf);
    vsnprintf(buf + ps, buf_len - ps, fmt, ap);
    if (!reproc)
        (void) sanitize_string(buf, buf_len);
}


err_t builddiag(const char *fmt, ...) {
    static char mydiag_space[LOG_WIDTH]; /* longer messages will be truncated */
    char t[sizeof(mydiag_space)]; /* build result here first */
    va_list args;

    va_start(args, fmt);
    t[0] = '\0'; /* in case nothing terminates string */
    vsnprintf(t, sizeof(t), fmt, args);
    va_end(args);
    strcpy(mydiag_space, t);
    return mydiag_space;
}

void openswanlib_passert_fail(const char *pred_str, const char *file_str,
                              unsigned long line_no) {
    /* we will get a possibly unplanned prefix.  Hope it works */
    loglog(RC_LOG_SERIOUS, "ASSERTION FAILED at %s:%lu: %s", file_str, line_no, pred_str);
    abort(); /* exiting correctly doesn't always work */
}

void
openswan_switch_fail(int n, const char *file_str, unsigned long line_no) {
    char buf[30];

    snprintf(buf, sizeof(buf), "case %d unexpected", n);
    openswanlib_passert_fail(buf, file_str, line_no);
}

void loglog(int mess_no, const char *message, ...) {
    va_list args;
    char m[LOG_WIDTH]; /* longer messages will be truncated */

    va_start(args, message);
    fmt_log(m, sizeof(m), message, args);
    va_end(args);

    if (log_to_stderr)
        fprintf(stderr, "%s\n", m);
    if (log_to_syslog)
        log_warn("%s", m);
}

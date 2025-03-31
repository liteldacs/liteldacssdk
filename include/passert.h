//
// Created by jiaxv on 23-6-14.
//

#ifndef TESTMS_PASSERT_H
#define TESTMS_PASSERT_H

#include "ldacs_sim.h"
#include "ldacs_def.h"


typedef void (*openswan_passert_fail_t)(const char *pred_str,
                                        const char *file_str,
                                        unsigned long line_no) __attribute__ ((noreturn));
extern openswan_passert_fail_t openswan_passert_fail;



# define passert(pred) do { \
	if (!(pred)) \
	  if(openswan_passert_fail) { \
	    (*openswan_passert_fail)(#pred, __FILE__, __LINE__);	\
	  } \
  } while(0)

# define bad_case(n) openswan_switch_fail((int) n, __FILE__, __LINE__)


enum rc_type {
    RC_COMMENT, /* non-commital utterance (does not affect exit status) */
    RC_WHACK_PROBLEM, /* whack-detected problem */
    RC_LOG, /* message aimed at log (does not affect exit status) */
    RC_LOG_SERIOUS, /* serious message aimed at log (does not affect exit status) */
    RC_SUCCESS, /* success (exit status 0) */
};


extern const char *progname;
#define LOG_WIDTH  1024
typedef unsigned long long lset_t;
static char bitnamesbuf[200]; /* only one!  I hope that it is big enough! */

int DBG_log(const char *message, ...);


# define PRINTF_LIKE(n) __attribute__ ((format(printf, n, n+1)))

//err_t builddiag(const char *fmt, ...);
extern err_t builddiag(const char *fmt, ...) PRINTF_LIKE(1);

void openswanlib_passert_fail(const char *pred_str, const char *file_str,
                              unsigned long line_no);

void openswan_switch_fail(int n, const char *file_str, unsigned long line_no);

void loglog(int mess_no, const char *message, ...);



#endif //TESTMS_PASSERT_H

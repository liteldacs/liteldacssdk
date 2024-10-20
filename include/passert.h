//
// Created by jiaxv on 23-6-14.
//

#ifndef TESTMS_PASSERT_H
#define TESTMS_PASSERT_H


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

#endif //TESTMS_PASSERT_H

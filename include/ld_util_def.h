//
// Created by jiaxv on 23-7-8.
//

#ifndef TEST_MSG_DEFINES_H
#define TEST_MSG_DEFINES_H
/* pad_up(n, m) is the amount to add to n to make it a multiple of m */
#define pad_up(n, m) (((m) - 1) - (((n) + (m) - 1) % (m)))

enum complement_size {
    COMPLEMENT_8 = 0xFF,
    COMPLEMENT_16 = 0xFFFF,
    COMPLEMENT_32 = 0xFFFFFFFF,
    COMPLEMENT_64 = 0xFFFFFFFFFFFFFFFF,
};

#define zero(x) memset((x), '\0', sizeof(*(x)))

#define UNFOUND -1

#ifndef ARG_MAX

#define __ARGS(X) (X)

#define __ARGC_N(_0,_1,_2,_3,_4,_5,_6,_7,N,...) N

#define __ARGC(...) __ARGS(__ARGC_N(__VA_ARGS__,8,7,6,5,4,3,2,1))

#define __ARG0(_0,...) _0
#define __ARG1(_0,_1,...) _1
#define __ARG2(_0,_1,_2,...) _2
#define __ARG3(_0,_1,_2,_3,...) _3
#define __ARG4(_0,_1,_2,_3,_4,...) _4
#define __ARG5(_0,_1,_2,_3,_4,_5,...) _5
#define __ARG6(_0,_1,_2,_3,_4,_5,_6,...) _6
#define __ARG7(_0,_1,_2,_3,_4,_5,_6,_7,...) _7

#define __VA0(...) __ARGS(__ARG0(__VA_ARGS__,0)+0)
#define __VA1(...) __ARGS(__ARG1(__VA_ARGS__,0,0))
#define __VA2(...) __ARGS(__ARG2(__VA_ARGS__,0,0,0))
#define __VA3(...) __ARGS(__ARG3(__VA_ARGS__,0,0,0,0))
#define __VA4(...) __ARGS(__ARG4(__VA_ARGS__,0,0,0,0,0))
#define __VA5(...) __ARGS(__ARG5(__VA_ARGS__,0,0,0,0,0,0))
#define __VA6(...) __ARGS(__ARG6(__VA_ARGS__,0,0,0,0,0,0,0))
#define __VA7(...) __ARGS(__ARG7(__VA_ARGS__,0,0,0,0,0,0,0,0))

#define ARG_MAX         8
#define ARGC(...)       __ARGC(__VA_ARGS__)
#define ARGS(x, ...)    __VA##x(__VA_ARGS__)

#endif

#define ACCURACY 0.001


enum TIME_MOD {
    LD_TIME_DAY = 0,
    LD_TIME_SEC,
};

static const char *time_format[] = {
    "%Y_%m_%d",
    "%Y_%m_%d_%H_%M_%S",
};

#endif //TEST_MSG_DEFINES_H

//
// Created by 邹嘉旭 on 2024/7/18.
//

#ifndef LDACS_DEF_H
#define LDACS_DEF_H

#define SYSTEM_BITS 64

typedef enum { TRUE = 1, FALSE = 0 } bool;

typedef bool is_stop;
extern is_stop volatile stop_flag;

typedef void (*free_func)(void *);

typedef enum { FL, RL } ld_orient;

typedef enum {
    LD_OK = 0,
    LD_ERR_WRONG_PARA,
    LD_ERR_INTERNAL,
    LD_ERR_WAIT,
    LD_ERR_INVALID,
    LD_ERR_INVALID_MAC,
    LD_ERR_LOCK,
    LD_ERR_NOMEM,
    LD_ERR_THREAD,
    LD_ERR_QUEUE,
    LD_ERR_NULL,
    LD_ERR_KEY,

    /*================= for statemachine ====================*/
    /**
         * \brief The error state was reached
         *
         * This value is returned either when the state machine enters the error
         * state itself as a result of an error, or when the error state is the
         * next state as a result of a successful transition.
         *
         * The state machine enters the state machine if any of the following
         * happens:
         * - The current state is NULL
         * - A transition for the current event did not define the next state
         */
    LD_ERR_INVAL_STATE_REACHED,
    /**
     * \brief The state changed back to itself
     *
     * The state can return to itself either directly or indirectly. An
     * indirect path may inlude a transition from a parent state and the use of
     * \ref state::entryState "entryStates".
     */
    LD_ERR_STATE_LOOP,
    /**
     * \brief The current state did not change on the given event
     *
     * If any event passed to the state machine should result in a state
     * change, this return value should be considered as an error.
     */
    LD_ERR_NO_STATE_CHANGE,

    /** \brief A final state (any but the error state) was reached */
    LD_FINAL_STATE_REACHD,

    LD_ERR_WRONG_STATE,

    /* ================ for file =================== */
    LD_ERR_WRONG_PATH,

    /* ================ for http =================== */
    LD_ERR_WRONG_REQ,
    LD_ERR_WRONG_QUERY,
    LD_ERR_WRONG_DATA,
    LD_ERR_WRONG_URI,
    LD_NO_REQ,

    /* ================ for sqlite =================== */
    LD_ERR_OPEN_SQL,
    LD_ERR_FAIL_EXEC_SQL,
} l_err;

#define UALEN 28

#define IPVERSION_4 4
#define IPVERSION_6 6

#define SF_TIMER 240000000
#define MF_TIMER 58000000
#define BC_MAC_INTVL 230000000
#define CC_MAC_INTVL 50000000

#define SAC_LEN 12
#define DEFAULT_SAC 0xFFFF
#define DEFAULT_CO 0xFFFF
#define DEFAULT_RPSO 0xFF
#define DEFAULT_NRPS 0xFF
#define CO_LEN 9


#define MAX_INPUT_BUFFER_SIZE 8192
#define MAX_OUTPUT_BUFFER_SIZE 8192
#define MAX_JSON_ENBASE64  3296  //the biggest length of phy_sdus (1) QPSK with 1/2 rate (9 sdus): 1092 bytes; (2) 64QAM with 3/4 rate (6 sdus): 3296 bytes



#define MAX_JSON_DEBASE64  2472  //the biggest length of phy_sdus (1) QPSK with 1/2 rate (9 sdus): 819 bytes; (2) 64QAM with 3/4 rate (6 sdus): 2472 bytes



#define ERROR (-1)
#define END (0)
#define OK (0)
#define AGAIN (1)

#define LELEM(opt) (1ULL << (opt)) //ULL -> unsigned long long
#define BITS_PER_BYTE 8

#define KEY_HANDLE void *

typedef const char *err_t; /* error message, or NULL for success */


#endif //LDACS_DEF_H

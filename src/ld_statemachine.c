//
// Created by 邹嘉旭 on 2024/4/3.
//

#include "ld_statemachine.h"

static void goToErrorState(struct sm_statemachine_s *stateMachine,
                           struct sm_event_s *const event);

static struct sm_transition_s *getTransition(struct sm_statemachine_s *stateMachine,
                                             struct sm_state_s *state, struct sm_event_s *const event);

void stateM_init(struct sm_statemachine_s *fsm,
                 struct sm_state_s *initialState, struct sm_state_s *errorState) {
    if (!fsm)
        return;

    fsm->currentState = initialState;
    fsm->previousState = NULL;
    fsm->errorState = errorState;
}

l_err stateM_handleEvent(struct sm_statemachine_s *fsm,
                         struct sm_event_s *event) {
    if (!fsm || !event)
        return LD_ERR_WRONG_PARA;

    if (!fsm->currentState) {
        goToErrorState(fsm, event);
        return LD_ERR_INVAL_STATE_REACHED;
    }

    if (!fsm->currentState->numTransitions)
        return LD_ERR_NO_STATE_CHANGE;

    struct sm_state_s *nextState = fsm->currentState;
    do {
        struct sm_transition_s *transition = getTransition(fsm, nextState, event);

        /* If there were no transitions for the given event for the current
         * state, check if there are any transitions for any of the parent
         * states (if any): */
        if (!transition) {
            nextState = nextState->parentState;
            continue;
        }
        /* A transition must have a next state defined. If the user has not
         * defined the next state, go to error state: */
        if (!transition->nextState) {
            goToErrorState(fsm, event);
            return LD_ERR_INVAL_STATE_REACHED;
        }

        nextState = transition->nextState;

        /* If the new state is a parent state, enter its entry state (if it has
         * one). Step down through the whole family tree until a state without
         * an entry state is found: */
        while (nextState->entryState)
            nextState = nextState->entryState;

        /* Run exit action only if the current state is left (only if it does
         * not return to itself): */
        if (nextState != fsm->currentState && fsm->currentState->exitAction)
            fsm->currentState->exitAction(fsm->currentState->data, event);

        /* Run transition action (if any): */
        if (transition->action)
            transition->action(fsm->currentState->data, event, nextState->
                               data);

        /* Call the new state's entry action if it has any (only if state does
         * not return to itself): */
        if (nextState != fsm->currentState && nextState->entryAction) {
            /*TODO: 未来entryAction需要返回l_err， 一旦L_err不等于0，即goToErrorState() */
            if (nextState->entryAction(nextState->data, event)) break;
        }

        fsm->previousState = fsm->currentState;
        fsm->currentState = nextState;


        /* If the state returned to itself: */
        if (fsm->currentState == fsm->previousState)
            return LD_ERR_STATE_LOOP;

        if (fsm->currentState == fsm->errorState)
            return LD_ERR_INVAL_STATE_REACHED;

        /* If the new state is a final state, notify user that the state
         * machine has stopped: */
        if (!fsm->currentState->numTransitions)
            return LD_FINAL_STATE_REACHD;

        return LD_OK;
    } while (nextState);

    return LD_ERR_NO_STATE_CHANGE;
}

struct sm_state_s *stateM_currentState(struct sm_statemachine_s *fsm) {
    if (!fsm)
        return NULL;

    return fsm->currentState;
}

struct sm_state_s *stateM_previousState(struct sm_statemachine_s *fsm) {
    if (!fsm)
        return NULL;

    return fsm->previousState;
}


static void goToErrorState(struct sm_statemachine_s *fsm,
                           struct sm_event_s *const event) {
    fsm->previousState = fsm->currentState;
    fsm->currentState = fsm->errorState;

    if (fsm->currentState && fsm->currentState->entryAction)
        fsm->currentState->entryAction(fsm->currentState->data, event);
}

static struct sm_transition_s *getTransition(struct sm_statemachine_s *fsm,
                                             struct sm_state_s *state, struct sm_event_s *const event) {
    size_t i;

    for (i = 0; i < state->numTransitions; ++i) {
        struct sm_transition_s *t = &state->transitions[i];

        /* A transition for the given event has been found: */
        if (t->eventType == event->type) {
            if (!t->guard)
                return t;
                /* If transition is guarded, ensure that the condition is held: */
            else if (t->guard(t->condition, event))
                return t;
        }
    }

    /* No transitions found for given event for given state: */
    return NULL;
}

bool stateM_stopped(struct sm_statemachine_s *stateMachine) {
    if (!stateMachine)
        return TRUE;

    return stateMachine->currentState->numTransitions == 0;
}

l_err change_state(sm_statemachine_t *fsm, int ev_type, fsm_event_data_t *to_state) {
    return stateM_handleEvent(fsm, &(struct sm_event_s){
                                  ev_type, to_state
                              });
}

bool in_state(sm_statemachine_t *fsm, const char *dest_state) {
    const char *curr_state = (char *) stateM_currentState(fsm)->data;
    return !strcmp(curr_state, dest_state);
}



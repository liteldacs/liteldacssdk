//
// Created by 邹嘉旭 on 2024/4/3.
//

/*
 * Copyright (c) 2013 Andreas Misje
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \mainpage %stateMachine
 *
 * %stateMachine is a feature-rich, yet simple finite state machine
 * implementation. It supports grouped states, guarded transitions, events
 * with payload, entry and exit actions, transition actions and access to
 * user-defined state data from all actions.
 *
 * The user must build the state machine by linking together states and
 * transitions arrays with pointers. A pointer to an initial state and an
 * error state is given to stateM_init() to initialise a state machine object.
 * The state machine is run by passing events to it with the function
 * stateM_handleEvent(). The return value of stateM_handleEvent() will
 * give an indication to what has happened.
 *
 * \image html stateMachine.svg "Illustrating a stateMachine"
 */

/**
 * \defgroup stateMachine State machine
 *
 * \author Andreas Misje
 * \date 27.03.13
 *
 * \brief Finite state machine
 *
 * A finite state machine implementation that supports nested states, guards
 * and entry/exit routines. All state machine data is stored in separate
 * objects, and the state machine must be built by the user. States are
 * connected using pointers, and all data can be stored on either the stack,
 * heap or both.
 */

/**
 * \addtogroup stateMachine
 * @{
 *
 * \file
 * \example stateMachineExample.c Simple example of how to create a state
 * machine
 * \example nestedTest.c Simple example testing the behaviour of nested
 * parent states
 */


#ifndef LD_STATEMACHINE_H
#define LD_STATEMACHINE_H


#include "../global/ldacs_sim.h"

/**
 * \brief Event
 *
 * Events trigger transitions from a state to another. Event types are defined
 * by the user. Any event may optionally contain a \ref #event::data
 * "payload".
 *
 * \sa state
 * \sa transition
 */
typedef struct sm_event_s {
    /** \brief Type of event. Defined by user. */
    int type;
    /**
     * \brief Event payload.
     *
     * How this is used is entirely up to the user. This data
     * is always passed together with #type in order to make it possible to
     * always cast the data correctly.
     */
    void *data;
} sm_event_t;

typedef struct fsm_event_s {
    char *state_str;
    /* the event will be activated at entryState().
     * It ***CAN NOT*** contain any procdure about state-changing. */
    l_err (*entry_event)(void *);
    l_err (*exit_event)(void *);
} fsm_event_t;

typedef struct fsm_event_data_s {
    fsm_event_t *fev;
    void *args;
} fsm_event_data_t;


struct sm_state_s;

/**
 * \brief Transition between a state and another state
 *
 * All states that are not final must have at least one transition. The
 * transition may be guarded or not. Transitions are triggered by events. If
 * a state has more than one transition with the same type of event (and the
 * same condition), the first transition in the array will be run. An
 * unconditional transition placed last in the transition array of a state can
 * act as a "catch-all". A transition may optionally run an #action, which
 * will have the triggering event passed to it as an argument, along with the
 * current and new states' \ref state::data "data".
 *
 * It is perfectly valid for a transition to return to the state it belongs
 * to. Such a transition will not call the state's \ref state::entryAction
 * "entry action" or \ref state::exitAction "exit action". If there are no
 * transitions for the current event, the state's parent will be handed the
 * event.
 *
 * ### Examples ###
 * - An ungarded transition to a state with no action performed:
 * ~~~{.c}
 * {
 *    .eventType = Event_timeout,
 *    .condition = NULL,
 *    .guard = NULL,
 *    .action = NULL,
 *    .nextState = &mainMenuState,
 * },
 * ~~~
 * - A guarded transition executing an action
 * ~~~{.c}
 * {
 *    .eventType = Event_keyboard,
 *    .condition = NULL,
 *    .guard = &ensureNumericInput,
 *    .action = &addToBuffer,
 *    .nextState = &awaitingInputState,
 * },
 * ~~~
 * - A guarded transition using a condition
 * ~~~{.c}
 * {
 *    .eventType = Event_mouse,
 *    .condition = boxLimits,
 *    .guard = &coordinatesWithinLimits,
 * },
 * ~~~
 * By using \ref #condition "conditions" a more general guard function can be
 * used, operating on the supplied argument #condition. In this example,
 * `coordinatesWithinLimits` checks whether the coordinates in the mouse event
 * are within the limits of the "box".
 *
 * \sa event
 * \sa state
 */
typedef struct sm_transition_s {
    /** \brief The event that will trigger this transition. */
    int eventType;
    /**
     * \brief Condition that event must fulfil
     *
     * This variable will be passed to the #guard (if #guard is non-NULL) and
     * may be used as a condition that the incoming event's data must fulfil in
     * order for the transition to be performed. By using this variable, the
     * number of #guard functions can be minimised by making them more general.
     */
    void *condition;
    /**
     * \brief Check if data passed with event fulfils a condition
     *
     * A transition may be conditional. If so, this function, if non-NULL, will
     * be called. Its first argument will be supplied with #condition, which
     * can be compared against the \ref event::data "payload" in the #event.
     * The user may choose to use this argument or not. Only if the result is
     * true, the transition will take place.
     *
     * \param condition event (data) to compare the incoming event against.
     * \param event the event passed to the state machine.
     *
     * \returns true if the event's data fulfils the condition, otherwise false.
     */
    bool (*guard)(void *condition, struct sm_event_s *event);

    //TODO: 返回l_err
    /**
     * \brief Function containing tasks to be performed during the transition
     *
     * The transition may optionally do some work in this function before
     * entering the next state. May be NULL.
     *
     * \param currentStateData the leaving state's \ref state::data "data"
     * \param event the event passed to the state machine.
     * \param newStateData the new state's (the \ref state::entryState
     * "entryState" of any (chain of) parent states, not the parent state
     * itself) \ref state::data "data"
     */
    void (*action)(void *currentStateData, struct sm_event_s *event,
                   void *newStateData);


    /**
     * \brief The next state
     *
     * This must point to the next state that will be entered. It cannot be
     * NULL. If it is, the state machine will detect it and enter the \ref
     * stateMachine::errorState "error state".
     */
    struct sm_state_s *nextState;
} sm_transition_t;

/**
 * \brief State
 *
 * The current state in a state machine moves to a new state when one of the
 * #transitions in the current state triggers on an event. An optional \ref
 * #exitAction "exit action" is called when the state is left, and an \ref
 * #entryAction "entry action" is called when the state machine enters a new
 * state. If a state returns to itself, neither #exitAction nor #entryAction
 * will be called. An optional \ref transition::action "transition action" is
 * called in either case.
 *
 * States may be organised in a hierarchy by setting \ref #parentState
 * "parent states". When a group/parent state is entered, the state machine is
 * redirected to the group state's \ref #entryState "entry state" (if
 * non-NULL). If an event does not trigger a transition in a state and if the
 * state has a parent state, the event will be passed to the parent state.
 * This behaviour is repeated for all parents. Thus all children of a state
 * have a set of common #transitions. A parent state's #entryAction will not
 * be called if an event is passed on to a child state.
 *
 * The following lists the different types of states that may be created, and
 * how to create them:
 *
 * ### Normal state ###
 * ~~~{.c}
 * struct state normalState = {
 *    .parentState = &groupState,
 *    .entryState = NULL,
 *    .transition = (struct transition[]){
 *       { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar,
 *          NULL, &msgReceivedState },
 *    },
 *    .numTransitions = 1,
 *    .data = normalStateData,
 *    .entryAction = &doSomething,
 *    .exitAction = &cleanUp,
 * };
 * ~~~
 * In this example, `normalState` is a child of `groupState`, but the
 * #parentState value may also be NULL to indicate that it is not a child of
 * any group state.
 *
 * ### Group/parent state ###
 * A state becomes a group/parent state when it is linked to by child states
 * by using #parentState. No members in the group state need to be set in a
 * particular way. A parent state may also have a parent.
 * ~~~{.c}
 * struct state groupState = {
 *    .entryState = &normalState,
 *    .entryAction = NULL,
 * ~~~
 * If there are any transitions in the state machine that lead to a group
 * state, it makes sense to define an entry state in the group. This can be
 * done by using #entryState, but it is not mandatory. If the #entryState
 * state has children, the chain of children will be traversed until a child
 * with its #entryState set to NULL is found.
 *
 * \note If #entryState is defined for a group state, the group state's
 * #entryAction will not be called (the state pointed to by #entryState (after
 * following the chain of children), however, will have its #entryAction
 * called).
 *
 * \warning The state machine cannot detect cycles in parent chains and
 * children chains. If such cycles are present, stateM_handleEvent() will
 * never finish due to never-ending loops.
 *
 * ### Final state ###
 * A final state is a state that terminates the state machine. A state is
 * considered as a final state if its #numTransitions is 0:
 * ~~~{.c}
 * struct state finalState = {
 *    .transitions = NULL,
 *    .numTransitions = 0,
 * ~~~
 * The error state used by the state machine to indicate errors should be a
 * final state. Any calls to stateM_handleEvent() when the current state is a
 * final state will return #stateM_noStateChange.
 *
 * \sa event
 * \sa transition
 */
typedef struct sm_state_s {
    /**
     * \brief If the state has a parent state, this pointer must be non-NULL.
     */
    struct sm_state_s *parentState;
    /**
     * \brief If this state a is a parent state, this pointer may point to a
     * child state that servess an entry point.
     */
    struct sm_state_s *entryState;
    /**
     * \brief An array of transitions for the state.
     */
    struct sm_transition_s *transitions;
    /**
     * \brief Number of transitions in the #transitions array.
     */
    size_t numTransitions;
    /**
     * \brief Data that will be available for the state in its #entryAction and
     * #exitAction, and in any \ref transition::action "transition action"
     */
    void *data;
    /**
     * \brief This function is called whenever the state is being entered. May
     * be NULL.
     *
     * \note If a state returns to itself through a transition (either directly
     * or through a parent/group sate), its #entryAction will not be called.
     *
     * \note A group/parent state with its #entryState defined will not have
     * its #entryAction called.
     *
     * \param stateData the state's #data will be passed.
     * \param event the event that triggered the transition will be passed.
     */
    l_err (*entryAction)(void *stateData, struct sm_event_s *event);


    //TODO: 返回l_err
    /**
     * \brief This function is called whenever the state is being left. May be
     * NULL.
     *
     * \note If a state returns to itself through a transition (either directly
     * or through a parent/group sate), its #exitAction will not be called.
     *
     * \param stateData the state's #data will be passed.
     * \param event the event that triggered a transition will be passed.
     */
    l_err (*exitAction)(void *stateData, struct sm_event_s *event);
} sm_state_t;

/**
 * \brief State machine
 *
 * There is no need to manipulate the members directly.
 */
typedef struct sm_statemachine_s {
    /** \brief Pointer to the current state */
    struct sm_state_s *currentState;
    /**
     * \brief Pointer to previous state
     *
     * The previous state is stored for convenience in case the user needs to
     * keep track of previous states.
     */
    struct sm_state_s *previousState;
    /**
     * \brief Pointer to a state that will be entered whenever an error occurs
     * in the state machine.
     *
     * See #stateM_errorStateReached for when the state machine enters the
     * error state.
     */
    struct sm_state_s *errorState;
} sm_statemachine_t;

/**
 * \brief Initialise the state machine
 *
 * This function initialises the supplied stateMachine and sets the current
 * state to \pn{initialState}. No actions are performed until
 * stateM_handleEvent() is called. It is safe to call this function numerous
 * times, for instance in order to reset/restart the state machine if a final
 * state has been reached.
 *
 * \note The \ref #state::entryAction "entry action" for \pn{initialState}
 * will not be called.
 *
 * \note If \pn{initialState} is a parent state with its \ref
 * state::entryState "entryState" defined, it will not be entered. The user
 * must explicitly set the initial state.
 *
 * \param stateMachine the state machine to initialise.
 * \param initialState the initial state of the state machine.
 * \param errorState pointer to a state that acts a final state and notifies
 * the system/user that an error has occurred.
 */
void stateM_init(struct sm_statemachine_s *stateMachine,
                 struct sm_state_s *initialState, struct sm_state_s *errorState);

/**
 * \brief Pass an event to the state machine
 *
 * The event will be passed to the current state, and possibly to the current
 * state's parent states (if any). If the event triggers a transition, a new
 * state will be entered. If the transition has an \ref transition::action
 * "action" defined, it will be called. If the transition is to a state other
 * than the current state, the current state's \ref state::exitAction
 * "exit action" is called (if defined). Likewise, if the state is a new
 * state, the new state's \ref state::entryAction "entry action" is called (if
 * defined).
 *
 * The returned value is negative if an error occurs.
 *
 * \param stateMachine the state machine to pass an event to.
 * \param event the event to be handled.
 *
 * \return #stateM_handleEventRetVals
 */
l_err stateM_handleEvent(struct sm_statemachine_s *stateMachine,
                         struct sm_event_s *event);

/**
 * \brief Get the current state
 *
 * \param stateMachine the state machine to get the current state from.
 *
 * \retval a pointer to the current state.
 * \retval NULL if \pn{stateMachine} is NULL.
 */
struct sm_state_s *stateM_currentState(struct sm_statemachine_s *stateMachine);

/**
 * \brief Get the previous state
 *
 * \param stateMachine the state machine to get the previous state from.
 *
 * \retval the previous state.
 * \retval NULL if \pn{stateMachine} is NULL.
 * \retval NULL if there has not yet been any transitions.
 */
struct sm_state_s *stateM_previousState(struct sm_statemachine_s *stateMachine);

/**
 * \brief Check if the state machine has stopped
 *
 * \param stateMachine the state machine to test.
 *
 * \retval true if the state machine has reached a final state.
 * \retval false if \pn{stateMachine} is NULL or if the current state is not a
 * final state.
 */
bool stateM_stopped(struct sm_statemachine_s *stateMachine);

l_err change_state(sm_statemachine_t *fsm, int ev_type, fsm_event_data_t *to_state);

bool in_state(sm_statemachine_t *fsm, const char *dest_state);

static bool default_guard(void *condition, struct sm_event_s *event) {
    fsm_event_data_t *ev_data = event->data;

    return !strcmp(condition, ev_data->fev->state_str);
}

static l_err sm_default_entry_action(void *stateData, struct sm_event_s *event) {
    fsm_event_data_t *ev_data = event->data;
    fsm_event_t *evi = ev_data->fev;
    if (evi->entry_event != NULL) {
        return evi->entry_event(ev_data->args);
    }
    return LD_OK;
}

static l_err sm_default_exit_action(void *statedata, struct sm_event_s *event) {
    // const char *statename = (const char *) statedata;
    fsm_event_data_t *ev_data = event->data;
    fsm_event_t *evi = ev_data->fev;
    if (evi->exit_event != NULL) {
        return evi->exit_event(ev_data->args);
    }
    return LD_OK;
}


#endif //LD_STATEMACHINE_H

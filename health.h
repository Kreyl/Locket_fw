#ifndef health_h
#define health_h
#ifdef __cplusplus
extern "C" {
#endif
#include "qhsm.h"    /* include own framework tagunil version */

//Start of h code from diagram
#define NORMAL 2
#define DANGER 1
#define DEAD 0
#define FLASH_MS 500
#define SHORT_VIBRO_MS 200
#define VIBRO_MS 300
#define DANGER_TO_S 1200
#define HP_THRESH 10
#define DEATH_TO_S 30
#define SHINE_FLASH_S 300
#define DEFAULT_HP  50
#define DANGER_DELAY_S 10
//End of h code from diagram


typedef struct {
/* protected: */
    QHsm super;

/* public: */
    unsigned int CharHP;
    unsigned int MaxHP;
    unsigned int DefaultHP;
    unsigned int DangerTime;
    unsigned int DeathTimer;
    unsigned int ShineTimer;
    unsigned int DangerDelay;
    QStateHandler StartState;
} Health;

/* protected: */
QState Health_initial(Health * const me, QEvt const * const e);
QState Health_global(Health * const me, QEvt const * const e);
QState Health_health(Health * const me, QEvt const * const e);
QState Health_dead(Health * const me, QEvt const * const e);
QState Health_just_dead(Health * const me, QEvt const * const e);
QState Health_wait_reset(Health * const me, QEvt const * const e);
QState Health_alive(Health * const me, QEvt const * const e);
QState Health_shining(Health * const me, QEvt const * const e);
QState Health_danger(Health * const me, QEvt const * const e);
QState Health_normal(Health * const me, QEvt const * const e);

#ifdef DESKTOP
QState Health_final(Health * const me, QEvt const * const e);
#endif /* def DESKTOP */

/*$enddecl${SMs::Health} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
__attribute__((__unused__))
static Health health; /* the only instance of the Health class */

typedef struct healthQEvt {
    QEvt super;
    unsigned int value;
} healthQEvt;

enum PlayerSignals {

TICK_SEC_SIG = Q_USER_SIG,

SHINE_RCVD_SIG,
DMG_RCVD_SIG,
SHINE_SIG,
SHINE_ORDER_SIG,

FIRST_BUTTON_PRESSED_SIG,
CENTRAL_BUTTON_PRESSED_SIG,
TIME_TICK_1S_SIG,
TIME_TICK_1M_SIG,

HEAL_SIG,
RESET_SIG,
DISABLE_SIG,
DIE_SIG,

PILL_HEAL_SIG,
PILL_RESET_SIG,
PILL_TAILOR_SIG,
PILL_STALKER_SIG,
PILL_LOCAL_SIG,
PILL_MUTANT_SIG,

TERMINATE_SIG,
LAST_USER_SIG
};
extern QHsm * const the_health; /* opaque pointer to the health HSM */

/*$declare${SMs::Health_ctor} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*${SMs::Health_ctor} ......................................................*/
void Health_ctor(
    unsigned int State,
    unsigned int HP,
    unsigned int MaxHP,
    unsigned int DangerTime);
/*$enddecl${SMs::Health_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#ifdef __cplusplus
}
#endif
#endif /* health_h */

#include "qhsm.h"
#include "player_type.h"
#include "eventHandlers.h"
#include <stdint.h>
#include "Glue.h"

//Q_DEFINE_THIS_FILE
/* global-scope definitions -----------------------------------------*/
QHsm * const the_player_type = (QHsm *) &player_type; /* the opaque pointer */


/*tranlated from diagrams code*/

//Start of c code from diagram
//stub code
unsigned int TailorHP = TAILOR_HP;
unsigned int LocalHP = LOCAL_HP;
unsigned int StalkerHP = STALKER_HP;
unsigned int HealAmount = 1;
//End of c code from diagram

/*${SMs::Player_type_ctor} .................................................*/
void Player_type_ctor(unsigned int State, Health* HealthSM)
{
    Player_type *me = &player_type;

     me->TimerRegen = 0;
     me->CharHealth = HealthSM;
     me->Timer = 0;
        switch (State) {
            case DEAD: {
                me->StartState =
                (QStateHandler)&Player_type_dead;
                break;
            }
            case LOCAL: {
                me->StartState =
                (QStateHandler)&Player_type_regenerating;
                break;
            }
            case TAILOR: {
                me->StartState =
                (QStateHandler)&Player_type_tailor;
                break;

            }
            case STALKER: {
                me->StartState =
                (QStateHandler)&Player_type_regenerating;
                break;
            }
            default:
                me->StartState =
                (QStateHandler)&Player_type_dead;
        }
    QHsm_ctor(&me->super, Q_STATE_CAST(&Player_type_initial));
}
/*$enddef${SMs::Player_type_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${SMs::Player_type} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*${SMs::Player_type} ......................................................*/
/*${SMs::Player_type::SM} ..................................................*/
QState Player_type_initial(Player_type * const me, QEvt const * const e) {
    /*${SMs::Player_type::SM::initial} */
    return Q_TRAN(me->StartState);
    return Q_TRAN(&Player_type_dead);
}
/*${SMs::Player_type::SM::global} ..........................................*/
QState Player_type_global(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {

        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type} .............................*/
QState Player_type_player_type(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state player_type\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state player_type\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::PILL_TAILOR} */
        case PILL_TAILOR_SIG: {
        	MaxHP_Update(me->CharHealth, TailorHP);
        	SIMPLE_DISPATCH(the_health, RESET);
        	SIMPLE_DISPATCH(the_ability, RESET);
            status_ = Q_TRAN(&Player_type_tailor);
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::PILL_STALKER} */
        case PILL_STALKER_SIG: {
        	MaxHP_Update(me->CharHealth, StalkerHP);
        	PlayerType_Save(STALKER);
        	Flash(255, 0, 0, FLASH_MS);
        	SIMPLE_DISPATCH(the_health, RESET);
        	SIMPLE_DISPATCH(the_ability, RESET);
            status_ = Q_TRAN(&Player_type_regenerating);
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::PILL_LOCAL} */
        case PILL_LOCAL_SIG: {
        	MaxHP_Update(me->CharHealth, LocalHP);
        	PlayerType_Save(LOCAL);
        	Flash(0, 0, 255, FLASH_MS);
        	SIMPLE_DISPATCH(the_health, RESET);
        	SIMPLE_DISPATCH(the_ability, RESET);
            status_ = Q_TRAN(&Player_type_regenerating);
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_global);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type::alive} ......................*/
QState Player_type_alive(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::alive} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state alive\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        case DIE_SIG: {
            status_ = Q_TRAN(&Player_type_dead);
            break;
        }
        case PILL_RESET_SIG: {
            SIMPLE_DISPATCH(the_health, RESET);
            SIMPLE_DISPATCH(the_ability, RESET);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state alive\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_player_type);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type::alive::local} ...............*/
QState Player_type_may_regen(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::alive::local} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state local\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state local\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_alive);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type::alive::local::normal} .......*/
QState Player_type_normal(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::alive::local::normal} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state normal\n");
            #endif /* def DESKTOP */
		    me->TimerRegen = 0;
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::normal} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state normal\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::normal::DMG_RCVD} */
        case DMG_RCVD_SIG: {
            me->TimerRegen = 0;
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::normal::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
            /*${SMs::Player_type::SM::global::player_type::alive::local::normal::TIME_TICK_1S::[me->TimerRegen>RegenThresh];]} */
            if (me->TimerRegen > REGEN_THRESH_S) {
                status_ = Q_TRAN(&Player_type_regenerating);
            }
            else {
                 me->TimerRegen++;
                 status_ = Q_HANDLED();
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_may_regen);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type::alive::local::regenerating} .*/
QState Player_type_regenerating(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::alive::local::regenerating} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state regenerating\n");
            #endif /* def DESKTOP */
            me->Timer = 0;
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::regenerating} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state regenerating\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::regenerating::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
        	me->Timer ++;
        	if (me->Timer == 10) {
                player_typeQEvt new_e;
                new_e.super.sig = HEAL_SIG;
                new_e.value = HealAmount;
                QMSM_DISPATCH(the_health, (QEvt *)&new_e);
        	    me->Timer = 0;
        	}
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::local::regenerating::DMG_RCVD} */
        case DMG_RCVD_SIG: {
            status_ = Q_TRAN(&Player_type_normal);
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_may_regen);
            break;
        }
    }
    return status_;
}
/*${SMs::Player_type::SM::global::player_type::alive::tailor} ..............*/
QState Player_type_tailor(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::alive::tailor} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state tailor\n");
            #endif /* def DESKTOP */
			Flash(255, 255, 0, FLASH_MS);
            PlayerType_Save(TAILOR);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::alive::tailor} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state tailor\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_alive);
            break;
        }
    }
    return status_;
}

/*${SMs::Player_type::SM::global::player_type::dead} .......................*/
QState Player_type_dead(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::global::player_type::dead} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state dead\n");
            #endif /* def DESKTOP */
            PlayerType_Save(DEAD);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Player_type::SM::global::player_type::dead} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state dead\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Player_type_player_type);
            break;
        }
    }
    return status_;
}

#ifdef DESKTOP
/*${SMs::Player_type::SM::final} ...........................................*/
QState Player_type_final(Player_type * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Player_type::SM::final} */
        case Q_ENTRY_SIG: {
            printf("Bye! Bye!");
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
#endif /* def DESKTOP */

/*$enddef${SMs::Player_type} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


#include "qhsm.h"
#include "ability.h"
#include "eventHandlers.h"
#include <stdint.h>
#include "Glue.h"

//Q_DEFINE_THIS_FILE
/* global-scope definitions -----------------------------------------*/
QHsm * const the_ability = (QHsm *) &ability; /* the opaque pointer */


/*translated from diagrams code*/

//Start of c code from diagram
unsigned int ChargeTO = CHARGE_TO_S;
//End of c code from diagram

void Ability_ctor(unsigned int State, unsigned int ChargeTime) {
    Ability *me = &ability;
        me->ChargeTime = ChargeTime;
        switch (State) {
            case NONE: {
                me->StartState = (QStateHandler)&Ability_none;
                break;
            }
            case MUTANT_READY: {
                me->StartState =
                (QStateHandler)&Ability_ready;
                break;
            }
            case MUTANT_CHARGING: {
                me->StartState =
                (QStateHandler)&Ability_charging;
                break;

            }
            case DISABLED: {
                me->StartState =
                (QStateHandler)&Ability_disabled;
                break;
            }
            default:
                me->StartState =
                (QStateHandler)&Ability_none;
        }
    QHsm_ctor(&me->super, Q_STATE_CAST(&Ability_initial));
}
/*$enddef${SMs::Ability_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${SMs::Ability} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*${SMs::Ability} ..........................................................*/
/*${SMs::Ability::SM} ......................................................*/
QState Ability_initial(Ability * const me, QEvt const * const e) {
    /*${SMs::Ability::SM::initial} */
    return Q_TRAN(me->StartState);
    return Q_TRAN(&Ability_ability);
}
/*${SMs::Ability::SM::global} ..............................................*/
QState Ability_global(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {

        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability} .....................................*/
QState Ability_ability(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state ability\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state ability\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::DISABLE} */
        case DISABLE_SIG: {
            status_ = Q_TRAN(&Ability_disabled);
            break;
        }
        /*${SMs::Ability::SM::global::ability::RESET} */
        case RESET_SIG: {
            status_ = Q_TRAN(&Ability_none);
            break;
        }
        default: {
            status_ = Q_SUPER(&Ability_global);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability::none} ...............................*/
QState Ability_none(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability::none} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state none\n");
            #endif /* def DESKTOP */
    		Ability_Save(NONE);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::none} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state none\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::none::PILL_MUTANT} */
        case PILL_MUTANT_SIG: {
            status_ = Q_TRAN(&Ability_ready);
            break;
        }
        default: {
            status_ = Q_SUPER(&Ability_ability);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability::disabled} ...........................*/
QState Ability_disabled(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability::disabled} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state disabled\n");
            #endif /* def DESKTOP */
			Ability_Save(DISABLED);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::disabled} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state disabled\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Ability_ability);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability::mutant} .............................*/
QState Ability_mutant(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability::mutant} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state mutant\n");
            #endif /* def DESKTOP */
            Flash(255, 255, 255, FLASH_MS);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state mutant\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Ability_ability);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability::mutant::ready} ......................*/
QState Ability_ready(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability::mutant::ready} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state ready\n");
            #endif /* def DESKTOP */
            Ability_Save(MUTANT_READY);
            ChargeTime_Update(me, 0);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::ready} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state ready\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::ready::FIRST_BUTTON_PRESSED} */
        case CENTRAL_BUTTON_PRESSED_SIG: {
            Flash(127, 127, 127, FLASH_MS);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::ready::SHINE} */
        case SHINE_SIG: {
        	ChargeTime_Update(me, 0);
            SendShining();
            status_ = Q_TRAN(&Ability_charging);
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::ready::SHINE_ORDER} */
        case SHINE_ORDER_SIG: {
        	ChargeTime_Update(me, 0);
        	SendShining();
            status_ = Q_TRAN(&Ability_charging);
            break;
        }
        default: {
            status_ = Q_SUPER(&Ability_mutant);
            break;
        }
    }
    return status_;
}
/*${SMs::Ability::SM::global::ability::mutant::charging} ...................*/
QState Ability_charging(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::global::ability::mutant::charging} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state charging\n");
            #endif /* def DESKTOP */
			Ability_Save(MUTANT_CHARGING);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::charging} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state charging\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::charging::FIRST_BUTTON_PRESSED} */
        case CENTRAL_BUTTON_PRESSED_SIG: {
            Flash(0, 0, 0, FLASH_MS);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Ability::SM::global::ability::mutant::charging::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
            /*${SMs::Ability::SM::global::ability::mutant::charging::TIME_TICK_1S::[me->ChargeTime>ChargeTO]} */
            if (me->ChargeTime > ChargeTO) {
                status_ = Q_TRAN(&Ability_ready);
            }
            else {
                me->ChargeTime++;
                status_ = Q_HANDLED();
            }
            break;
        }
		case TIME_TICK_1M_SIG: {
			ChargeTime_Save(me->ChargeTime);
			status_ = Q_HANDLED();
			break;
		}
        default: {
            status_ = Q_SUPER(&Ability_mutant);
            break;
        }
    }
    return status_;
}

#ifdef DESKTOP
/*${SMs::Ability::SM::final} ...............................................*/
QState Ability_final(Ability * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Ability::SM::final} */
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

/*$enddef${SMs::Ability} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/



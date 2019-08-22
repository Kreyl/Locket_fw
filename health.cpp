#include "qhsm.h"
#include "health.h"
#include "eventHandlers.h"
#include <stdint.h>
#include "Glue.h"
#include "Shell.h"


//#include "stdio.h"
//#define DESKTOP
//Q_DEFINE_THIS_FILE
/* global-scope definitions -----------------------------------------*/
QHsm * const the_health = (QHsm *) &health; /* the opaque pointer */

/*tranlated from diagrams code*/

//this code is for stubs only
unsigned int DangerTO = DANGER_TO_S;
unsigned int DefaultHP = DEFAULT_HP;
unsigned int HPThresh = HP_THRESH;
unsigned int DangerDelay = DANGER_DELAY_S;
//End of c code from diagramH

void Health_ctor(unsigned int State,
    unsigned int HP,
    unsigned int MaxHP,
    unsigned int DangerTime)
{
    Health *me = &health;
        me->CharHP = HP;
        me->MaxHP = MaxHP;
        me->DefaultHP = DefaultHP;
        me->DangerTime = 0;
        me->DeathTimer = 0;
        me->DangerDelay = 0;
        me->ShineTimer = 0;
        switch (State) {
            case NORMAL: {
                me->StartState =
                (QStateHandler)&Health_normal;
                break;
            }
            case DANGER: {
                me->StartState =
                (QStateHandler)&Health_danger;
                break;
            }
            case DEAD: {
                me->StartState =
                (QStateHandler)&Health_wait_reset;
                break;

            }
            default:
                me->StartState =
                (QStateHandler)&Health_wait_reset;
        }
    QHsm_ctor(&me->super, Q_STATE_CAST(&Health_initial));
}
/*$enddef${SMs::Health_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${SMs::Health} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*${SMs::Health} ...........................................................*/
/*${SMs::Health::SM} .......................................................*/
QState Health_initial(Health * const me, QEvt const * const e) {
    /*${SMs::Health::SM::initial} */
    return Q_TRAN(me->StartState);
    return Q_TRAN(&Health_wait_reset);
}
/*${SMs::Health::SM::global} ...............................................*/
QState Health_global(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {

       default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health} .......................................*/
QState Health_health(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state health\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state health\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::PILL_RESET} */
        case RESET_SIG: {
            Reset(me);
            status_ = Q_TRAN(&Health_normal);
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_global);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::dead} .................................*/
QState Health_dead(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::dead} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state dead\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::dead} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state dead\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_health);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::dead::just_dead} ......................*/
QState Health_just_dead(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::dead::just_dead} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state just_dead\n");
            #endif /* def DESKTOP */
            Player_Died(me);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::dead::just_dead} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state just_dead\n");
            #endif /* def DESKTOP */
            SetDefaultColor(0, 0, 0);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::dead::just_dead::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
            /*${SMs::Health::SM::global::health::dead::just_dead::TIME_TICK_1S::[me->DeathTimer>DEATH_TO_S]} */
            if (me->DeathTimer > DEATH_TO_S) {
                status_ = Q_TRAN(&Health_wait_reset);
            }
            /*${SMs::Health::SM::global::health::dead::just_dead::TIME_TICK_1S::[else]} */
            else {
                me->DeathTimer++;
                status_ = Q_HANDLED();
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_dead);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::dead::wait_reset} .....................*/
QState Health_wait_reset(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::dead::wait_reset} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state wait_reset\n");
            #endif /* def DESKTOP */
			me->DeathTimer = 0;
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::dead::wait_reset} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state wait_reset\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::dead::wait_reset::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
            FlashDeath();
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_dead);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::alive} ................................*/
QState Health_alive(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::alive} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state alive\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state alive\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::SHINE_RCVD} */
        case SHINE_RCVD_SIG: {
             Vibro(VIBRO_MS);
             status_ = Q_TRAN(&Health_shining);
             break;
        }
        /*${SMs::Health::SM::global::health::alive::CENTRAL_BUTTON_PRESSED} */
        case FIRST_BUTTON_PRESSED_SIG: {
             ShowHP(me);
             status_ = Q_HANDLED();
             break;
        }
        default: {
            status_ = Q_SUPER(&Health_health);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::shining} ................................*/
QState Health_shining(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::shining} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state shining\n");
            #endif /* def DESKTOP */
//            Printf("Shine enter\n");
            me->ShineTimer = 0;
            Vibro(10*VIBRO_MS);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::shining} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state shining\n");
            #endif /* def DESKTOP */
//            Printf("Shine exit\n");
            status_ = Q_HANDLED();
            Vibro(10*VIBRO_MS);
            break;
        }
        /*${SMs::Health::SM::global::health::shining::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
        	if (me->ShineTimer <= SHINE_FLASH_S) {
                Flash(255, 0, 255, 1001);
                me->ShineTimer++;
                status_ = Q_HANDLED();
            } else {
            	if (me->CharHP <= me->MaxHP/3) {
            		status_ = Q_TRAN(&Health_danger);
            	} else {
            		status_ = Q_TRAN(&Health_normal);
            	}
            }
            break;
        }
        /*${SMs::Health::SM::global::health::alive::CENTRAL_BUTTON_PRESSED} */
        default: {
            status_ = Q_SUPER(&Health_health);
            break;
        }
    }
    return status_;
}

/*${SMs::Health::SM::global::health::alive::danger} ........................*/
QState Health_danger(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::alive::danger} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state danger\n");
            #endif /* def DESKTOP */
			State_Save(DANGER);
            Vibro(10*VIBRO_MS);
            me->DangerDelay = 0;
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::danger} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state danger\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::danger::PILL_HEAL} */
        case PILL_HEAL_SIG: {
            HP_Update(me, me->MaxHP);
            Flash(255, 165, 0, FLASH_MS);
            status_ = Q_TRAN(&Health_normal);
            break;
        }
        /*${SMs::Health::SM::global::health::alive::danger::HEAL} */
        case HEAL_SIG: {
            /*${SMs::Health::SM::global::health::alive::danger::HEAL::[me->CharHP+((HealthQEvt*)e->val~} */
        	if (me->CharHP + ((healthQEvt*)e)->value  >=  HPThresh) {
                HP_Update(me, me->CharHP + ((healthQEvt*)e)->value);
                status_ = Q_TRAN(&Health_normal);
            }
            /*${SMs::Health::SM::global::health::alive::danger::HEAL::[else]} */
            else {
                HP_Update(me, me->CharHP + ((healthQEvt*)e)->value);
                status_ = Q_HANDLED();
            }
            break;
        }
        /*${SMs::Health::SM::global::health::alive::danger::TIME_TICK_1S} */
        case TIME_TICK_1S_SIG: {
            /*${SMs::Health::SM::global::health::alive::danger::TIME_TICK_1S::[me->danger_time>DangerTO]} */
            if (me->DangerTime > DangerTO) {
                status_ = Q_TRAN(&Health_just_dead);
            }
            /*${SMs::Health::SM::global::health::alive::danger::TIME_TICK_1S::[else]} */
            else {
                me->DangerTime++;
                me->DangerDelay++;
                FlashAgony();
                status_ = Q_HANDLED();
            }
            break;
        }
		 case TIME_TICK_1M_SIG: {
			 DangerTime_Save(me->DangerTime);
			 status_ = Q_HANDLED();
			 break;
		 }
        /*${SMs::Health::SM::global::health::alive::danger::DMG_RCVD} */
        case DMG_RCVD_SIG: {
            /*${SMs::Health::SM::global::health::alive::danger::DMG_RCVD::[me->CharHP-((HealthQEvt*)e)->va~} */
            if (me->DangerDelay > DangerDelay) {
            	Vibro(VIBRO_MS);
        	    if ((me->CharHP <= ((healthQEvt*)e)->value)) {
                    status_ = Q_TRAN(&Health_just_dead);
                }
                /*${SMs::Health::SM::global::health::alive::danger::DMG_RCVD::[else]} */
               else {
                    HP_Update(me, me->CharHP - ((healthQEvt*)e)->value);
                    status_ = Q_HANDLED();
               }
            } else {
            	status_ = Q_HANDLED();
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_alive);
            break;
        }
    }
    return status_;
}
/*${SMs::Health::SM::global::health::alive::normal} ........................*/
QState Health_normal(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::global::health::alive::normal} */
        case Q_ENTRY_SIG: {
            #ifdef DESKTOP
                printf("Entered state normal\n");
            #endif /* def DESKTOP */
			State_Save(NORMAL);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::normal} */
        case Q_EXIT_SIG: {
            #ifdef DESKTOP
                printf("Exited state normal\n");
            #endif /* def DESKTOP */
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::normal::PILL_HEAL} */
        case PILL_HEAL_SIG: {
            HP_Update(me, me->MaxHP);
            Flash(255, 165, 0, FLASH_MS);
            status_ = Q_HANDLED();
            break;
        }
        case HEAL_SIG: {
            HP_Update(me, me->CharHP + ((healthQEvt*)e)->value);
            status_ = Q_HANDLED();
            break;
        }
        /*${SMs::Health::SM::global::health::alive::normal::DMG_RCVD} */
        case DMG_RCVD_SIG: {
            /*${SMs::Health::SM::global::health::alive::normal::DMG_RCVD::[me->CharHP-((HealthQEvt*)e)->va~} */
        	IndicateDamage(me);
        	if (me->CharHP - ((healthQEvt*)e)->value <= me->MaxHP/3) {
                HP_Update(me, me->CharHP - ((healthQEvt*)e)->value);
                DangerTime_Update(me, 0);
                status_ = Q_TRAN(&Health_danger);
            }
            /*${SMs::Health::SM::global::health::alive::normal::DMG_RCVD::[else]} */
            else {
                HP_Update(me, me->CharHP - ((healthQEvt*)e)->value);
                status_ = Q_HANDLED();
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Health_alive);
            break;
        }
    }
    return status_;
}

#ifdef DESKTOP
/*${SMs::Health::SM::final} ................................................*/
QState Health_final(Health * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*${SMs::Health::SM::final} */
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

/*$enddef${SMs::Health} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


/*
 * keys.cpp
 *
 *  Created on: 07.02.2013
 *      Author: kreyl
 */

#include "buttons.h"
#include "ch.h"
#include "uart.h"
#include "MsgQ.h"

#if BUTTONS_ENABLED

#if BTN_GETSTATE_REQUIRED
static PinSnsState ibtn_state[BUTTONS_CNT];
PinSnsState GetBtnState(uint8_t btn_indx) {
    if(btn_indx > BUTTONS_CNT) return pssNone;
    else return ibtn_state[btn_indx];
}
#endif

#if BTN_LONGPRESS
static bool is_long_press[BUTTONS_CNT];
static systime_t longpress_timer;
#endif
#if BTN_REPEAT
static bool is_repeating[BUTTONS_CNT];
static systime_t repeat_timer;
#endif
#if BTN_COMBO || BTN_LONG_COMBO
static bool is_combo;
#endif
#if BTN_LONG_COMBO
static systime_t longcombo_timer;
static bool is_long_combo;
#endif
#if BTN_DOUBLE_CLICK
static systime_t doubleclick_timer;
static bool is_waiting_second_click = false;
static uint8_t first_click_indx = 0;
#endif

static void AddEvtToQueue(BtnEvtInfo &evt);
static void AddEvtToQueue(BtnEvt atype, uint8_t btn_indx);

// ========================= Postprocessor for PinSns ==========================
void ProcessButtons(PinSnsState *btn_state, uint32_t len) {
//    Printf("%A\r", btn_state, len, ' ');
    for(uint8_t i=0; i<len; i++) {
#if BTN_GETSTATE_REQUIRED
        ibtn_state[i] = btn_state[i];
#endif
#if 1 // ==== Button Press ====
        if(btn_state[i] == BTN_PRESSING_STATE) {
#if BTN_LONGPRESS
            is_long_press[i] = false;
#endif
#if BTN_REPEAT
            is_repeating[i] = false;
#endif
#if BTN_COMBO || BTN_LONG_COMBO // Check if combo
            BtnEvtInfo IEvt;
            IEvt.BtnCnt = 0;
            for(uint8_t j=0; j<BUTTONS_CNT; j++) {
                if(btn_state[j] == BTN_HOLDDOWN_STATE or btn_state[j] == BTN_PRESSING_STATE) {
                    IEvt.btn_indx[IEvt.BtnCnt] = j;
                    IEvt.BtnCnt++;
                    if(j != i) is_combo = true;
                }
            } // for j
            if(IEvt.BtnCnt > 1) { // Combo
#if BTN_COMBO
                IEvt.Type = beCombo;
                AddEvtToQueue(IEvt);
#endif
#if BTN_LONG_COMBO
                // Restart long combo timer on new keypress
                longcombo_timer = chVTGetSystemTimeX();
#endif
                continue; // go to next button
            }
            else is_combo = false;
#endif // combo

#if BTN_DOUBLE_CLICK
            // Check if same button pressed within timeframe
            if(is_waiting_second_click and first_click_indx == i and chVTTimeElapsedSinceX(doubleclick_timer) < (TIME_MS2I(BTN_DOUBLECLICK_DELAY_MS))) {
                AddEvtToQueue(beDoubleClick, i);
                is_waiting_second_click = false;
            }
            else {
                AddEvtToQueue(beShortPress, i); // First click
                is_waiting_second_click = true;
                doubleclick_timer = chVTGetSystemTimeX();
                first_click_indx = i;
            }
#endif

// Single key pressed, no combo
#if BTN_SHORTPRESS && !BTN_DOUBLE_CLICK && !BTN_LONGPRESS
            AddEvtToQueue(beShortPress, i);  // Add single keypress
#endif



#if BTN_LONGPRESS
            longpress_timer = chVTGetSystemTimeX();
#endif
#if BTN_REPEAT
            repeat_timer = chVTGetSystemTimeX();
#endif
        } // if press
#endif

// ==== Button Release ====
#if BTN_COMBO || BTN_RELEASE || BTN_LONG_COMBO || (BTN_SHORTPRESS && BTN_LONGPRESS)
        else if(btn_state[i] == BTN_RELEASING_STATE) {
#if BTN_COMBO || BTN_LONG_COMBO // Check if combo completely released
            if(is_combo) {
                is_combo = false;
                for(uint8_t j=0; j<BUTTONS_CNT; j++) {
                    if(btn_state[j] == BTN_HOLDDOWN_STATE) {
                        is_combo = true;
                        break;
                    }
                }
#if BTN_LONG_COMBO
                if(!is_combo) is_long_combo = false;
#endif
                return; // do not send release evt (if enabled)
            } // if combo
#endif // BTN_COMBO || BTN_LONG_COMBO

#if BTN_RELEASE // Send evt if not combo
            AddEvtToQueue(beRelease, i);
#endif // BTN_RELEASE

#if BTN_SHORTPRESS && BTN_LONGPRESS
            if(!is_long_press[i]) AddEvtToQueue(beShortPress, i);  // Add single keypress
#endif
        }
#endif //BTN_COMBO || BTN_RELEASE || BTN_LONG_COMBO || (BTN_SHORTPRESS && BTN_LONGPRESS)

#if 1 // ==== Holddown ====
#if BTN_LONGPRESS || BTN_REPEAT || BTN_LONG_COMBO
        else if(btn_state[i] == BTN_HOLDDOWN_STATE) {
#if BTN_LONGPRESS // Check if long press
            if(!is_long_press[i]
#if BTN_COMBO || BTN_LONG_COMBO
                and !is_combo
#endif
            ) {
//                Uart.Printf("Elapsed %u\r", chVTTimeElapsedSinceX(longpress_timer));
                if(chVTTimeElapsedSinceX(longpress_timer) >= TIME_MS2I(BTN_LONGPRESS_DELAY_MS)) {
                    is_long_press[i] = true;
                    AddEvtToQueue(beLongPress, i);
                }
            }
#endif

#if BTN_LONG_COMBO
            if(is_combo and !is_long_combo) {
                if(chVTTimeElapsedSinceX(longcombo_timer) >= MS2ST(BTN_LONGPRESS_DELAY_MS)) {
                    is_long_combo = true;
                    BtnEvtInfo IEvt;
                    IEvt.BtnCnt = 0;
                    for(uint8_t j=0; j<BUTTONS_CNT; j++) {
                        if(btn_state[j] == BTN_HOLDDOWN_STATE or btn_state[j] == BTN_PRESSING_STATE) {
                            IEvt.btn_indx[IEvt.BtnCnt] = j;
                            IEvt.BtnCnt++;
                        }
                    } // for j
                    IEvt.Type = beLongCombo;
                    AddEvtToQueue(IEvt);
                } // if time elapsed
            } // if is combo
#endif

#if BTN_REPEAT // Check if repeat
            if(!is_repeating[i]) {
                if(TimeElapsed(&repeat_timer, BTN_DELAY_BEFORE_REPEAT_MS)) {
                    is_repeating[i] = true;
                    AddEvtToQueue(beRepeat, i);
                }
            }
            else {
                if(TimeElapsed(&repeat_timer, BTN_REPEAT_PERIOD_MS)) {
                    AddEvtToQueue(beRepeat, i);
                }
            }
#endif
        } // if still pressed
#endif // BTN_LONGPRESS || BTN_REPEAT
#endif
    } // for i
}

__unused
void AddEvtToQueue(BtnEvtInfo &evt) {
    EvtMsg_t msg(EvtId::Buttons);
    msg.btn_info = evt;
    evt_q_main.SendNowOrExit(msg);
}

void AddEvtToQueue(BtnEvt atype, uint8_t btn_indx) {
    EvtMsg_t msg(EvtId::Buttons);
    msg.btn_info.type = atype;
#if BTN_COMBO || BTN_LONG_COMBO
    msg.BtnEvtInfo.BtnCnt = 1;
    msg.BtnEvtInfo.btn_indx[0] = btn_indx;
#elif BUTTONS_CNT != 1
    msg.btn_info.btn_indx = btn_indx;
#endif
    evt_q_main.SendNowOrExit(msg);
}
#endif

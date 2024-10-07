#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "MsgQ.h"
#include "SimpleSensors.h"
#include "adcL151.h"

#include <vector>

// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t dbg_uart{&kCmdUartParams};
void OnCmd(Cmd_t *pcmd);
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
PinOutputPWM_t LedSingle {LED_B_PIN};

// IDs and channel
const uint8_t kRadioChnl = 2;
const uint8_t kLedBrt = 255;
// EE Addresses
const uint32_t kEeAddrDelay = 4;

static const PinInputSetup_t kDipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
//void ReadEE();

cc1101_t CC(CC_Setup0);
uint8_t pwr_lvl_id = 0;
rPkt_t rpkt;
const uint32_t kTheWord = 0xCa110fEa;
uint32_t tx_period = 162;
uint32_t rx_sleep_duration = 1530, rx_receive_dur = 180, rx_cycle_duration = 207;

static const char* kPwrNames[12] = {
        "-30dBm", "-27dBm", "-25dBm", "-20dBm", "-15dBm", "-10dBm", "-6dBm",
        "0dBm", "+5dBm", "+7dBm", "+10dBm", "+12dBm",
};

LedRGBChunk_t lsqOn[] =  { {csSetup, 450, clRed},    {csEnd} }; // Will be changed in RX
LedRGBChunk_t lsqOff[] = { {csSetup, 450, clBlack},  {csEnd} };

class RxTable {
private:
    struct IdRssi {
        uint32_t adding_cycle = 0;
        int32_t rssi = -207;
    };
    static const uint32_t kIdCnt = 4;
    IdRssi ids[kIdCnt];
    uint32_t curr_cycle = 0;
    bool led_is_initialyzed = false;
public:
    static const uint32_t kMaxCycleCnt = 9;

    void AddId(uint8_t aid, int8_t rssi) {
        if(aid >= kIdCnt) return;
        ids[aid].rssi = rssi;
        ids[aid].adding_cycle = curr_cycle;
    }

    retv Process() {
        // Find max rssi
        int32_t max_rssi = -180, id_max = -1;
        for(int32_t i=0; i<kIdCnt; i++) {
            uint32_t diff = curr_cycle - ids[i].adding_cycle;
            if(diff > kMaxCycleCnt) continue;
            if(ids[i].rssi > max_rssi) {
                max_rssi = ids[i].rssi;
                id_max = i;
            }
        }
        if(id_max == -1) return retv::NotFound;
        // Setup color
        switch(id_max) {
            case 0: lsqOn[0].Color = clRed;   break;
            case 1: lsqOn[0].Color = clGreen; break;
            case 2: lsqOn[0].Color = clBlue;  break;
            case 3: lsqOn[0].Color = clYellow; break;
        }
        if(!led_is_initialyzed) {
            led_is_initialyzed = true;
            Led.Init();
        }
        Led.StartOrContinue(lsqOn);
        return retv::Ok;
    }

    void IncCycle() { curr_cycle++; }
} rx_table;

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();
    // === Init OS ===
    halInit();
    chSysInit();

    // ==== Init hardware ====
    dbg_uart.Init();
    uint8_t b = GetDipSwitch();
    // Get mode
    bool mode_rx = b & 0b10000;
    if(!mode_rx) { // Read TX params
        Led.Init();
        // Select power
        pwr_lvl_id = b & 0b1111; // Remove high bits
        if(pwr_lvl_id > 11) pwr_lvl_id = 11;
        // Get id
        rpkt.id = (b >> 6) & 0b11;
        switch(rpkt.id) {
            case 0: Led.SetColor({kLedBrt, 0,       0}); break;
            case 1: Led.SetColor({0,       kLedBrt, 0}); break;
            case 2: Led.SetColor({0,       0,       kLedBrt}); break;
            case 3: Led.SetColor({kLedBrt, kLedBrt, 0}); break;
        }
    }

    if(Sleep::WasInStandby()) {
        if(mode_rx) Printf("RX\r");
        else Printf("TX id %u; %S\r", rpkt.id, kPwrNames[pwr_lvl_id]);
    }
    // Not in standby => just powered on
    else {
        if(mode_rx) {
            Led.Init();
            Led.StartOrRestart(lsqStart);
            Printf("\r%S RX %S; ch=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), kRadioChnl);
        }
        else { // mode tx
            Printf("\r%S %S; ch=%u; period=%u; id=%u\r", APP_NAME,
                XSTRINGIFY(BUILD_TIME), kRadioChnl, tx_period, rpkt.id);
        }
        Clk.PrintFreqs();
        // Measure battery
        chThdSleepMilliseconds(54);
        Adc.Init();
        Adc.StartMeasurementAndWaitCompletion(); // Skip this as bad one
        Adc.StartMeasurementAndWaitCompletion();
        uint32_t adc_v = Adc.GetResultMedian(0);
        Printf("VDDA = %u mV\r", Adc.GetVdda_mv(adc_v));

        // Try to receive Cmd by UART
        for(int i=0; i<27; i++) {
            chThdSleepMilliseconds(99);
            while(dbg_uart.GetByte(&b) == retv::Ok) {
                if(dbg_uart.Cmd.PutChar(b) == pdrNewCmd) {
                    OnCmd(&dbg_uart.Cmd);
                    i = 0;
                }
            } // while get byte
        } // for
    } // if WasInStandby

    if(CC.Init() == retv::Ok) {
        // Setup CC
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(kRadioChnl);
        CC.SetBitrate(CCBitrate100k);
        CC.SetTxPower(PwrTable[pwr_lvl_id]);
#if 1 // =================== RX =====================
        if(mode_rx) {
            int8_t rssi;
            while(true) {
                CC.Recalibrate();
                // Receive for rx_receive_dur ms
                systime_t start = chVTGetSystemTimeX();
                while(chVTTimeElapsedSinceX(start) < TIME_MS2I(rx_receive_dur)) {
                    if(CC.Receive(rx_receive_dur, reinterpret_cast<uint8_t*>(&rpkt), RPKT_LEN, &rssi) == retv::Ok) {
                        Printf("id=%d; Rssi=%d\r", rpkt.id, rssi);
                        if(rpkt.the_word == kTheWord) rx_table.AddId(rpkt.id, rssi);
                    }
                } // RX done

                if(rx_table.Process() == retv::NotFound) { // Nothing there, time to fade out
                    if(Led.IsOff()) break; // Go to sleep
                    else Led.StartOrContinue(lsqOff);
                }
                // When LED is active, let CC sleep for what left from cycle duration
                chThdSleepMilliseconds(rx_cycle_duration - rx_receive_dur);
                rx_table.IncCycle();
            } // while true
        }
#endif
#if 1 // ===================== TX ====================
        else { // Transmit
            rpkt.the_word = kTheWord;
            CC.Recalibrate();
            CC.Transmit(reinterpret_cast<uint8_t*>(&rpkt), RPKT_LEN);
        }
#endif
    }
    else { // CC failure
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(999);
    }

    // Enter sleep
    CC.EnterPwrDown();
    chSysLock();
    Iwdg::InitAndStart(mode_rx? rx_sleep_duration : tx_period);
    Sleep::EnterStandby();
    chSysUnlock();

    while(true); // Will never be here
}

//void ReadEE() {
//    delay = EE::Read32(kEeAddrDelay);
//    if(delay < 4 or delay > 306000) {
//        Printf("\rUsing default delay\r");
//        delay = 162;
//    }
//}

retv SetDelay(int32_t NewDelay) {
    retv rslt = EE::Write32(kEeAddrDelay, NewDelay);
    if(rslt == retv::Ok) {
        tx_period = NewDelay;
        return retv::Ok;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retv::Fail;
    }
}

#if 1 // ================= Command processing ====================
void Ack(int32_t Result) { Printf("Ack %d\r\n", Result); }

void OnCmd(Cmd_t *pcmd) {
    if(pcmd->NameIs("Ping")) dbg_uart.Ok();
    else if(pcmd->NameIs("Version")) Printf("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(pcmd->NameIs("GetDelay")) Printf("delay %u\r", tx_period);

    else if(pcmd->NameIs("SetDelay")) {
        int32_t NewDelay;
        if(pcmd->GetNext<int32_t>(&NewDelay) != retv::Ok) { dbg_uart.CmdError(); return; }
        if(SetDelay(NewDelay) == retv::Ok) dbg_uart.Ok();
        else dbg_uart.Failure();
    }

    else dbg_uart.CmdUnknown();
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(kDipSwPin[i].PGpio, kDipSwPin[i].Pin, kDipSwPin[i].PullUpDown);
    for(int i=0; i<DIP_SW_CNT; i++) {
        if(!PinIsHi(kDipSwPin[i].PGpio, kDipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(kDipSwPin[i].PGpio, kDipSwPin[i].Pin);
    }
    return Rslt;
}

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

#include <vector>

// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t dbg_uart{&kCmdUartParams};
void OnCmd(Cmd_t *pcmd);
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
PinOutputPWM_t LedSingle {LED_B_PIN};

// IDs and channel
const uint8_t kIdMin = 1, kIdMax = 254, kRadioChnl = 7;
// EE Addresses
const uint32_t kEeAddrId = 0, kEeAddrDelay = 4;

static const PinInputSetup_t kDipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
void ReadEE();

cc1101_t CC(CC_Setup0);
int32_t id;
uint8_t pwr_lvl_id = 0;
rPkt_t pkt_tx;
uint32_t delay;

static const char* kPwrNames[12] = {
        "-30dBm", "-27dBm", "-25dBm", "-20dBm", "-15dBm", "-10dBm", "-6dBm",
        "0dBm", "+5dBm", "+7dBm", "+10dBm", "+12dBm",
};

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
    ReadEE();
    if(Sleep::WasInStandby()) {
        // Init only one channel of LED
        LedSingle.Init();
        LedSingle.SetFrequencyHz(0xFFFFFFFF);
        LedSingle.Set(4);
    }
    else {
        Led.Init();
        Led.StartOrRestart(lsqStart);
        Printf("\r%S %S; id=%u; ch=%u; delay=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME),
                id, kRadioChnl, delay);
        Clk.PrintFreqs();

        // Try to receive Cmd by UART
        for(int i=0; i<27; i++) {
            chThdSleepMilliseconds(99);
            uint8_t b;
            while(dbg_uart.GetByte(&b) == retv::Ok) {
                if(dbg_uart.Cmd.PutChar(b) == pdrNewCmd) {
                    OnCmd(&dbg_uart.Cmd);
                    i = 0;
                }
            } // while get byte
        } // for
    } // if WasInStandby

    if(CC.Init() == retv::Ok) {
        // Select power
        uint8_t b = GetDipSwitch();
        pwr_lvl_id = b & 0b1111; // Remove high bits
        if(pwr_lvl_id > 11) pwr_lvl_id = 11;
        Printf("id %u; %S\r", id, kPwrNames[pwr_lvl_id]);
        // Setup CC
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(kRadioChnl);
        CC.SetBitrate(CCBitrate100k);
        CC.SetTxPower(PwrTable[pwr_lvl_id]);
        // Transmit
        pkt_tx.id = id;
        pkt_tx.the_word = 0xCA110FEA;
        CC.Recalibrate();
        CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), RPKT_LEN);
    }
    else { // CC failure
        Led.Init();
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(999);
    }

    // Enter sleep
    CC.EnterPwrDown();
    chSysLock();
    Iwdg::InitAndStart(delay);
    Sleep::EnterStandby();
    chSysUnlock();

    while(true); // Will never be here
}

void ReadEE() {
    id = EE::Read32(kEeAddrId);  // Read device id
    if(id < kIdMin or id > kIdMax) {
        Printf("\rUsing default id\r");
        id = kIdMin;
    }

    delay = EE::Read32(kEeAddrDelay);
    if(delay < 4 or delay > 306000) {
        Printf("\rUsing default delay\r");
        delay = 162;
    }
}

retv SetID(int32_t NewID) {
    retv rslt = EE::Write32(kEeAddrId, NewID);
    if(rslt == retv::Ok) {
        id = NewID;
        return retv::Ok;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retv::Fail;
    }
}

retv SetDelay(int32_t NewDelay) {
    retv rslt = EE::Write32(kEeAddrDelay, NewDelay);
    if(rslt == retv::Ok) {
        delay = NewDelay;
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

    else if(pcmd->NameIs("GetID")) Printf("id %u\r", id);

    else if(pcmd->NameIs("SetID")) {
        int32_t NewID;
        if(pcmd->GetNext<int32_t>(&NewID) != retv::Ok) { dbg_uart.CmdError(); return; }
        if(SetID(NewID) == retv::Ok) dbg_uart.Ok();
        else dbg_uart.Failure();
    }

    else if(pcmd->NameIs("GetDelay")) Printf("delay %u\r", delay);

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

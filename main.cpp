#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "MsgQ.h"
#include "main.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
void OnCmd(Cmd_t *PCmd);

static void ReadAndSetupMode();
#define EE_ADDR_ID       0
#define EE_ADDR_DELAY    4

cc1101_t CC(CC_Setup0);

int32_t ID;
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
void ReadEE();

static const uint8_t PwrTable[12] = {
        CC_PwrMinus30dBm, // 0
        CC_PwrMinus27dBm, // 1
        CC_PwrMinus25dBm, // 2
        CC_PwrMinus20dBm, // 3
        CC_PwrMinus15dBm, // 4
        CC_PwrMinus10dBm, // 5
        CC_PwrMinus6dBm,  // 6
        CC_Pwr0dBm,       // 7
        CC_PwrPlus5dBm,   // 8
        CC_PwrPlus7dBm,   // 9
        CC_PwrPlus10dBm,  // 10
        CC_PwrPlus12dBm   // 11
};

uint8_t PwrLvlId = 0;
rPkt_t PktTx;
uint32_t Delay;

// ==== Periphery ====
Vibro_t Vibro {VIBRO_SETUP};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();
    // === Init OS ===
    halInit();
    chSysInit();

    // ==== Init hardware ====
    Uart.Init();
    ReadEE();
    if(!Sleep::WasInStandby()) {
        Led.Init();
        Led.StartOrRestart(lsqStart);
        Printf("\r%S %S; ID=%u; Delay=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), ID, Delay);
        Clk.PrintFreqs();
        Beeper.Init();
        Beeper.StartOrRestart(bsqBeepBeep);

        // Try to receive Cmd by UART
        for(int i=0; i<27; i++) {
            chThdSleepMilliseconds(99);
            uint8_t b;
            while(Uart.GetByte(&b) == retvOk) {
                if(Uart.Cmd.PutChar(b) == pdrNewCmd) {
                    OnCmd(&Uart.Cmd);
                    i = 0;
                }
            } // while get byte
        } // for
    } // if WasInStandby

    if(CC.Init() == retvOk) {
        if(!Sleep::WasInStandby()) { // Show CC is ok
            Led.StartOrRestart(lsqStart);
            chThdSleepMilliseconds(999);
        }
        // CC params
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(0);
        ReadAndSetupMode();
        CC.SetTxPower(PwrTable[PwrLvlId]);
        PktTx.ID = ID;
        PktTx.TheWord = 0x5A110BE1;
        CC.Recalibrate();
        CC.Transmit(&PktTx, RPKT_LEN);
    }
    else { // CC failure
        Led.Init();
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(999);
    }

    // Enter sleep
    CC.EnterPwrDown();
    chSysLock();
    Iwdg::InitAndStart(Delay);
    Sleep::EnterStandby();
    chSysUnlock();

    while(true); // Will never be here
}

__unused
void ReadAndSetupMode() {
    uint8_t b = GetDipSwitch();
    // ==== Something has changed ====
    Printf("Dip: 0x%02X\r", b);
    // Select power
    PwrLvlId = b & 0b1111; // Remove high bits
}

void ReadEE() {
    ID = EE::Read32(EE_ADDR_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        ID = ID_DEFAULT;
    }

    Delay = EE::Read32(EE_ADDR_DELAY);
    if(Delay < 4 or Delay > 306000) {
        Printf("\rUsing default Delay\r");
        Delay = 162;
    }
}

uint8_t SetID(int32_t NewID) {
    uint8_t rslt = EE::Write32(EE_ADDR_ID, NewID);
    if(rslt == retvOk) {
        ID = NewID;
        return retvOk;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retvFail;
    }
}

uint8_t SetDelay(int32_t NewDelay) {
    uint8_t rslt = EE::Write32(EE_ADDR_DELAY, NewDelay);
    if(rslt == retvOk) {
        Delay = NewDelay;
        return retvOk;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retvFail;
    }
}

#if 1 // ================= Command processing ====================
void Ack(int32_t Result) { Printf("Ack %d\r\n", Result); }

void OnCmd(Cmd_t *PCmd) {
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        Ack(retvOk);
    }
    else if(PCmd->NameIs("Version")) Printf("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("GetID")) Printf("ID %u\r", ID);

    else if(PCmd->NameIs("SetID")) {
        int32_t NewID;
        if(PCmd->GetNext<int32_t>(&NewID) != retvOk) { Ack(retvCmdError); return; }
        Ack(SetID(NewID));
    }

    else if(PCmd->NameIs("GetDelay")) Printf("Delay %u\r", Delay);

    else if(PCmd->NameIs("SetDelay")) {
        int32_t NewDelay;
        if(PCmd->GetNext<int32_t>(&NewDelay) != retvOk) { Ack(retvCmdError); return; }
        Ack(SetDelay(NewDelay));
    }

    else Ack(retvCmdUnknown);
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(DipSwPin[i].PGpio, DipSwPin[i].Pin, DipSwPin[i].PullUpDown);
    for(int i=0; i<DIP_SW_CNT; i++) {
        if(!PinIsHi(DipSwPin[i].PGpio, DipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(DipSwPin[i].PGpio, DipSwPin[i].Pin);
    }
    return Rslt;
}

#ifndef BALLY_STERN_OS_H

struct PlayfieldLight {
  byte lightNum;
  byte row;
  byte col;
};

struct PlayfieldAndCabinetSwitch {
  byte switchNum;
  byte solenoid;
  byte solenoidHoldTime;
};


// Arduino wiring
// J5       DEFINITION   ARDUINO
// n/a      IRQ           2
// PIN 32 - NOT USED
// PIN 31 - GND           GND  
// PIN 30 - +5V           VIN
// PIN 29 - Not Used
// PIN 28 - Ext Mem
// PIN 27 - Theta2        4
// PIN 26 - VMA *         A5
// PIN 25 - /RESET
// PIN 24 - /HLT *        A6
// PIN 23 - R/W *         3
// PIN 22 - A0 *          A0
// PIN 21 - A1 *          A1
// PIN 20 - A2
// PIN 19 - A3 *          A2
// PIN 18 - A4 *          A3
// PIN 17 - A5
// PIN 16 - A6
// PIN 15 - A7 *          A4
// PIN 14 - A8
// PIN 13 - A9 *          GND
// PIN 12 - A10           GND
// PIN 11 - A11           GND
// PIN 10 - A12 *         GND
// PIN 9 - A13            
// PIN 8 - D0             5
// PIN 7 - D1             6
// PIN 6 - D2             7 
// PIN 5 - D3             8
// PIN 4 - D4             9
// PIN 3 - D5             10
// PIN 2 - D6             11
// PIN 1 - D7             12

// PIA @ U11 = A4, A7, !A9, !A12      
// PIA @ U10 = A3, A7, !A9, !A12




#define SW_SELF_TEST_SWITCH 0x7F
#define SOL_NONE 0x0F
#define SWITCH_STACK_EMPTY  0xFF
#define CONTSOL_DISABLE_FLIPPERS      0x40
#define CONTSOL_DISABLE_COIN_LOCKOUT  0x20


// Function Prototypes

//   Initialization
void BSOS_InitializeMPU();
void BSOS_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, PlayfieldAndCabinetSwitch *s_gameSwitchArray);
void BSOS_SetupGameLights(int s_numLights, PlayfieldLight *s_gameLightArray);
byte BSOS_GetDipSwitches(byte index);

//   Swtiches
byte BSOS_PullFirstFromSwitchStack();
boolean BSOS_ReadSingleSwitchState(byte switchNum);

//   Solenoids
void BSOS_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride = false);
void BSOS_SetCoinLockout(boolean lockoutOn = false, byte solbit = CONTSOL_DISABLE_COIN_LOCKOUT);
void BSOS_SetDisableFlippers(boolean disableFlippers = true, byte solbit = CONTSOL_DISABLE_FLIPPERS);
//void BSOS_SetContinuousSolenoids(byte continuousSolenoidMask = CONTSOL_DISABLE_FLIPPERS | CONTSOL_DISABLE_COIN_LOCKOUT);
byte BSOS_ReadContinuousSolenoids();
void BSOS_DisableSolenoidStack();
void BSOS_EnableSolenoidStack();
boolean BSOS_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride = false);
void BSOS_UpdateTimedSolenoidStack(unsigned long curTime);

//   Displays
void BSOS_SetDisplay(int displayNumber, unsigned long value);
void BSOS_SetDisplayBlank(int displayNumber, byte bitMask);
void BSOS_SetDisplayBlankByMagnitude(int displayNumber, unsigned long value);
void BSOS_SetDisplayBlankForCreditMatch(boolean creditsOn, boolean matchOn);
void BSOS_SetDisplayCredits(int value, boolean displayOn = true);
void BSOS_SetDisplayMatch(int value, boolean displayOn = true);
void BSOS_SetDisplayBallInPlay(int value, boolean displayOn = true);
void BSOS_SetDisplayBIPBlank(byte digitsOn=1);
void BSOS_SetDisplayFlash(int displayNumber, unsigned long curTime, int period=500, unsigned long magnitude=999999);
void BSOS_SetDisplayFlashCredits(unsigned long curTime, int period=100);
void BSOS_CycleAllDisplays(unsigned long curTime); // Self-test function

//   Lamps
void BSOS_SetLampState(int lampNum, byte s_lampState, byte s_lampDim=0, int s_lampFlashPeriod=0);
void BSOS_ApplyFlashToLamps(unsigned long curTime);
void BSOS_FlashAllLamps(unsigned long curTime); // Self-test function
void BSOS_TurnOffAllLamps();

//   Sound
void BSOS_PlaySound(byte soundByte);


byte BSOS_DataRead(int address);


#ifdef BALLY_STERN_CPP_FILE
  int NumGameSwitches = 0;
  int NumGamePrioritySwitches = 0;
  int NumGameLights = 0;
  
  PlayfieldLight *GameLights = NULL;
  PlayfieldAndCabinetSwitch *GameSwitches = NULL;
#endif


#define BALLY_STERN_OS_H
#endif

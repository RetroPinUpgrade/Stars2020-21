#include "BallySternOS.h"
#include "PinballMachineBaseDefinitions.h"
#include <EEPROM.h>

#define DEBUG_MESSAGES  1

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
int MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_TEST_LIGHTS     -1
#define MACHINE_STATE_TEST_DISPLAYS   -2
#define MACHINE_STATE_TEST_SOLENOIDS  -3
#define MACHINE_STATE_TEST_HISCR      -4
#define MACHINE_STATE_TEST_DONE       -5
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_UNVALIDATED     3
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100



// Game/machine global variables
unsigned long HighScore = 0;
int Credits = 0;
int MaximumCredits = 20;
boolean FreePlayMode = false;

byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentScores[4];

unsigned long CurrentTime = 0;


// EEPROM functions should be called as infrequently as possible
// because the EEPROM can only be read or written a limited number of 
// times 
#define CREDITS_EEPROM_BYTE         5
#define HIGHSCORE_EEPROM_START_BYTE 1

void WriteCreditsToEEProm(byte credits) {
  EEPROM.write(CREDITS_EEPROM_BYTE, credits);
}

byte ReadCreditsFromEEProm() {
  byte value = EEPROM.read(CREDITS_EEPROM_BYTE);

  // If this value is unset, set it
  if (value==0xFF) {
    value = 0;
    WriteCreditsToEEProm(value);
  }
  return value;
}

void WriteHighScoreToEEProm(unsigned long score) {
  EEPROM.write(HIGHSCORE_EEPROM_START_BYTE+3, (byte)(score>>24));
  EEPROM.write(HIGHSCORE_EEPROM_START_BYTE+2, (byte)((score>>16) & 0x000000FF));
  EEPROM.write(HIGHSCORE_EEPROM_START_BYTE+1, (byte)((score>>8) & 0x000000FF));
  EEPROM.write(HIGHSCORE_EEPROM_START_BYTE, (byte)(score & 0x000000FF));
}

unsigned long ReadHighScoreFromEEProm() {
  unsigned long value;

  value = (((unsigned long)EEPROM.read(HIGHSCORE_EEPROM_START_BYTE+3))<<24) | 
          ((unsigned long)(EEPROM.read(HIGHSCORE_EEPROM_START_BYTE+2))<<16) | 
          ((unsigned long)(EEPROM.read(HIGHSCORE_EEPROM_START_BYTE+1))<<8) | 
          ((unsigned long)(EEPROM.read(HIGHSCORE_EEPROM_START_BYTE)));

  if (value==0xFFFFFFFF) {
    value = 100000; // default score
    WriteHighScoreToEEProm(value);
  }
  return value;
}





void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }
    
  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);
 
  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  byte dipBank = BSOS_GetDipSwitches(0);

  // Use dip switches to set up game variables

}



unsigned long LastSelfTestChange = 0;
int LastTestSolNum = -1;
unsigned long LastSolTestTime = 0;

int RunSelfTest(int curState, boolean curStateChanged) {
  byte curSwitch = BSOS_PullFirstFromSwitchStack();
  int returnState = curState;

  // Things to add:
  //  Free Play Mode
  //  Extra ball/credit award scores (3)
  
  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>500) {
    returnState -= 1;
    if (returnState==MACHINE_STATE_TEST_DONE) returnState = MACHINE_STATE_ATTRACT;
    LastSelfTestChange = CurrentTime;
  }

  if (curState==MACHINE_STATE_TEST_LIGHTS) {
    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);
        BSOS_DisableSolenoidStack();        
        BSOS_SetDisableFlippers(true);
      }
      BSOS_SetDisableFlippers(true);
      BSOS_SetDisplayCredits(1);
      BSOS_TurnOffAllLamps();
      for (int count=0; count<60; count++) {
        BSOS_SetLampState(count, 1, 0, 500);
      }
    }
    BSOS_SetDisplayFlash(4, CurrentTime, 500);
    BSOS_ApplyFlashToLamps(CurrentTime);
  } else if (curState==MACHINE_STATE_TEST_DISPLAYS) {
    if (curStateChanged) {
      BSOS_TurnOffAllLamps();
      for (int count=0; count<5; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x3F);   
        BSOS_SetDisableFlippers(true);       
      }
      BSOS_DisableSolenoidStack();        
    }
    BSOS_CycleAllDisplays(CurrentTime);
  } else if (curState==MACHINE_STATE_TEST_SOLENOIDS) {
    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);        
      }
      BSOS_SetDisplayCredits(3);
      BSOS_TurnOffAllLamps();
      LastSolTestTime = CurrentTime;
      BSOS_EnableSolenoidStack(); 
      BSOS_SetDisableFlippers(false);       
    }

    int testSolNum = ((CurrentTime-LastSolTestTime)/1000)%15;
    if (testSolNum!=LastTestSolNum) {
      LastTestSolNum = testSolNum;
      BSOS_PushToSolenoidStack(testSolNum, 3);
      BSOS_SetDisplay(0, testSolNum);
      BSOS_SetDisplayBlankByMagnitude(0, testSolNum);
    }
    
    BSOS_SetDisplayFlash(4, CurrentTime, 500);    
  } else if (curState==MACHINE_STATE_TEST_HISCR) {
    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);        
      }
      BSOS_SetDisplayCredits(4);
      BSOS_TurnOffAllLamps();
      BSOS_DisableSolenoidStack();
      BSOS_SetDisplay(0, HighScore);
      BSOS_SetDisplayBlankByMagnitude(0, HighScore);
      BSOS_SetDisableFlippers(true);
    }
    if (curSwitch==SW_CREDIT_RESET) {
      HighScore = 50000;
      BSOS_SetDisplay(0, HighScore);
      BSOS_SetDisplayBlankByMagnitude(0, HighScore);
      WriteHighScoreToEEProm(HighScore);
    }
  }

  return returnState;
}



void AddCredit() {
  if (Credits<MaximumCredits) {
    Credits++;
    WriteCreditsToEEProm(Credits);
//    PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
  } else {
  }

  if (Credits<MaximumCredits) {
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetCoinLockout(true);
  }

}


boolean AddPlayer() {

  if (Credits<1 && !FreePlayMode) return false;
  if (CurrentNumPlayers>=4) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers-1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers-1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    WriteCreditsToEEProm(Credits);
  }
//  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1+(CurrentNumPlayers-1));
//  SetNumPlayersLamp(CurrentNumPlayers);

  return true;
}


byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  // If this is the first time in the attract mode loop
  if (curStateChanged) {
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
  }

  // Alternate displays between high score and blank
  if ((CurrentTime/6000)%2==0) {

    if (AttractLastHeadMode!=1) {
//      BSOS_SetLampState(HIGHEST_SCORE, 1, 0, 250);
//      BSOS_SetLampState(GAME_OVER, 0);
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
  
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, HighScore);
        BSOS_SetDisplayBlankByMagnitude(count, HighScore);
      }
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
    }
    AttractLastHeadMode = 1;
    
  } else {
    if (AttractLastHeadMode!=2) {
//      BSOS_SetLampState(HIGHEST_SCORE, 0);
//      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
      for (int count=0; count<4; count++) {
        if (CurrentNumPlayers>0) {
          if (count<CurrentNumPlayers) {
            BSOS_SetDisplayBlankByMagnitude(count, CurrentScores[count]);
            BSOS_SetDisplay(count, CurrentScores[count]); 
          } else {
            BSOS_SetDisplayBlank(count, 0x00);
            BSOS_SetDisplay(count, 0);          
          }          
        } else {
          BSOS_SetDisplayBlank(count, 0x30);
          BSOS_SetDisplay(count, 0);          
        }
      }
    }
    if ((CurrentTime/250)%4==0) {
      BSOS_SetLampState(PLAYER_1, 1);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
    } else if ((CurrentTime/250)%4==1) {
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 1);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
    } else if ((CurrentTime/250)%4==2) {
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 1);
      BSOS_SetLampState(PLAYER_4, 0);
    } else {
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 1);
    }
    AttractLastHeadMode = 2;
  }

  if ((CurrentTime/10000)%3==0) {  
    if (AttractLastPlayfieldMode!=1) {
      BSOS_TurnOffAllLamps();
    }
    
    AttractLastPlayfieldMode = 1;
  } else {
    if (AttractLastPlayfieldMode!=2) {
      BSOS_TurnOffAllLamps();
    }

    BSOS_ApplyFlashToLamps(CurrentTime);
    AttractLastPlayfieldMode = 2;
  }

  byte switchHit;
  while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
    if (switchHit==SW_CREDIT_RESET) {
      if (AddPlayer()) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      AddCredit();
      BSOS_SetDisplayCredits(Credits, true);
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>500) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      LastSelfTestChange = CurrentTime;
    }
  }

  return returnState;
}

int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  byte switchHit;
  while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {

    switch (switchHit) {
      case SW_SELF_TEST_SWITCH:
        returnState = MACHINE_STATE_TEST_LIGHTS;
        LastSelfTestChange = CurrentTime;
        break; 
      case SW_COIN_1:
      case SW_COIN_2:
      case SW_COIN_3:
        AddCredit();
        BSOS_SetDisplayCredits(Credits, true);
        break;
      case SW_CREDIT_RESET:
        if (CurrentBallInPlay<2) {
          // If we haven't finished the first ball, we can add players
          AddPlayer();
        } else {
          // If the first ball is over, pressing start again resets the game
          returnState = MACHINE_STATE_INIT_GAMEPLAY;
        }
        if (DEBUG_MESSAGES) {
          Serial.write("Start game button pressed\n\r");
        }
        break;        
    }
  }
}


void loop() {
  // This line has to be in the main loop
  BSOS_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  // Machine state is self-test/attract/game play
  if (MachineState<0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState!=MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  BSOS_UpdateTimedSolenoidStack(CurrentTime);

}

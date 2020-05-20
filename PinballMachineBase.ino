#include "BallySternOS.h"
#include "PinballMachineBaseDefinitions.h"
#include "SelfTestAndAudit.h"

#define DEBUG_MESSAGES  1

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
int MachineState = 0;
boolean MachineStateChanged = true;
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






void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(115200);
  }
    
  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

/*
  int testVal;
  if (testVal = BSOS_TestPIAChip(10, 0)) {
    if (DEBUG_MESSAGES) {
      char buf[32];
      sprintf(buf, "Failed test on U10:A (%d)\n", testVal);
      Serial.write(buf);    
    }
  }
  if (testVal = BSOS_TestPIAChip(10, 1)) {
    if (DEBUG_MESSAGES) {
      char buf[32];
      sprintf(buf, "Failed test on U10:B (%d)\n", testVal);
      Serial.write(buf);    
    }
  }
  if (testVal = BSOS_TestPIAChip(11, 0)) {
    if (DEBUG_MESSAGES) {
      char buf[32];
      sprintf(buf, "Failed test on U11:A (%d)\n", testVal);
      Serial.write(buf);    
    }
  }
  if (testVal = BSOS_TestPIAChip(11, 1)) {
    if (DEBUG_MESSAGES) {
      char buf[32];
      sprintf(buf, "Failed test on U11:B (%d)\n", testVal);
      Serial.write(buf);    
    }
  }
*/

  if (DEBUG_MESSAGES) {
    Serial.write("Attempting to initialize the MPU\n");
  }
 
  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  byte dipBank = BSOS_GetDipSwitches(0);

  // Use dip switches to set up game variables

  if (DEBUG_MESSAGES) {
    Serial.write("Done with setup\n");
  }

}





void AddCredit() {
  if (Credits<MaximumCredits) {
    Credits++;
    BSOS_WriteCreditsToEEProm(Credits);
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
    BSOS_WriteCreditsToEEProm(Credits);
  }
//  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1+(CurrentNumPlayers-1));
//  SetNumPlayersLamp(CurrentNumPlayers);

  return true;
}



int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState>=MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET);  
  } else {
    returnState = MACHINE_STATE_ATTRACT;
  }

  return returnState;
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
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>500) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
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
        SetLastSelfTestChangedTime(CurrentTime);
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

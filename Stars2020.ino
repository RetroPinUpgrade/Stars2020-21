#include "BallySternOS.h"
#include "SternStarsDefinitions.h"
#include <EEPROM.h>

#define DEBUG_MESSAGES  0

/*********************************************************************
*
*   Game specific code
*
*********************************************************************/
#define CREDITS_EEPROM_BYTE         5
#define HIGHSCORE_EEPROM_START_BYTE 1

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


#define SOUND_EFFECT_STAR_REWARD    1
#define SOUND_EFFECT_BONUS_COUNT    2
#define SOUND_EFFECT_INLANE_UNLIT   3
#define SOUND_EFFECT_OUTLANE_UNLIT  4
#define SOUND_EFFECT_INLANE_LIT     5
#define SOUND_EFFECT_OUTLANE_LIT    6
#define SOUND_EFFECT_BUMPER_HIT     7
#define SOUND_EFFECT_7K_BONUS       8
#define SOUND_EFFECT_DROP_TARGET    9
#define SOUND_EFFECT_ADD_CREDIT     10
#define SOUND_EFFECT_ADD_PLAYER_1   11
#define SOUND_EFFECT_ADD_PLAYER_2   SOUND_EFFECT_ADD_PLAYER_1+1
#define SOUND_EFFECT_ADD_PLAYER_3   SOUND_EFFECT_ADD_PLAYER_1+2
#define SOUND_EFFECT_ADD_PLAYER_4   SOUND_EFFECT_ADD_PLAYER_1+3
#define SOUND_EFFECT_PLAYER_1_UP    15
#define SOUND_EFFECT_PLAYER_2_UP    SOUND_EFFECT_PLAYER_1_UP+1
#define SOUND_EFFECT_PLAYER_3_UP    SOUND_EFFECT_PLAYER_1_UP+2
#define SOUND_EFFECT_PLAYER_4_UP    SOUND_EFFECT_PLAYER_1_UP+3
#define SOUND_EFFECT_BALL_OVER      19
#define SOUND_EFFECT_GAME_OVER      20
#define SOUND_EFFECT_2X_BONUS_COUNT    21
#define SOUND_EFFECT_3X_BONUS_COUNT    22
#define SOUND_EFFECT_EXTRA_BALL     23
#define SOUND_EFFECT_MACHINE_START  24
#define SOUND_EFFECT_SKILL_SHOT     25
#define SOUND_EFFECT_HIT_STAR_LEVEL_UP  26
#define SOUND_EFFECT_MISSED_STAR_LEVEL_UP   27

#define BUMPER_HITS_UNTIL_INLANES_LIGHT 50
#define BUMPER_HITS_UNTIL_ROLLOVER_LIT 25

#define SKILL_SHOT_DURATION 15
#define DROP_TARGET_GOAL_TIME 20

unsigned long HighScore = 0;
int Credits = 0;
boolean FreePlayMode = false;

byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentScores[4];
byte Bonus[4];
byte BonusX[4];
byte StarHit[5][4];
boolean StarLevelValidated[5][4];
boolean StarGoalComplete[4];
int BumperHits[4];
boolean InlanesLit[4];
boolean OutlanesLit[4];
boolean SamePlayerShootsAgain = false;
boolean Left7kLight = false;
boolean Right7kLight = false;
boolean RolloverLit = false;
byte Spinner400Bonus = 0;

byte BallSaveNumSeconds = 0;
boolean BallSaveUsed = false;
unsigned long BallFirstSwitchHitTime = 0;

unsigned long CurrentTime = 0;
unsigned long LastBumperHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long LeftSpinnerHurryUp = 0;
unsigned long RightSpinnerHurryUp = 0;
unsigned long DropTargetSpinnerGoal = 0;

#define MAX_BONUS           55
#define MAX_DISPLAY_BONUS   55

int MaximumCredits = 5;
int BallsPerGame = 3;
boolean CreditDisplay = false;
boolean ScoreGoalAwardsReplay = false;
boolean HighScoreReplay = false;
boolean MatchFeature = false;
boolean GameMelodyMinimal = false;
int SpecialLightAward = 0;
boolean WowExtraBall = false;
boolean BonusCountdown1000Steps = false;
boolean BothTargetSetsFor3X = true;
boolean MaximumNumber4Players = false;
boolean StarSpecialOncePerBall = false;

byte RovingStarLight = 0;
boolean SkillShotRunning = false;
boolean StarLevelUpMode = false;

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
  BSOS_SetupGameLights(NUM_STARS_LIGHTS, StarsLights);
  BSOS_SetupGameSwitches(NUM_STARS_SWITCHES_WITH_TRIGGERS, NUM_STARS_PRIORITY_SWITCHES_WITH_TRIGGERS, StarsSwitches);
 
  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  byte dipBank = BSOS_GetDipSwitches(0);
  HighScoreReplay = (dipBank&0x20)?true:false;
  BallsPerGame = (dipBank&40)?5:3;
  GameMelodyMinimal = (dipBank&0x80)?false:true;
  
  dipBank = BSOS_GetDipSwitches(1);
  HighScoreReplay = (dipBank&40)?true:false;
  
  dipBank = BSOS_GetDipSwitches(2);
  MaximumCredits = (dipBank&0x07)*5 + 5;
  CreditDisplay = (dipBank&0x08)?true:false;
  MatchFeature = (dipBank&0x10)?true:false;
  BonusCountdown1000Steps = (dipBank&0x20)?true:false;
  BothTargetSetsFor3X = (dipBank&80)?true:false;

  dipBank = BSOS_GetDipSwitches(3);
  MaximumNumber4Players = (dipBank&0x01)?true:false;
  WowExtraBall = (dipBank&0x02)?true:false;
  StarSpecialOncePerBall = (dipBank&0x20)?true:false;
  SpecialLightAward = (dipBank)>>6;
  
  HighScore = ReadHighScoreFromEEProm();
  Credits = ReadCreditsFromEEProm();  

  for (int count=0; count<4; count++) {
    BSOS_SetDisplay(count, HighScore);
    BSOS_SetDisplayBlankByMagnitude(count, HighScore);
  }

  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);

  // These settings should be setable in Self Test
  FreePlayMode = true;
  BallSaveNumSeconds = 16;
  BallSaveUsed = false;

  for (byte count=0; count<4; count++) {
    CurrentScores[count] = 0;
  }
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
    PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
  } else {
  }

  if (Credits<MaximumCredits) {
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetCoinLockout(true);
  }

}


byte AttractSweepLights = 1;
unsigned long AttractLastSweepTime = 0;
unsigned long AttractLastLadderTime = 0; 
byte AttractLastLadderBonus = 0;
unsigned long AttractLastStarTime = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
  }

  // Alternate displays between high score and blank
  if ((CurrentTime/6000)%2==0) {

    if (AttractLastHeadMode!=1) {
      BSOS_SetLampState(HIGHEST_SCORE, 1, 0, 250);
      BSOS_SetLampState(GAME_OVER, 0);
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
  
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, HighScore);
        BSOS_SetDisplayBlankByMagnitude(count, HighScore);
      }
      BSOS_SetDisplayCredits(Credits, false);
    }
    AttractLastHeadMode = 1;
    if (HighScore>999999) {
      ScrollScore(HighScore, 5);
    }

    
  } else {
    if (AttractLastHeadMode!=2) {
      BSOS_SetLampState(HIGHEST_SCORE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits);
      for (int count=0; count<4; count++) {
        if (CurrentNumPlayers>0) {
          if (count<CurrentNumPlayers) {
            if (CurrentScores[count]>999999) {
              ScrollScore(CurrentScores[count], count);  
            } else {
              BSOS_SetDisplayBlankByMagnitude(count, CurrentScores[count]);
              BSOS_SetDisplay(count, CurrentScores[count]); 
            }
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
    if ((CurrentTime-AttractLastSweepTime)>50) {
      AttractLastSweepTime = CurrentTime;
      for (int lightcount=0; lightcount<NUM_STARS_LIGHTS; lightcount++) {
        if (StarsLights[lightcount].row==(AttractSweepLights-1) && (AttractSweepLights-1)<27 && (AttractSweepLights-1)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-2) && (AttractSweepLights-2)<27 && (AttractSweepLights-2)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-3) && (AttractSweepLights-3)<27 && (AttractSweepLights-3)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-4) && (AttractSweepLights-4)<27 && (AttractSweepLights-4)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 0);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-5) && (AttractSweepLights-5)<27 && (AttractSweepLights-5)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-6) && (AttractSweepLights-6)<27 && (AttractSweepLights-6)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row==(AttractSweepLights-7) && (AttractSweepLights-7)<27 && (AttractSweepLights-7)>0) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 1, 1);
        } else if (StarsLights[lightcount].row<27) {
          BSOS_SetLampState(StarsLights[lightcount].lightNum, 0);
        }
      }
      
      AttractSweepLights += 1;
      if (AttractSweepLights>49) AttractSweepLights = 0;
      
    }
    AttractLastPlayfieldMode = 1;
  } else {
    if (AttractLastPlayfieldMode!=2) {
      BSOS_TurnOffAllLamps();
      AttractLastLadderBonus = 0;
    }
    if ( (CurrentTime-AttractLastLadderTime)>1000 ) {
      ShowBonusOnLadder(AttractLastLadderBonus);
      AttractLastLadderBonus += 1;
      AttractLastLadderTime = CurrentTime;
    }

    if ( (CurrentTime-AttractLastStarTime)>2000 ) {
      int starOn = (CurrentTime/2000)%5;

      for (int count=0; count<5; count++) {
        if (count==starOn) SetStarLampState(count, 1, 0, 500);
        else SetStarLampState(count, 1, 1, 0);
      }
      
      AttractLastStarTime = CurrentTime;
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
      BSOS_SetDisplayCredits(Credits);
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>500) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      LastSelfTestChange = CurrentTime;
    }
  }

  return returnState;
}


void SetStarLampState(int starNum, byte state, byte dim, int flash) {
  if (starNum<0 || starNum>4) return;
  switch (starNum) {
    case 0: BSOS_SetLampState(STAR_WHITE, state, dim, flash); break;
    case 1: BSOS_SetLampState(STAR_GREEN, state, dim, flash); break;
    case 2: BSOS_SetLampState(STAR_AMBER, state, dim, flash); break;
    case 3: BSOS_SetLampState(STAR_PURPLE, state, dim, flash); break;
    case 4: BSOS_SetLampState(STAR_YELLOW, state, dim, flash); break;
  }
}


void PlaySoundEffect(byte soundEffectNum) {
  
  switch (soundEffectNum) {
    case SOUND_EFFECT_STAR_REWARD:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+150);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+350);
      for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500+150*count);
      break;
    case SOUND_EFFECT_HIT_STAR_LEVEL_UP:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+150);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+350);
      for (int count=0; count<5; count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500+150*count);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+575+150*count);
      }
      break;
    case SOUND_EFFECT_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      break;
    case SOUND_EFFECT_2X_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+100);
      break;
    case SOUND_EFFECT_3X_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+200);
      break;
    case SOUND_EFFECT_INLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+250);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+350);
      for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+150*count);
     break;
    case SOUND_EFFECT_MISSED_STAR_LEVEL_UP:  
    case SOUND_EFFECT_OUTLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+250);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+350);
      break;
    case SOUND_EFFECT_INLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+200);
      for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+150*count);
      break;
    case SOUND_EFFECT_OUTLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
      break;
    case SOUND_EFFECT_SKILL_SHOT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+200);
      for (int count=0; count<15; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+75*count);
      break;
    case SOUND_EFFECT_BUMPER_HIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
      break;
    case SOUND_EFFECT_7K_BONUS:
      for (int count=0; count<7; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100+250*count);
      break;
    case SOUND_EFFECT_EXTRA_BALL:
      for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100+100*count);
      break;
    case SOUND_EFFECT_DROP_TARGET:
      for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+200*count);
      break;
    case SOUND_EFFECT_ADD_CREDIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+75, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+150, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+225, true);
      break;
    case SOUND_EFFECT_ADD_PLAYER_1:
    case SOUND_EFFECT_ADD_PLAYER_2:
    case SOUND_EFFECT_ADD_PLAYER_3:
    case SOUND_EFFECT_ADD_PLAYER_4:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+200, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+325, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+450, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+575, true);
      for (int count=0; count<(1+soundEffectNum-SOUND_EFFECT_ADD_PLAYER_1); count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+800+(count*300), true);
      }
      break;
    case SOUND_EFFECT_PLAYER_1_UP:
    case SOUND_EFFECT_PLAYER_2_UP:
    case SOUND_EFFECT_PLAYER_3_UP:
    case SOUND_EFFECT_PLAYER_4_UP:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+150, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+150, true);
      for (int count=0; count<(1+soundEffectNum-SOUND_EFFECT_PLAYER_1_UP); count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+count*150, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+300+count*150, true);
      }
      break;
    case SOUND_EFFECT_BALL_OVER:   
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+166, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+250, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+750, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+1000, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1166, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+1250, true);
      break;
    case SOUND_EFFECT_GAME_OVER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+166, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+250, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+500, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+666, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+750, true);
      break;
    case SOUND_EFFECT_MACHINE_START:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+150, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+300, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+450, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+600, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+750, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+900, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1250, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1400, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+1550, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1700, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+1850, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+2000, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+2150, true);
      break;
  }
}



int InitializeGamePlay() {
  int returnSTate = MACHINE_STATE_INIT_GAMEPLAY;

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  BSOS_EnableSolenoidStack();
  if (Credits>=MaximumCredits) {
    BSOS_SetCoinLockout(true);
  } else {
    BSOS_SetCoinLockout(false);
  }
  BSOS_TurnOffAllLamps();

  // Turn back on all lamps that are needed
  BSOS_SetLampState(PLAYER_1, 1);


  // Reset displays & game state variables
  for (int count=0; count<4; count++) {
    BSOS_SetDisplay(count, 0);
    if (count==0) BSOS_SetDisplayBlank(count, 0x30);
    else BSOS_SetDisplayBlank(count, 0x00);      

    CurrentScores[count] = 0;

    Bonus[count] = 0;
    BonusX[count] = 1;
    for (int starCount=0; starCount<5; starCount++) {
      StarHit[starCount][count] = 0;
    }
    for (int levelCount=0; levelCount<5; levelCount++) {
      StarLevelValidated[levelCount][count] = false;
    }
    StarGoalComplete[count] = false;  
    SamePlayerShootsAgain = false;
    
    InlanesLit[count] = false;
    OutlanesLit[count]=false;

    BumperHits[count] = 0;
  }

  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;

  Left7kLight = false;
  Right7kLight = false;
    
  return MACHINE_STATE_INIT_NEW_BALL;  
}


int InitializeNewBall(bool curStateChanged, int playerNum, int ballNum) {  

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    SamePlayerShootsAgain = false;
    BallFirstSwitchHitTime = 0;
    
    BSOS_SetDisableFlippers(false);
    BSOS_SetDisplayCredits(Credits);
    SetCurrentPlayerLamp(playerNum);
    
    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(BALL_IN_PLAY, 1);
    BSOS_SetDisplayBIPBlank(1);
    BSOS_SetLampState(SPECIAL_PURPLE_STAR, 0);
    BSOS_SetLampState(SPECIAL_WHITE_STAR, 0);
    BSOS_SetLampState(SPECIAL_GREEN_STAR, 0);
    BSOS_SetLampState(SPECIAL_AMBER_STAR, 0);
    BSOS_SetLampState(SPECIAL_YELLOW_STAR, 0);
  
    if (BallSaveNumSeconds>0) {
      BSOS_SetLampState(SHOOT_AGAIN, 1, 0, 500);
    }
    
    Left7kLight = false;
    Right7kLight = false;
    Bonus[playerNum] = 0;
    BonusX[playerNum] = 1;
    Spinner400Bonus = 0;
    SetDropTargetRelatedLights(BonusX[playerNum]);
    DropTargetSpinnerGoal = 0;
    FlipInOutLanesLights();
    BallSaveUsed = false;
    RovingStarLight = 1;
    SkillShotRunning = true;
    StarLevelUpMode = false;
    BallTimeInTrough = 0;

    for (int count=0; count<5; count++) {    
      SetStarLampState(count, (StarHit[count][playerNum])?1:0, (StarHit[count][playerNum]==1?true:false), (StarHit[count][playerNum]==3?500:0));
    }
  
  
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      if (DEBUG_MESSAGES) {
        Serial.write("Ball in trough - pushing to plunger\n\r");
      }
  
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
    }
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime + 20);
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime + 150);

  }

  
  // We should only consider the ball initialized when 
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (DEBUG_MESSAGES) {
      Serial.write("Switch still closed - staying in MACHINE_STATE_INIT_NEW_BALL\n\r");
    }
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    if (DEBUG_MESSAGES) {
      Serial.write("Switch open - moving to MACHINE_STATE_NORMAL_GAMEPLAY\n\r");
    }
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }
  
}



unsigned long LastRovingStarLightReportTime = 0;
int RovingStarPhase = 0;
int RovingStarPeriod = 1000;

void ShowStarGoalCompleteLights() {
  BSOS_SetLampState(SPECIAL_PURPLE_STAR, 1, 0, 500);
  BSOS_SetLampState(SPECIAL_WHITE_STAR, 1, 0, 500);
  BSOS_SetLampState(SPECIAL_GREEN_STAR, 1, 0, 500);
  BSOS_SetLampState(SPECIAL_AMBER_STAR, 1, 0, 500);
  BSOS_SetLampState(SPECIAL_YELLOW_STAR, 1, 0, 500);
}

void ShowRovingStarLight() {
  if (RovingStarLight==0) {
    BSOS_SetLampState(SPECIAL_PURPLE_STAR, 0);
    BSOS_SetLampState(SPECIAL_WHITE_STAR, 0);
    BSOS_SetLampState(SPECIAL_GREEN_STAR, 0);
    BSOS_SetLampState(SPECIAL_AMBER_STAR, 0);
    BSOS_SetLampState(SPECIAL_YELLOW_STAR, 0);
    return;
  }

  if ((CurrentTime-LastRovingStarLightReportTime)>(RovingStarPeriod/5)) {
    int rovingStarLampNum = 0;
    switch (RovingStarLight) {
      case 1: rovingStarLampNum = SPECIAL_PURPLE_STAR; break;
      case 2: rovingStarLampNum = SPECIAL_WHITE_STAR; break;
      case 3: rovingStarLampNum = SPECIAL_GREEN_STAR; break;
      case 4: rovingStarLampNum = SPECIAL_AMBER_STAR; break;
      case 5: rovingStarLampNum = SPECIAL_YELLOW_STAR; break;
    }
    
    BSOS_SetLampState(rovingStarLampNum, (RovingStarPhase==0 || RovingStarPhase==4)?0:1, (RovingStarPhase%2));

    RovingStarPhase += 1;
    if (RovingStarPhase>4) {
      RovingStarPhase = 0;
      RovingStarLight += 1;
      if (RovingStarLight>5) RovingStarLight = 1;
    }
    LastRovingStarLightReportTime = CurrentTime;
  }
}


boolean CheckForRovingStarHit(int starNum) {
  if (RovingStarLight==0) return false;
  if (RovingStarLight==starNum) return true;
  return false;
}


void SetBonusIndicator(byte number, byte state, byte dim, int flashPeriod=0) {
  if (number==0 || number>10) return;
  switch (number) {
    case 1: BSOS_SetLampState(D1K_BONUS, state, dim, flashPeriod); break;
    case 2: BSOS_SetLampState(D2K_BONUS, state, dim, flashPeriod); break;
    case 3: BSOS_SetLampState(D3K_BONUS, state, dim, flashPeriod); break;
    case 4: BSOS_SetLampState(D4K_BONUS, state, dim, flashPeriod); break;
    case 5: BSOS_SetLampState(D5K_BONUS, state, dim, flashPeriod); break;
    case 6: BSOS_SetLampState(D6K_BONUS, state, dim, flashPeriod); break;
    case 7: BSOS_SetLampState(D7K_BONUS, state, dim, flashPeriod); break;
    case 8: BSOS_SetLampState(D8K_BONUS, state, dim, flashPeriod); break;
    case 9: BSOS_SetLampState(D9K_BONUS, state, dim, flashPeriod); break;
    case 10: BSOS_SetLampState(D10K_BONUS, state, dim, flashPeriod); break;
  }

}

void ShowBonusOnLadder(byte bonus) {
  if (bonus>MAX_DISPLAY_BONUS) bonus = MAX_DISPLAY_BONUS;
  byte cap = 10;

  for (byte count=1; count<11; count++) SetBonusIndicator(count, 0, 0);

  while (bonus>cap) {
    SetBonusIndicator(cap, 1, 0, 100);
    bonus -= cap;
    cap -= 1;
    if (cap==0) bonus = 0;
  }

  byte bottom; 
  for (bottom=1; bottom<bonus; bottom++){
    SetBonusIndicator(bottom, 1, 1);
  }

  if (bottom<=cap) {
    SetBonusIndicator(bottom, 1, 0);
  }
  
}


void SetNumPlayersLamp(byte numPlayers) {
    switch(numPlayers) {
    case 1:
      BSOS_SetLampState(PLAYER_1, 1);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
      break;
    case 2:
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 1);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 0);
      break;
    case 3:
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 1);
      BSOS_SetLampState(PLAYER_4, 0);
      break;
    case 4:
      BSOS_SetLampState(PLAYER_1, 0);
      BSOS_SetLampState(PLAYER_2, 0);
      BSOS_SetLampState(PLAYER_3, 0);
      BSOS_SetLampState(PLAYER_4, 1);
      break;
  }  
}


void SetCurrentPlayerLamp(byte playerToShow) {

  switch(playerToShow) {
    case 0:
      BSOS_SetLampState(PLAYER_1_UP, 1);
      BSOS_SetLampState(PLAYER_2_UP, 0);
      BSOS_SetLampState(PLAYER_3_UP, 0);
      BSOS_SetLampState(PLAYER_4_UP, 0);
      break;
    case 1:
      BSOS_SetLampState(PLAYER_1_UP, 0);
      BSOS_SetLampState(PLAYER_2_UP, 1);
      BSOS_SetLampState(PLAYER_3_UP, 0);
      BSOS_SetLampState(PLAYER_4_UP, 0);
      break;
    case 2:
      BSOS_SetLampState(PLAYER_1_UP, 0);
      BSOS_SetLampState(PLAYER_2_UP, 0);
      BSOS_SetLampState(PLAYER_3_UP, 1);
      BSOS_SetLampState(PLAYER_4_UP, 0);
      break;
    case 3:
      BSOS_SetLampState(PLAYER_1_UP, 0);
      BSOS_SetLampState(PLAYER_2_UP, 0);
      BSOS_SetLampState(PLAYER_3_UP, 0);
      BSOS_SetLampState(PLAYER_4_UP, 1);
      break;
  }
}


boolean AddPlayer() {

  if (Credits<1 && !FreePlayMode) return false;
  if (CurrentNumPlayers>=4 || (CurrentNumPlayers>=2 && !MaximumNumber4Players)) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers-1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers-1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    WriteCreditsToEEProm(Credits);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1+(CurrentNumPlayers-1));
  SetNumPlayersLamp(CurrentNumPlayers);

  return true;
}



int LastReportedValue = 0;
boolean CurrentlyShowingBumperHits = false;
boolean CurrentlyShowingBallSave = false;
boolean PlayerUpLightBlinking = false;

// This function manages all timers, flags, and lights
int NormalGamePlay() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  // If the playfield hasn't been validated yet, flash score and player up num
  if (BallFirstSwitchHitTime==0) {
    if (CurrentScores[CurrentPlayer]>999999) {
      if ((CurrentTime/500)%2) ScrollScore(CurrentScores[CurrentPlayer], CurrentPlayer);
      else BSOS_SetDisplayBlank(CurrentPlayer, 0x00);
    } else {
      BSOS_SetDisplayFlash(CurrentPlayer, CurrentTime, 500, (CurrentScores[CurrentPlayer]==0)?99:CurrentScores[CurrentPlayer]);
    }
    if (!PlayerUpLightBlinking) {
      switch(CurrentPlayer) {
        case 0: BSOS_SetLampState(PLAYER_1_UP, 1, 0, 500); break;
        case 1: BSOS_SetLampState(PLAYER_2_UP, 1, 0, 500); break;
        case 2: BSOS_SetLampState(PLAYER_3_UP, 1, 0, 500); break;
        case 3: BSOS_SetLampState(PLAYER_4_UP, 1, 0, 500); break;
      }
      PlayerUpLightBlinking = true;
    }
  } else {
    if (PlayerUpLightBlinking) {
      switch(CurrentPlayer) {
        case 0: BSOS_SetLampState(PLAYER_1_UP, 1); break;
        case 1: BSOS_SetLampState(PLAYER_2_UP, 1); break;
        case 2: BSOS_SetLampState(PLAYER_3_UP, 1); break;
        case 3: BSOS_SetLampState(PLAYER_4_UP, 1); break;
      }
      PlayerUpLightBlinking = false;
    }
  }

  // For one second after the bumper is hit
  // show the number of hits left until 
  // the inlanes light
  if (!CurrentlyShowingBallSave && (CurrentTime-LastBumperHitTime)<2000) {
    int bumperHitsLeft = BUMPER_HITS_UNTIL_INLANES_LIGHT - BumperHits[CurrentPlayer];
    if (bumperHitsLeft>=0) {
      if (LastReportedValue!=bumperHitsLeft) {
        BSOS_SetDisplayCredits(bumperHitsLeft);
        CurrentlyShowingBumperHits = true;
        LastReportedValue = bumperHitsLeft;
      } else {
        BSOS_SetDisplayFlashCredits(CurrentTime);
      }
      CurrentlyShowingBumperHits = true;      
    }
    
  } else if (CurrentlyShowingBumperHits) {
    BSOS_SetDisplayCredits(Credits);
    CurrentlyShowingBumperHits = false;
    LastReportedValue = 0;
  }

  // if we're in ball save time
  if ( BallFirstSwitchHitTime>0 && BallSaveNumSeconds>0 && ((CurrentTime-BallFirstSwitchHitTime)/1000)<BallSaveNumSeconds && !BallSaveUsed ) {
    int tenthsLeft = ((int)BallSaveNumSeconds)*10 - (CurrentTime-BallFirstSwitchHitTime)/100;
    if (LastReportedValue!=tenthsLeft) {
      CurrentlyShowingBallSave = true;
      if (tenthsLeft>99) {
        BSOS_SetDisplayCredits(tenthsLeft/10);
      } else {
        BSOS_SetDisplayCredits(tenthsLeft);
      }
      LastReportedValue = tenthsLeft;
      int flashSpeed = ((tenthsLeft/30)+1);
      switch (flashSpeed) {
        case 10:
        case 9:
        case 8:
        case 7:
        case 6:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/5)%2);
          break;
        case 5:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/5)%2);
          break;
        case 4:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/4)%2);
          break;
        case 3:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/4)%2);
          break;
        case 2:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/3)%2);
          break;
        case 1:
          BSOS_SetLampState(SHOOT_AGAIN, tenthsLeft%2);
          break;
      }
    } else {
//      BSOS_SetDisplayFlashCredits(CurrentTime, 250);      
    }
  } else if (CurrentlyShowingBallSave) {
    if (SamePlayerShootsAgain) BSOS_SetLampState(SHOOT_AGAIN, 1);
    else BSOS_SetLampState(SHOOT_AGAIN, 0);
    BSOS_SetDisplayCredits(Credits);
    CurrentlyShowingBallSave = false;
    LastReportedValue = 0;
  }


  // Skill shot
  if (BallFirstSwitchHitTime==0 || (CurrentTime-BallFirstSwitchHitTime)<(SKILL_SHOT_DURATION*1000) ) {
    if (SkillShotRunning && RovingStarLight) ShowRovingStarLight();
  } else if (SkillShotRunning) {
    if (!StarLevelUpMode) {
      RovingStarLight = 0;
      ShowRovingStarLight();
    }
    SkillShotRunning = false;
    if (StarGoalComplete[CurrentPlayer]) ShowStarGoalCompleteLights();
  }

  if (StarLevelUpMode) {
    if (RovingStarLight) ShowRovingStarLight();
  }

  // Drop target completion goal
  if (DropTargetSpinnerGoal) {
    SetLeftSpinnerLights(BonusX[CurrentPlayer]);
    if (((CurrentTime-DropTargetSpinnerGoal)/1000)>DROP_TARGET_GOAL_TIME) {
      DropTargetSpinnerGoal = 0;
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime+130);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime);
      Spinner400Bonus = 0; 
    }
  }


  // Check to see if ball is in the outhole
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough==0) {
      BallTimeInTrough = CurrentTime;
      if (DEBUG_MESSAGES) {
        Serial.write("Starting outhole timer\n\r");
      }
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime-BallTimeInTrough)>500) {


        if (DEBUG_MESSAGES) {
          Serial.write("Outhole timer done\n\r");
        }

        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        
        // if we haven't used the ball save, and we're under the time limit, then save the ball
        if (  !BallSaveUsed && 
              ((CurrentTime-BallFirstSwitchHitTime)/1000)<BallSaveNumSeconds ) {
        
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
          if (BallFirstSwitchHitTime>0) {
            BallSaveUsed = true;
            BSOS_SetLampState(SHOOT_AGAIN, 0);
          }
          BallTimeInTrough = CurrentTime;

          if (DEBUG_MESSAGES) {
            Serial.write("Ball saved\n\r");
          }
          
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
        } else {
          if (DEBUG_MESSAGES) {
            Serial.write("Ball save has been used, ending ball\n\r");
          }
          returnState = MACHINE_STATE_COUNTDOWN_BONUS;
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }
  
  BSOS_ApplyFlashToLamps(CurrentTime);

  return returnState;
}


int GetNumStarsLit(int CurrentPlayer) {
  int numStars = 0;
  for (int i=0; i<5; i++) {
    numStars += StarHit[i][CurrentPlayer];
  }
  return numStars;
}

byte GetNextStarLevel(int starNum, int CurrentPlayer) {

  // Find the smallest level and return it.
  // This will make sure that you have to complete
  // all stars before the level can increase.
  byte minLevel = 10;
  for (int i=0; i<5; i++) {
    if (StarHit[i][CurrentPlayer]<minLevel) minLevel = StarHit[i][CurrentPlayer];
  }

  minLevel += 1;
  if (minLevel<=3) return minLevel;

  return 3;
}

void SetLeftSpinnerLights(int bonusX) {

  if (bonusX<6) {
    if (DropTargetSpinnerGoal && ((CurrentTime-DropTargetSpinnerGoal)/1000)<DROP_TARGET_GOAL_TIME) {
      int lightSweep = ((CurrentTime-DropTargetSpinnerGoal)/250)%3;
      BSOS_SetLampState(SPECIAL_FEATURE, (lightSweep==0)?1:0);
      BSOS_SetLampState(D400_1_SPINNER, (lightSweep==2)?1:0);
      BSOS_SetLampState(D400_2_SPINNER, (lightSweep==1)?1:0);    
    } else {
      BSOS_SetLampState(D400_1_SPINNER, (Spinner400Bonus>0)?1:0);
      BSOS_SetLampState(D400_2_SPINNER, (Spinner400Bonus>1)?1:0);    
    }
  } else {
    BSOS_SetLampState(D400_1_SPINNER, (Spinner400Bonus>0)?1:0);
    BSOS_SetLampState(D400_2_SPINNER, (Spinner400Bonus>1)?1:0);    
  }
}

void SetDropTargetRelatedLights(int bonusX) {

  // Lights at bottom of playfield
  BSOS_SetLampState(DOUBLE_BONUS, (bonusX==2)?1:0);  
  BSOS_SetLampState(TRIPLE_BONUS, (bonusX>2)?1:0);

  if (BallSaveNumSeconds==0 || (BallFirstSwitchHitTime>0 && ((CurrentTime-BallFirstSwitchHitTime)/1000)>BallSaveNumSeconds)) {
    // Only set this lamp here, if there's no ballsave or
    // the playfield is validated & the ballsave is expired
    BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
  }

  // Lights indicating next award
  BSOS_SetLampState(DOUBLE_BONUS_FEATURE, (bonusX==1)?1:0);
  BSOS_SetLampState(TRIPLE_BONUS_FEATURE, (bonusX==2)?1:0);
  BSOS_SetLampState(WOW, (bonusX==3)?1:0);  
  BSOS_SetLampState(SPECIAL_FEATURE, (bonusX>3)?1:0, (bonusX==4)?1:0, (bonusX>5)?500:0);  

  BSOS_SetLampState(D7000_LEFT, Left7kLight);
  BSOS_SetLampState(D7000_RIGHT, Right7kLight);
  SetLeftSpinnerLights(bonusX);
}


boolean WaitForDropTargetsToReset = false;

void HandleDropTargetHit(int switchHit, int curPlayer) {
  boolean requireBothSetsOfDrops = false;

  if (BonusX[curPlayer]>2 || (BonusX[curPlayer]==2 && BothTargetSetsFor3X)) requireBothSetsOfDrops = true;
    
  CurrentScores[CurrentPlayer] += 500;

  if (switchHit==SW_DROP_TARGET_1 || switchHit==SW_DROP_TARGET_3) {
    if (  BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && 
          BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) && 
          !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) ) {
      Left7kLight = true;
    }
    CurrentScores[curPlayer] += 500;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET);
  }
  if (switchHit==SW_DROP_TARGET_2) {
    CurrentScores[curPlayer] += 500;
    if (Left7kLight) {
      CurrentScores[curPlayer] += 6500;
      PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
      Left7kLight = false;
    } else {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET);
    }
    Spinner400Bonus += 1;
  }

  if (switchHit==SW_DROP_TARGET_4 || switchHit==SW_DROP_TARGET_6) {
    if (  BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && 
          BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6) && 
          !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) ) {
      Right7kLight = true;
    }
    CurrentScores[curPlayer] += 500;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET);
  }
  if (switchHit==SW_DROP_TARGET_5) {
    CurrentScores[curPlayer] += 500;
    if (Right7kLight) {
      CurrentScores[curPlayer] += 6500;
      PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
      Right7kLight = false;
    } else {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET);
    }
    Spinner400Bonus += 1;
  }

  if (!WaitForDropTargetsToReset) {
    if (requireBothSetsOfDrops) {
      if (  BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && 
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) && 
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) && 
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && 
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) && 
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6)  ) {

        if (BonusX[curPlayer]==5) {
          DropTargetSpinnerGoal = CurrentTime;
        } else {
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime+130);
        }
        WaitForDropTargetsToReset = true;
        Spinner400Bonus = 0;
        Right7kLight = false;
        Left7kLight = false;

        if (BonusX[curPlayer]<5) BonusX[curPlayer] += 1;
        
        if (BonusX[curPlayer]==4) {
          if (WowExtraBall) SamePlayerShootsAgain = true;
          else CurrentScores[curPlayer] += 25000;
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        }
      }
    } else {
      if (  ( BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) ) ||
            ( BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6) ) ) {

        if (BonusX[curPlayer]==5) {
          DropTargetSpinnerGoal = CurrentTime;
        } else {
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime+130);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime);
        }
        WaitForDropTargetsToReset = true;
        Spinner400Bonus = 0;
        Right7kLight = false;
        Left7kLight = false;

        if (BonusX[curPlayer]<5) BonusX[curPlayer] += 1;
  
        if (BonusX[curPlayer]==4) {
          if (WowExtraBall) SamePlayerShootsAgain = true;
          else CurrentScores[curPlayer] += 25000;
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        }      
      }
    }
  }
  
  SetDropTargetRelatedLights(BonusX[curPlayer]);

}


unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
    //BSOS_SetDisableFlippers(true);
    BSOS_SetLampState(BALL_IN_PLAY, 0);
//    BSOS_SetDisplayBIPBlank(0);

    CountdownStartTime = CurrentTime;
    ShowBonusOnLadder(Bonus[CurrentPlayer]);
    
    LastCountdownReportTime = CountdownStartTime;
//    PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  if ((CurrentTime-LastCountdownReportTime)>300) {
    
    if (Bonus[CurrentPlayer]>0) {

      if (BonusX[CurrentPlayer]==1) {
        PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT);
        CurrentScores[CurrentPlayer] += 1000;
      } else if (BonusX[CurrentPlayer]==2) {
        PlaySoundEffect(SOUND_EFFECT_2X_BONUS_COUNT);
        CurrentScores[CurrentPlayer] += 2000;
      } else if (BonusX[CurrentPlayer]>2) {
        PlaySoundEffect(SOUND_EFFECT_3X_BONUS_COUNT);
        CurrentScores[CurrentPlayer] += 3000;
      }
      
      Bonus[CurrentPlayer] -= 1;
      ShowBonusOnLadder(Bonus[CurrentPlayer]);
    } else if (BonusCountDownEndTime==0xFFFFFFFF) {
      PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
      BSOS_SetLampState(D1K_BONUS, 0);
      BonusCountDownEndTime = CurrentTime+1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime>BonusCountDownEndTime) {
    BonusCountDownEndTime = 0xFFFFFFFF;

    // unset any star level that's not validated
    for (int level=0; level<5; level++) {
      if (!StarLevelValidated[level][CurrentPlayer]) {
        for (int starCount=0; starCount<5; starCount++) {
          StarHit[starCount][CurrentPlayer] = level;
        }
        break;
      }
    }

    RovingStarLight = 0;
    ShowRovingStarLight();
    
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}


void FlipInOutLanesLights() {
  if (InlanesLit[CurrentPlayer]) {
    InlanesLit[CurrentPlayer] = false;
    BSOS_SetLampState(IN_LANES, 0);
    OutlanesLit[CurrentPlayer] = true;
    BSOS_SetLampState(OUT_LANES, 1);
  } else if (OutlanesLit[CurrentPlayer]) {
    InlanesLit[CurrentPlayer] = true;
    BSOS_SetLampState(IN_LANES, 1);
    OutlanesLit[CurrentPlayer] = false;
    BSOS_SetLampState(OUT_LANES, 0);
  } else {
    BSOS_SetLampState(IN_LANES, 0);
    BSOS_SetLampState(OUT_LANES, 0);
  }
}


void CheckHighScores() {
  unsigned long highestScore = 0;
  for (int count=0; count<4; count++) {
    if (CurrentScores[count]>highestScore) highestScore = CurrentScores[count];
  }

  if (highestScore>HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      Credits+=3;
      if (Credits>MaximumCredits) Credits = MaximumCredits;
      WriteCreditsToEEProm(Credits);
    }
    WriteHighScoreToEEProm(highestScore);
    
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime+300, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime+600, true);
  }
}


void HandleStarHit(byte switchNum) {
  int rovingStarCheck=1, starID=0;
  byte nextStarLevel = 0;

  switch (switchNum) {
    case SW_STAR_4:
      rovingStarCheck = 2;
      starID = 0;
      break;
    case SW_STAR_2:
      rovingStarCheck = 3;
      starID = 1;
      break;
    case SW_STAR_5:
      rovingStarCheck = 4;
      starID = 2;
      break;
    case SW_STAR_1:
      rovingStarCheck = 1;
      starID = 3;
      break;
    case SW_STAR_3:
      rovingStarCheck = 5;
      starID = 4;
      break;
  }

  if (!StarLevelUpMode) {
    if (SkillShotRunning && CheckForRovingStarHit(rovingStarCheck)) {
      SkillShotRunning = false;
      RovingStarLight = 0;
      ShowRovingStarLight();
      CurrentScores[CurrentPlayer] += 25000;
      PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
      Bonus[CurrentPlayer] += 1;
      if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
    } else {
      PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
      CurrentScores[CurrentPlayer] += 400 + 100*nextStarLevel;
      Bonus[CurrentPlayer] += 1;
      if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
    }
  
    nextStarLevel = GetNextStarLevel(starID, CurrentPlayer);
    StarHit[starID][CurrentPlayer] = nextStarLevel;
    SetStarLampState(starID, 1, (nextStarLevel==1?true:false), (nextStarLevel==3?500:0));

    // Check to see if we need to be in level-up mode
    if (  !StarGoalComplete[CurrentPlayer] &&
          StarHit[0][CurrentPlayer]==nextStarLevel &&
          StarHit[1][CurrentPlayer]==nextStarLevel &&
          StarHit[2][CurrentPlayer]==nextStarLevel &&
          StarHit[3][CurrentPlayer]==nextStarLevel &&
          StarHit[4][CurrentPlayer]==nextStarLevel ) {
      StarLevelUpMode = true;
      RovingStarLight = 1;
    }
  
  } else {
    // StarLevelUpMode only ends if the player hits the right target
    if (CheckForRovingStarHit(rovingStarCheck)) {
      PlaySoundEffect(SOUND_EFFECT_HIT_STAR_LEVEL_UP);
      StarLevelUpMode = false;  
      RovingStarLight = 0;
      ShowRovingStarLight();
      CurrentScores[CurrentPlayer] += StarHit[0][CurrentPlayer]*25000;
      StarLevelValidated[StarHit[0][CurrentPlayer]-1][CurrentPlayer] = true;
      if (StarHit[0][CurrentPlayer]==3) {
        StarGoalComplete[CurrentPlayer] = true;
        ShowStarGoalCompleteLights();
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_MISSED_STAR_LEVEL_UP);
      CurrentScores[CurrentPlayer] += 100;
    }
  }
  

}


unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeScoreScrolled[6] = {0, 0, 0, 0, 0, 0}; 
int ScrollIndex[6] = {0, 0, 0, 0, 0, 0};

void ResetScoreScroll(byte displayNum) {
  ScrollIndex[displayNum] = 0;
}

byte MagnitudeOfScore(unsigned long score) {
  if (score==0) return 0;

  byte retval = 0;
  while (score>0) {
    score = score/10;
    retval += 1;
  }

  return retval;
}

void ScrollScore(unsigned long score, byte displayNum) {

  if (ScrollIndex[displayNum]==0 && (CurrentTime-LastTimeScoreScrolled[displayNum])>1500) {
    // Pause at zero position for 2 seconds
    ScrollIndex[displayNum] += 1;
    LastTimeScoreScrolled[displayNum] = CurrentTime;
  } else if ( ScrollIndex[displayNum]>0 && (CurrentTime-LastTimeScoreScrolled[displayNum])>250 ) {
    // Scroll 1 digit every .5 seconds
    unsigned long reportedScore = score;
    byte scoreMask = 0x3F;
    int count;

    // figure out left part of score
    for (count=0; count<ScrollIndex[displayNum]; count++) {
      reportedScore = (reportedScore % 1000000)*10;
      scoreMask = scoreMask>>1;
    }

    // Add in right part of score
    if (ScrollIndex[displayNum]>2) {
      unsigned long bottomScore = score;
      for (count=0; count<(12-ScrollIndex[displayNum]); count++) bottomScore = bottomScore/10;
      if (bottomScore>0) {
        reportedScore += bottomScore;
        byte mag = MagnitudeOfScore(bottomScore);
        for (count=0; count<mag; count++) {
          scoreMask |= 0x20>>count;
        }
      }
    }

    if (displayNum<5) {
      BSOS_SetDisplay(displayNum, reportedScore);
      BSOS_SetDisplayBlank(displayNum, scoreMask);
    } else {
      for (count=0; count<4; count++) {
        BSOS_SetDisplay(count, reportedScore);
        BSOS_SetDisplayBlank(count, scoreMask);
      }
    }

    if (DEBUG_MESSAGES) {
      char msg[100];
      sprintf(msg, "Setting display %d to %lu with mask 0x%02X\n\r", displayNum, reportedScore, scoreMask);
      Serial.write(msg);
    }

    ScrollIndex[displayNum]+=1;
    if (ScrollIndex[displayNum]>12) ScrollIndex[displayNum] = 0;
    LastTimeScoreScrolled[displayNum] = CurrentTime;
  }
}



int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte bonusAtTop = Bonus[CurrentPlayer];
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState==MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitializeGamePlay();    
  } else if (curState==MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitializeNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState==MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = NormalGamePlay();
  } else if (curState==MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
  } else if (curState==MACHINE_STATE_BALL_OVER) {
    if (SamePlayerShootsAgain) {
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {
      CurrentPlayer+=1;
      if (CurrentPlayer>=CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay+=1;
      }
        
      if (CurrentBallInPlay>BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        returnState = MACHINE_STATE_ATTRACT;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  }


  if (  !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && 
    !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) && 
    !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) && 
    !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && 
    !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) && 
    !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6)  ) {
    WaitForDropTargetsToReset = false;    
  }



  byte switchHit;
  while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {

    switch (switchHit) {
      case SW_SELF_TEST_SWITCH:
        returnState = MACHINE_STATE_TEST_LIGHTS;
        LastSelfTestChange = CurrentTime;
        break; 
      case SW_LEFT_INLANE:
      case SW_RIGHT_INLANE:
        if (InlanesLit[CurrentPlayer]) {
          Bonus[CurrentPlayer] += 3;
          if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
          CurrentScores[CurrentPlayer] += 3000;
          PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
        } else {
          CurrentScores[CurrentPlayer] += 500;
          PlaySoundEffect(SOUND_EFFECT_INLANE_UNLIT);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_LEFT_OUTLANE:
      case SW_RIGHT_OUTLANE:
        if (OutlanesLit[CurrentPlayer]) {
          Bonus[CurrentPlayer] += 3;
          if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
          CurrentScores[CurrentPlayer] += 3000;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
        } else {
          CurrentScores[CurrentPlayer] += 500;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_10_PTS:
        CurrentScores[CurrentPlayer] += 10;
        FlipInOutLanesLights();
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_STAR_1:
      case SW_STAR_2:
      case SW_STAR_3:
      case SW_STAR_4:
      case SW_STAR_5:
        HandleStarHit(switchHit);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_OUTHOLE:
        break;
      case SW_RIGHT_SPINNER:
        CurrentScores[CurrentPlayer] += GetNumStarsLit(CurrentPlayer)*200;

        // Allow for right spinner to not start ball save
//        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_LEFT_SPINNER:
        CurrentScores[CurrentPlayer] += (200 + Spinner400Bonus*400);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        if (DropTargetSpinnerGoal && ((CurrentTime-DropTargetSpinnerGoal)/1000)<DROP_TARGET_GOAL_TIME && BonusX[CurrentPlayer]!=6) {
          BonusX[CurrentPlayer] = 6;
          DropTargetSpinnerGoal = 0;
          SetDropTargetRelatedLights(BonusX[CurrentPlayer]);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime+130);
          Spinner400Bonus = 0;
        }
        break;
      case SW_ROLLOVER:
        CurrentScores[CurrentPlayer] += 500;
        if (RolloverLit) {
          Bonus[CurrentPlayer] += 1;
          if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_DROP_TARGET_1:
      case SW_DROP_TARGET_2:
      case SW_DROP_TARGET_3:
      case SW_DROP_TARGET_4:
      case SW_DROP_TARGET_5:
      case SW_DROP_TARGET_6:
        HandleDropTargetHit(switchHit, CurrentPlayer);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_BUMPER:
        CurrentScores[CurrentPlayer] += 100;
        PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
        BumperHits[CurrentPlayer] += 1;
        if (BumperHits[CurrentPlayer]==BUMPER_HITS_UNTIL_INLANES_LIGHT) {
          OutlanesLit[CurrentPlayer] = true;
          FlipInOutLanesLights();
        }
        if (BumperHits[CurrentPlayer]==BUMPER_HITS_UNTIL_ROLLOVER_LIT) {
          RolloverLit = (RolloverLit?false:true);
          if (RolloverLit) BSOS_SetLampState(D2_ADVANCE_BONUS, 1);
          else BSOS_SetLampState(D2_ADVANCE_BONUS, 0);
        }
        LastBumperHitTime = CurrentTime;
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
      case SW_COIN_1:
      case SW_COIN_2:
      case SW_COIN_3:
        AddCredit();
        BSOS_SetDisplayCredits(Credits);
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

  if (bonusAtTop!=Bonus[CurrentPlayer]) {
    ShowBonusOnLadder(Bonus[CurrentPlayer]);
  }

  if (scoreAtTop!=CurrentScores[CurrentPlayer]) {
    BSOS_SetDisplay(CurrentPlayer, CurrentScores[CurrentPlayer]);
    if (CurrentScores[CurrentPlayer]>0) BSOS_SetDisplayBlankByMagnitude(CurrentPlayer, CurrentScores[CurrentPlayer]);
    else BSOS_SetDisplayBlank(CurrentPlayer, 0x30);
    LastTimeScoreChanged = CurrentTime;
    ResetScoreScroll(CurrentPlayer);
  }

  if ((CurrentTime-LastTimeScoreChanged)>5000) {
    if (CurrentScores[CurrentPlayer]>999999) ScrollScore(CurrentScores[CurrentPlayer], CurrentPlayer);
  }

  BSOS_ApplyFlashToLamps(CurrentTime);

  return returnState;
}


void loop() {

//  loopCount = loopCount + 1;
  BSOS_DataRead(0);
  CurrentTime = millis();
  int newMachineState = MachineState;


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

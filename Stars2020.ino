#include "BallySternOS.h"
#include "SternStarsDefinitions.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>
#include <AltSoftSerial.h>
#include <wavTrigger.h>


wavTrigger wTrig;             // Our WAV Trigger object

#define DEBUG_MESSAGES  0

/*********************************************************************
*
*   Game specific code
*
*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_UNVALIDATED     3
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_WIZARD_MODE     5
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#define MACHINE_STATE_TEST_FREEPLAY         -16
#define MACHINE_STATE_TEST_BALL_SAVE        -17
#define MACHINE_STATE_TEST_MUSIC_LEVEL      -18
#define MACHINE_STATE_TEST_SKILL_SHOT_AWARD -19
#define MACHINE_STATE_TEST_TILT_WARNING     -20
#define MACHINE_STATE_TEST_AWARD_OVERRIDE   -21
#define MACHINE_STATE_TEST_BALLS_OVERRIDE   -22

#define EEPROM_BALL_SAVE_BYTE       100
#define EEPROM_FREE_PLAY_BYTE       101
#define EEPROM_MUSIC_LEVEL_BYTE     102
#define EEPROM_SKILL_SHOT_BYTE      103
#define EEPROM_TILT_WARNING_BYTE    104
#define EEPROM_AWARD_OVERRIDE_BYTE  105
#define EEPROM_BALLS_OVERRIDE_BYTE  106


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
#define SOUND_EFFECT_TILT_WARNING   28
#define SOUND_EFFECT_WIZARD_SCORE   29
#define SOUND_EFFECT_MATCH_SPIN     30
#define SOUND_EFFECT_WIZARD_TIMER   31

#define BUMPER_HITS_UNTIL_INLANES_FLASH 60
#define BUMPER_HITS_UNTIL_INLANES_LIGHT 40
#define BUMPER_HITS_UNTIL_ROLLOVER_LIT 20

#define SKILL_SHOT_DURATION 15
#define DROP_TARGET_GOAL_TIME 20

#define TILT_WARNING_DEBOUNCE_TIME 1000

unsigned long HighScore = 0;
unsigned long AwardScores[3];
byte Credits = 0;
boolean FreePlayMode = false;

byte dipBank0, dipBank1, dipBank2, dipBank3;
byte MusicLevel = 2;
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
unsigned long WizardModeStartTime = 0;

#define MAX_BONUS           55
#define MAX_DISPLAY_BONUS   55

byte MaximumCredits = 5;
byte BallsPerGame = 3;
boolean CreditDisplay = false;
byte ScoreAwardReplay = 0;
boolean HighScoreReplay = false;
boolean MatchFeature = false;
boolean GameMelodyMinimal = false;
byte SpecialLightAward = 0;
boolean WowExtraBall = false;
boolean BonusCountdown1000Steps = false;
boolean BothTargetSetsFor3X = true;
boolean MaximumNumber4Players = false;
boolean StarSpecialOncePerBall = false;

byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
unsigned long LastTiltWarningTime = 0;

byte RovingStarLight = 0;
boolean SkillShotRunning = false;
boolean StarLevelUpMode = false;
byte SkillShotAwardsLevel = 0;
unsigned int WizardSwitchReward = 50000;
unsigned long WizardModeTimeLimit = 30000;



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

  dipBank0 = BSOS_GetDipSwitches(0);
  ScoreAwardReplay = (dipBank0&0x20) ? 7 : 0;
  BallsPerGame = (dipBank0&40)?5:3;
  GameMelodyMinimal = (dipBank0&0x80)?false:true;
  
  dipBank1 = BSOS_GetDipSwitches(1);
  HighScoreReplay = (dipBank1&0x40)?true:false;
  
  dipBank2 = BSOS_GetDipSwitches(2);
  MaximumCredits = (dipBank2&0x07)*5 + 5;
  CreditDisplay = (dipBank2&0x08)?true:false;
  MatchFeature = (dipBank2&0x10)?true:false;
  BonusCountdown1000Steps = (dipBank2&0x20)?true:false;
  BothTargetSetsFor3X = (dipBank2&80)?true:false;

  dipBank3 = BSOS_GetDipSwitches(3);
  MaximumNumber4Players = (dipBank3&0x01)?true:false;
  WowExtraBall = (dipBank3&0x02)?true:false;
  StarSpecialOncePerBall = (dipBank3&0x20)?true:false;
  SpecialLightAward = (dipBank3)>>6;
  
  HighScore = BSOS_ReadHighScoreFromEEProm();
  Credits = BSOS_ReadCreditsFromEEProm();  
  if (Credits>MaximumCredits) {
    Credits = MaximumCredits;
    BSOS_WriteCreditsToEEProm(Credits);
  }

  for (int count=0; count<4; count++) {
    BSOS_SetDisplay(count, HighScore);
    BSOS_SetDisplayBlankByMagnitude(count, HighScore);
  }

  // These settings should be setable in Self Test
  if (EEPROM.read(EEPROM_FREE_PLAY_BYTE)==0xFF) {
    EEPROM.write(EEPROM_FREE_PLAY_BYTE, 0);
  }
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;
  
  BallSaveNumSeconds = EEPROM.read(EEPROM_BALL_SAVE_BYTE);
  if (BallSaveNumSeconds==0xFF) {
    BallSaveNumSeconds = 16;
    EEPROM.write(EEPROM_BALL_SAVE_BYTE, BallSaveNumSeconds);
  }
  BallSaveUsed = false;

  MaxTiltWarnings = EEPROM.read(EEPROM_TILT_WARNING_BYTE);
  if (MaxTiltWarnings==0xFF) {
    MaxTiltWarnings = 2;
    EEPROM.write(EEPROM_TILT_WARNING_BYTE, MaxTiltWarnings);
  }

  MusicLevel = EEPROM.read(EEPROM_MUSIC_LEVEL_BYTE);
  if (MusicLevel==0xFF) {
    MusicLevel = 2;
    EEPROM.write(EEPROM_MUSIC_LEVEL_BYTE, MusicLevel);
  }

  byte AwardOverride = EEPROM.read(EEPROM_AWARD_OVERRIDE_BYTE);
  if (AwardOverride==0xFF) {
    EEPROM.write(EEPROM_AWARD_OVERRIDE_BYTE, 99);
    AwardOverride = 99;
  }

  if (AwardOverride!=99) {
    ScoreAwardReplay = AwardOverride;
  }

  byte BallsOverride = EEPROM.read(EEPROM_BALLS_OVERRIDE_BYTE);
  if (BallsOverride==0xFF) {
    EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
    BallsOverride = 99;
  }

  if (BallsOverride!=99) {
    BallsPerGame = BallsOverride;
  }

  SkillShotAwardsLevel = EEPROM.read(EEPROM_SKILL_SHOT_BYTE);
  if (SkillShotAwardsLevel==0xFF) {
    EEPROM.write(EEPROM_SKILL_SHOT_BYTE, 0);
    SkillShotAwardsLevel = 0;
  }

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);

  for (byte count=0; count<4; count++) {
    CurrentScores[count] = 0;
  }


  // WAV Trigger startup at 57600
/*  wTrig.start();
  delay(10);
  
  // Send a stop-all command and reset the sample-rate offset, in case we have
  //  reset while the WAV Trigger was already playing.
  wTrig.stopAllTracks();
  wTrig.samplerateOffset(0);  
*/
  
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);
}


void AddCoinToAudit(byte switchHit) {

  unsigned short coinAuditStartByte = 0;

  switch (switchHit) {
    case SW_COIN_3: coinAuditStartByte = BSOS_CHUTE_3_COINS_START_BYTE; break;
    case SW_COIN_2: coinAuditStartByte = BSOS_CHUTE_2_COINS_START_BYTE; break;
    case SW_COIN_1: coinAuditStartByte = BSOS_CHUTE_1_COINS_START_BYTE; break;
  }

  if (coinAuditStartByte) {
    BSOS_WriteULToEEProm(coinAuditStartByte, BSOS_ReadULFromEEProm(coinAuditStartByte) + 1);
  }
  
}



int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState>=MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET);  
  } else {
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
//      if (returnState==MACHINE_STATE_TEST_DONE) returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);        
      }
      BSOS_SetDisplayCredits(abs(curState)-4.);
      BSOS_SetDisplayBallInPlay(0, false);
    }    

    if (curState==MACHINE_STATE_TEST_FREEPLAY) {
      if (curStateChanged) {
        BSOS_SetDisplay(0, FreePlayMode);
        BSOS_SetDisplayBlank(0, 0x20); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        FreePlayMode = (FreePlayMode) ? false : true;
        BSOS_SetDisplay(0, FreePlayMode);
        BSOS_SetDisplayBlank(0, 0x20); 
        EEPROM.write(EEPROM_FREE_PLAY_BYTE, FreePlayMode);
      }
    } else if (curState==MACHINE_STATE_TEST_BALL_SAVE) {
      if (curStateChanged) {
        BSOS_SetDisplay(0, BallSaveNumSeconds);
        BSOS_SetDisplayBlankByMagnitude(0, BallSaveNumSeconds); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        if (BallSaveNumSeconds==0) {
          BallSaveNumSeconds = 6;
        } else if (BallSaveNumSeconds>=21) {
          BallSaveNumSeconds = 0;
        } else {
          BallSaveNumSeconds+=5;
        }
        BSOS_SetDisplay(0, BallSaveNumSeconds);
        BSOS_SetDisplayBlankByMagnitude(0, BallSaveNumSeconds); 
        EEPROM.write(EEPROM_BALL_SAVE_BYTE, BallSaveNumSeconds);
      }
    } else if (curState==MACHINE_STATE_TEST_MUSIC_LEVEL) {
      if (curStateChanged) {
        BSOS_SetDisplay(0, MusicLevel);
        BSOS_SetDisplayBlank(0, 0x20); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        MusicLevel += 1;
        if (MusicLevel>2) MusicLevel = 0;
        BSOS_SetDisplay(0, MusicLevel);
        BSOS_SetDisplayBlank(0, 0x20); 
        EEPROM.write(EEPROM_MUSIC_LEVEL_BYTE, MusicLevel);
      }
    } else if (curState==MACHINE_STATE_TEST_SKILL_SHOT_AWARD) {
      if (curStateChanged) {
        BSOS_SetDisplay(0, SkillShotAwardsLevel);
        BSOS_SetDisplayBlank(0, 0x20); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        SkillShotAwardsLevel += 1;
        if (SkillShotAwardsLevel>1) SkillShotAwardsLevel = 0;
        BSOS_SetDisplay(0, SkillShotAwardsLevel);
        BSOS_SetDisplayBlank(0, 0x20); 
        EEPROM.write(EEPROM_SKILL_SHOT_BYTE, SkillShotAwardsLevel);
      }
    } else if (curState==MACHINE_STATE_TEST_TILT_WARNING) {
      if (curStateChanged) {
        BSOS_SetDisplay(0, MaxTiltWarnings);
        BSOS_SetDisplayBlank(0, 0x20); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        MaxTiltWarnings += 1;
        if (MaxTiltWarnings>2) MaxTiltWarnings = 0;
        BSOS_SetDisplay(0, MaxTiltWarnings);
        BSOS_SetDisplayBlank(0, 0x20); 
        EEPROM.write(EEPROM_TILT_WARNING_BYTE, MaxTiltWarnings);
      }
    } else if (curState==MACHINE_STATE_TEST_AWARD_OVERRIDE) {
      if (curStateChanged) {        
        BSOS_SetDisplay(0, EEPROM.read(EEPROM_AWARD_OVERRIDE_BYTE));
        BSOS_SetDisplayBlank(0, 0x30); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        ScoreAwardReplay += 1;
        if (ScoreAwardReplay==8) ScoreAwardReplay = 99;
        if (ScoreAwardReplay>99) ScoreAwardReplay = 0;
        BSOS_SetDisplay(0, ScoreAwardReplay);
        BSOS_SetDisplayBlank(0, 0x30); 
        EEPROM.write(EEPROM_AWARD_OVERRIDE_BYTE, ScoreAwardReplay);
      }
    } else if (curState==MACHINE_STATE_TEST_BALLS_OVERRIDE) {
      if (curStateChanged) {        
        BSOS_SetDisplay(0, EEPROM.read(EEPROM_BALLS_OVERRIDE_BYTE));
        BSOS_SetDisplayBlank(0, 0x30); 
      }
      if (curSwitch==SW_CREDIT_RESET) {
        switch (BallsPerGame) {
          case 3: BallsPerGame = 5; break;
          case 5: BallsPerGame = 99; break;
          case 99: BallsPerGame = 3; break;
        }
        BSOS_SetDisplay(0, BallsPerGame);
        BSOS_SetDisplayBlank(0, 0x30); 
        EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, BallsPerGame);
      }
    } else {
      HighScore = BSOS_ReadHighScoreFromEEProm();
      Credits = BSOS_ReadCreditsFromEEProm();  
      AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
      AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
      AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);
      if (ScoreAwardReplay==99) {
        ScoreAwardReplay = (dipBank0&0x20)?true:false;
      }
      if (BallsPerGame==99) {
        BallsPerGame = (dipBank0&40)?5:3;        
      }
      returnState = MACHINE_STATE_ATTRACT;
    }
  }

  return returnState;
}


void AddCredit() {
  if (Credits<MaximumCredits) {
    Credits++;
    BSOS_WriteCreditsToEEProm(Credits);
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
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
    }
    AttractLastHeadMode = 1;
    if (HighScore>999999) {
      ScrollScore(HighScore, 5);
    }

    
  } else {
    if (AttractLastHeadMode!=2) {
      BSOS_SetLampState(HIGHEST_SCORE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
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
      CurrentNumPlayers = 0;
      if (AddPlayer()) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      AddCoinToAudit(switchHit);
      AddCredit();
      BSOS_SetDisplayCredits(Credits, true);
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
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

  if (MusicLevel==0) return;
  
  switch (soundEffectNum) {
    case SOUND_EFFECT_STAR_REWARD:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+150);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+350);
        for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500+150*count);
      }
      break;
    case SOUND_EFFECT_HIT_STAR_LEVEL_UP:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+150);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+350);
        for (int count=0; count<5; count++) {
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500+150*count);
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+575+150*count);
        }
      }
      break;
    case SOUND_EFFECT_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      break;
    case SOUND_EFFECT_2X_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+100);
      break;
    case SOUND_EFFECT_3X_BONUS_COUNT:
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+200);
      break;
    case SOUND_EFFECT_INLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      if (MusicLevel>1) for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+150*count);
     break;
    case SOUND_EFFECT_MISSED_STAR_LEVEL_UP:  
    case SOUND_EFFECT_OUTLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      break;
    case SOUND_EFFECT_INLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      if (MusicLevel>1) for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+150*count);
      break;
    case SOUND_EFFECT_OUTLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
      break;
    case SOUND_EFFECT_SKILL_SHOT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+100);
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+200);
      if (MusicLevel>1) for (int count=0; count<15; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+75*count);
      break;
    case SOUND_EFFECT_BUMPER_HIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      break;
    case SOUND_EFFECT_7K_BONUS:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100);
      if (MusicLevel>1) for (int count=0; count<7; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100+250*count);
      break;
    case SOUND_EFFECT_EXTRA_BALL:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100);
      if (MusicLevel>1) for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, CurrentTime+100+100*count);
      break;
    case SOUND_EFFECT_DROP_TARGET:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      if (MusicLevel>1) for (int count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+200*count);
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
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+325, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+450, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+575, true);
        for (int count=0; count<(1+soundEffectNum-SOUND_EFFECT_ADD_PLAYER_1); count++) {
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+800+(count*300), true);
        }
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
      if (MusicLevel>1) {
        for (int count=0; count<(1+soundEffectNum-SOUND_EFFECT_PLAYER_1_UP); count++) {
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300+count*150, true);
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+300+count*150, true);
        }
      }
      break;
    case SOUND_EFFECT_BALL_OVER:   
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+166, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+250, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+750, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+1000, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1166, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+1250, true);
      }
      break;
    case SOUND_EFFECT_GAME_OVER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+166, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+250, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+500, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+666, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+750, true);
      }
      break;
    case SOUND_EFFECT_MACHINE_START:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+150, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+300, true);
      if (MusicLevel>1) {
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
      }
      break;
    case SOUND_EFFECT_TILT_WARNING:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+300);
      }
      break;
    case SOUND_EFFECT_WIZARD_SCORE:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+200, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime+400, true);
      break;
    case SOUND_EFFECT_MATCH_SPIN:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime, true);
      break;
    case SOUND_EFFECT_WIZARD_TIMER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime, true);
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
    BSOS_EnableSolenoidStack(); 
    BSOS_SetDisplayCredits(Credits, true);
    SetCurrentPlayerLamp(playerNum);
    
    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(BALL_IN_PLAY, 1);
    BSOS_SetDisplayBIPBlank(1);
    BSOS_SetLampState(SPECIAL_PURPLE_STAR, 0);
    BSOS_SetLampState(SPECIAL_WHITE_STAR, 0);
    BSOS_SetLampState(SPECIAL_GREEN_STAR, 0);
    BSOS_SetLampState(SPECIAL_AMBER_STAR, 0);
    BSOS_SetLampState(SPECIAL_YELLOW_STAR, 0);
    BSOS_SetLampState(TILT, 0);
  
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
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    for (int count=0; count<5; count++) {    
      SetStarLampState(count, (StarHit[count][playerNum])?1:0, (StarHit[count][playerNum]==1?true:false), (StarHit[count][playerNum]==3?500:0));
    }
  
  
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
    }
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime + 20);
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime + 150);

  }

  
  // We should only consider the ball initialized when 
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }
  
}



unsigned long LastRovingStarLightReportTime = 0;
int RovingStarPhase = 0;
int RovingStarPeriod = 1000;

void ShowStarGoalCompleteLights() {
  BSOS_SetLampState(SPECIAL_PURPLE_STAR, 1, 0, 400);
  BSOS_SetLampState(SPECIAL_WHITE_STAR, 1, 0, 450);
  BSOS_SetLampState(SPECIAL_GREEN_STAR, 1, 0, 500);
  BSOS_SetLampState(SPECIAL_AMBER_STAR, 1, 0, 550);
  BSOS_SetLampState(SPECIAL_YELLOW_STAR, 1, 0, 600);
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
    BSOS_WriteCreditsToEEProm(Credits);
    BSOS_SetDisplayCredits(Credits);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1+(CurrentNumPlayers-1));
  SetNumPlayersLamp(CurrentNumPlayers);

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

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
    int bumperHitsLeft = BUMPER_HITS_UNTIL_INLANES_FLASH - BumperHits[CurrentPlayer];
    if (bumperHitsLeft>=0) {
      if (LastReportedValue!=bumperHitsLeft) {
        BSOS_SetDisplayCredits(bumperHitsLeft, true);
        CurrentlyShowingBumperHits = true;
        LastReportedValue = bumperHitsLeft;
      } else {
        BSOS_SetDisplayFlashCredits(CurrentTime);
      }
      CurrentlyShowingBumperHits = true;      
    }
    
  } else if (CurrentlyShowingBumperHits) {
    BSOS_SetDisplayCredits(Credits, true);
    CurrentlyShowingBumperHits = false;
    LastReportedValue = 0;
  }

  // if we're in ball save time
  if ( BallFirstSwitchHitTime>0 && BallSaveNumSeconds>0 && ((CurrentTime-BallFirstSwitchHitTime)/1000)<BallSaveNumSeconds && !BallSaveUsed ) {
    int tenthsLeft = ((int)BallSaveNumSeconds)*10 - (CurrentTime-BallFirstSwitchHitTime)/100;
    if (LastReportedValue!=tenthsLeft) {
      CurrentlyShowingBallSave = true;
      if (tenthsLeft>99) {
        BSOS_SetDisplayCredits(tenthsLeft/10, true);
      } else {
        BSOS_SetDisplayCredits(tenthsLeft, true);
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
    BSOS_SetDisplayCredits(Credits, true);
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
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime-BallTimeInTrough)>500) {

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

          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
        } else {
          returnState = MACHINE_STATE_COUNTDOWN_BONUS;
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }
  
  BSOS_ApplyFlashToLamps(CurrentTime);


  // Check for Wizard Mode
  if (  BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_FLASH && 
        BonusX[CurrentPlayer]==6 &&
        StarGoalComplete[CurrentPlayer] ) {
    WizardModeStartTime = CurrentTime;
    returnState = MACHINE_STATE_WIZARD_MODE;
  }

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
    BSOS_SetLampState(BALL_IN_PLAY, 0);

    CountdownStartTime = CurrentTime;
    ShowBonusOnLadder(Bonus[CurrentPlayer]);
    
    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  if ((CurrentTime-LastCountdownReportTime)>300) {
    
    if (Bonus[CurrentPlayer]>0) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings<=MaxTiltWarnings) {
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
  boolean inlanesOn = (BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_FLASH) ? true : false;
  
  if (InlanesLit[CurrentPlayer]) {
    if (!inlanesOn) {
      InlanesLit[CurrentPlayer] = false;
      BSOS_SetLampState(IN_LANES, 0);
      OutlanesLit[CurrentPlayer] = true;
      BSOS_SetLampState(OUT_LANES, 1);
    } else {
      InlanesLit[CurrentPlayer] = true;
      BSOS_SetLampState(IN_LANES, 1, 0, 300);
      OutlanesLit[CurrentPlayer] = false;
      BSOS_SetLampState(OUT_LANES, 0);
    }
  } else if (OutlanesLit[CurrentPlayer]) {
    if (!inlanesOn) {
      InlanesLit[CurrentPlayer] = true;
      BSOS_SetLampState(IN_LANES, 1);
      OutlanesLit[CurrentPlayer] = false;
      BSOS_SetLampState(OUT_LANES, 0);
    } else {
      InlanesLit[CurrentPlayer] = true;
      BSOS_SetLampState(IN_LANES, 1, 0, 300);
      OutlanesLit[CurrentPlayer] = false;
      BSOS_SetLampState(OUT_LANES, 0);
    }
  } else {
    BSOS_SetLampState(IN_LANES, 0);
    BSOS_SetLampState(OUT_LANES, 0);
  }
}


void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count=0; count<CurrentNumPlayers; count++) {
    if (CurrentScores[count]>highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore>HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      Credits+=3;
      if (Credits>MaximumCredits) Credits = MaximumCredits;
      BSOS_WriteCreditsToEEProm(Credits);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteHighScoreToEEProm(highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count=0; count<4; count++) {
      if (count==highScorePlayerNum) {
        BSOS_SetDisplayBlankByMagnitude(count, CurrentScores[count], 2);
      } else {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }
    
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

  boolean triggerAllStars = false;

  if (!StarLevelUpMode) {
    if (SkillShotRunning && CheckForRovingStarHit(rovingStarCheck)) {
      SkillShotRunning = false;
      RovingStarLight = 0;
      ShowRovingStarLight();
      CurrentScores[CurrentPlayer] += 25000;
      PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
      Bonus[CurrentPlayer] += 1;
      if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
      if (SkillShotAwardsLevel) triggerAllStars = true;
    } else {
      PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
      CurrentScores[CurrentPlayer] += 400 + 100*((unsigned long)nextStarLevel);
      Bonus[CurrentPlayer] += 1;
      if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
    }
  
    nextStarLevel = GetNextStarLevel(starID, CurrentPlayer);
    StarHit[starID][CurrentPlayer] = nextStarLevel;
    SetStarLampState(starID, 1, (nextStarLevel==1?true:false), (nextStarLevel==3?500:0));

    if (triggerAllStars) {
      for (int starCount=0; starCount<5; starCount++) {
        StarHit[starCount][CurrentPlayer] = nextStarLevel;
        SetStarLampState(starCount, 1, (nextStarLevel==1?true:false), (nextStarLevel==3?500:0));
      }
    }

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
      CurrentScores[CurrentPlayer] += ((unsigned long)StarHit[0][CurrentPlayer])*25000;
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

    ScrollIndex[displayNum]+=1;
    if (ScrollIndex[displayNum]>12) ScrollIndex[displayNum] = 0;
    LastTimeScoreScrolled[displayNum] = CurrentTime;
  }
}

unsigned long LastWizardTimeReported = 0;
int WizardMode() {
  int returnState = MACHINE_STATE_WIZARD_MODE;

  // Show timer & play sound for Wizard Mode
  if ((CurrentTime-LastWizardTimeReported)>250) {
    unsigned long numSeconds = 1 + ((WizardModeTimeLimit - (CurrentTime-WizardModeStartTime))/1000);
    byte offset = ((CurrentTime-WizardModeStartTime)/250)%8;
    if (offset>4) offset = 8 - offset;
    unsigned long shift10 = 1;
    for (int powCount=0; powCount<offset; powCount++) {
      shift10 *= 10;
    }

    if (  (numSeconds>20 && (offset==0)) ||
          (numSeconds>10 && numSeconds<=20 && (offset==0 || offset==4)) ||
          (numSeconds<=10 && (offset%2)==0) ) {
      PlaySoundEffect(SOUND_EFFECT_WIZARD_TIMER);
    }

    for (int count=0; count<4; count++) {
      if (count!=CurrentPlayer) {
        BSOS_SetDisplay(count, numSeconds * shift10);
        if (numSeconds<10) BSOS_SetDisplayBlank(count, 0x20>>offset);
        else BSOS_SetDisplayBlank(count, 0x30>>offset);
      }
    }
    LastWizardTimeReported = CurrentTime;
  }

  if ((CurrentTime-WizardModeStartTime)>WizardModeTimeLimit) {
    for (int count=0; count<4; count++) {
      BSOS_SetDisplay(count, CurrentScores[count]);
      if (count<CurrentNumPlayers) {
        if (CurrentScores[count]>0) BSOS_SetDisplayBlankByMagnitude(count, CurrentScores[count]);
        else BSOS_SetDisplayBlank(count, 0x30);
      }
      else BSOS_SetDisplayBlank(count, 0x00);
    }

    // Reset star goals and bumper stuff
    for (int starCount=0; starCount<5; starCount++) {
      StarHit[starCount][CurrentPlayer] = 0;
    }
    for (int levelCount=0; levelCount<5; levelCount++) {
      StarLevelValidated[levelCount][CurrentPlayer] = false;
    }

    StarGoalComplete[CurrentPlayer] = false;      
    InlanesLit[CurrentPlayer] = false;
    OutlanesLit[CurrentPlayer] = false;
    BumperHits[CurrentPlayer] = 0;

    // Put star lights and inlane lights back to normal
    BSOS_SetLampState(SPECIAL_PURPLE_STAR, 0);
    BSOS_SetLampState(SPECIAL_WHITE_STAR, 0);
    BSOS_SetLampState(SPECIAL_GREEN_STAR, 0);
    BSOS_SetLampState(SPECIAL_AMBER_STAR, 0);
    BSOS_SetLampState(SPECIAL_YELLOW_STAR, 0);
    BSOS_SetLampState(STAR_WHITE, 0);
    BSOS_SetLampState(STAR_GREEN, 0);
    BSOS_SetLampState(STAR_AMBER, 0);
    BSOS_SetLampState(STAR_YELLOW, 0);
    BSOS_SetLampState(STAR_PURPLE, 0);
    FlipInOutLanesLights();
    returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
  }

  return returnState;
}

unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;
  
  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = random(0,9);
    NumMatchSpins = 0;
    BSOS_SetLampState(MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins<40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit>9) MatchDigit = 0;
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      BSOS_SetDisplayBallInPlay((int)MatchDigit*10);
      MatchDelay += 50 + 4*NumMatchSpins;
      NumMatchSpins += 1;
      BSOS_SetLampState(MATCH, NumMatchSpins%2, 0);

      if (NumMatchSpins==40) {
        BSOS_SetLampState(MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins>=40 && NumMatchSpins<=43) {
    if (CurrentTime>(MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers>(NumMatchSpins-40)) && ((CurrentScores[NumMatchSpins-40]/10)%10)==MatchDigit) {
        ScoreMatches |= (1<<(NumMatchSpins-40));
        Credits+=1;
        if (Credits>MaximumCredits) Credits = MaximumCredits;
        BSOS_WriteCreditsToEEProm(Credits);
        BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
        BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
        MatchDelay += 1000;
        NumMatchSpins += 1;
        BSOS_SetLampState(MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins==44) {
        MatchDelay += 5000;
      }
    }      
  }

  if (NumMatchSpins>43) {
    if (CurrentTime>(MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }    
  }

  for (int count=0; count<4; count++) {
    if ((ScoreMatches>>count)&0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime/200)%2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x0F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  
  return MACHINE_STATE_MATCH_MODE;
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
  } else if (curState==MACHINE_STATE_WIZARD_MODE) {
    returnState = WizardMode();
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
        BSOS_SetLampState(PLAYER_1, 0);
        BSOS_SetLampState(PLAYER_2, 0);
        BSOS_SetLampState(PLAYER_3, 0);
        BSOS_SetLampState(PLAYER_4, 0);
        for (int count=0; count<CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count]);
          BSOS_SetDisplayBlankByMagnitude(count, CurrentScores[count], 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState==MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);    
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

  if (NumTiltWarnings<=MaxTiltWarnings) {
    while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
  
      switch (switchHit) {
        case SW_SLAM:
          BSOS_DisableSolenoidStack();
          BSOS_SetDisableFlippers(true);
          BSOS_TurnOffAllLamps();
          BSOS_SetLampState(GAME_OVER, 1);
          delay(1000);
          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // This should be debounced
          if (CurrentTime-LastTiltWarningTime > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings>MaxTiltWarnings) {
              BSOS_DisableSolenoidStack();
              BSOS_SetDisableFlippers(true);
              BSOS_TurnOffAllLamps();
              BSOS_SetLampState(TILT, 1);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break; 
        case SW_LEFT_INLANE:
        case SW_RIGHT_INLANE:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
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
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          }
          break;
        case SW_LEFT_OUTLANE:
        case SW_RIGHT_OUTLANE:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
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
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          }
          break;
        case SW_10_PTS:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 10;
            FlipInOutLanesLights();
            if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          }
          break;
        case SW_STAR_1:
        case SW_STAR_2:
        case SW_STAR_3:
        case SW_STAR_4:
        case SW_STAR_5:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            HandleStarHit(switchHit);
            if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          }
          break;
        case SW_OUTHOLE:
          break;
        case SW_RIGHT_SPINNER:
          CurrentScores[CurrentPlayer] += ((unsigned long)GetNumStarsLit(CurrentPlayer))*200;
          // Allow for right spinner to not start ball save by not setting BallFirstSwitchHitTime
          break;
        case SW_LEFT_SPINNER:
          CurrentScores[CurrentPlayer] += (200 + ((unsigned long)Spinner400Bonus)*400);
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
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 500;
            if (RolloverLit) {
              Bonus[CurrentPlayer] += 1;
              if (Bonus[CurrentPlayer]>MAX_BONUS) Bonus[CurrentPlayer] = MAX_BONUS;
            }
            if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);          
          }
          break;
        case SW_DROP_TARGET_1:
        case SW_DROP_TARGET_2:
        case SW_DROP_TARGET_3:
        case SW_DROP_TARGET_4:
        case SW_DROP_TARGET_5:
        case SW_DROP_TARGET_6:
          HandleDropTargetHit(switchHit, CurrentPlayer);
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          if (curState==MACHINE_STATE_WIZARD_MODE) CurrentScores[CurrentPlayer] += WizardSwitchReward;
          break;
        case SW_BUMPER:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 100;
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
            BumperHits[CurrentPlayer] += 1;
            if (BumperHits[CurrentPlayer]==BUMPER_HITS_UNTIL_INLANES_LIGHT || BumperHits[CurrentPlayer]==BUMPER_HITS_UNTIL_INLANES_FLASH) {
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
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);          
          }
          break;
        case SW_RIGHT_SLING:
        case SW_LEFT_SLING:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 10;
          } else {
            CurrentScores[CurrentPlayer] += WizardSwitchReward;
            PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);          
          }
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
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
  } else {
    // We're tilted, so just wait for outhole
    while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {  
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break; 
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
          AddCredit();
          BSOS_SetDisplayCredits(Credits, true);
          break;
      }
    }
  }
  
  if (bonusAtTop!=Bonus[CurrentPlayer]) {
    ShowBonusOnLadder(Bonus[CurrentPlayer]);
  }

  if (scoreAtTop!=CurrentScores[CurrentPlayer]) {

    for (int awardCount=0; awardCount<3; awardCount++) {
      if (AwardScores[awardCount]!=0 && scoreAtTop<AwardScores[awardCount] && CurrentScores[CurrentPlayer]>=AwardScores[awardCount]) {
        // Player has just passed an award score, so we need to award it
        if (((ScoreAwardReplay>>awardCount)&0x01)==0x01) {
          AddCredit();
          BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
          BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
        } else {
          SamePlayerShootsAgain = true;
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        }
      }
    }
    
    BSOS_SetDisplay(CurrentPlayer, CurrentScores[CurrentPlayer]);
    BSOS_SetDisplayBlankByMagnitude(CurrentPlayer, CurrentScores[CurrentPlayer], 2);
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

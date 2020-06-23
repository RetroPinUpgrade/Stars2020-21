/**************************************************************************
 *     This file is part of Stars2020.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    Stars2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Stars2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#include "BallySternOS.h"
#include "SternStarsDefinitions.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>


// The defines for sound can be used separately or in combination
//#define USE_WAV_TRIGGER
//#define USE_WAV_TRIGGER_1p3
#define USE_CHIMES


#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
#include <wavTrigger.h>
wavTrigger wTrig;             // Our WAV Trigger object
#endif 

#define STARS2020_MAJOR_VERSION  2020
#define STARS2020_MINOR_VERSION  2
#define DEBUG_MESSAGES  0


// This constant defines how much gap is inserted between chime hits 
// during melodies and sound effects
// 40 - quick chimes
// 50 - normal (default)
// 75 - slow chimes
#define CHIME_SPACING_CONSTANT    50


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

#define MACHINE_STATE_ADJUST_FREEPLAY           -16
#define MACHINE_STATE_ADJUST_BALL_SAVE          -17
#define MACHINE_STATE_ADJUST_MUSIC_LEVEL        -18
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -19
#define MACHINE_STATE_ADJUST_REBOOT             -20
#define MACHINE_STATE_ADJUST_SKILL_SHOT_AWARD   -21
#define MACHINE_STATE_ADJUST_NUM_STARS          -22
#define MACHINE_STATE_ADJUST_TILT_WARNING       -23
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     -24
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     -25
#define MACHINE_STATE_ADJUST_SPINNER_CHIME      -26
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   -27
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   -28
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      -29
#define MACHINE_STATE_ADJUST_STAR_LEVEL_AWARD   -30
#define MACHINE_STATE_ADJUST_PLAYFIELD_VALID    -31
#define MACHINE_STATE_ADJUST_WIZARD_DURATION    -32
#define MACHINE_STATE_ADJUST_WIZARD_REWARD      -33
#define MACHINE_STATE_ADJUST_DONE               -34

#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_MUSIC_LEVEL_BYTE         102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_NUM_STARS_BYTE           108
#define EEPROM_SPINNER_CHIME_BYTE       109
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_PLAYFIELD_VALID_BYTE     111
#define EEPROM_WIZARD_DURATION_BYTE     112
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144
#define EEPROM_STAR_LEVEL_AWARD_BYTE    148
#define EEPROM_WIZARD_REWARD_BYTE       152

#define SOUND_EFFECT_NONE               0
#define SOUND_EFFECT_STAR_REWARD        1
#define SOUND_EFFECT_BONUS_COUNT        2
#define SOUND_EFFECT_INLANE_UNLIT       3
#define SOUND_EFFECT_OUTLANE_UNLIT      4
#define SOUND_EFFECT_INLANE_LIT         5
#define SOUND_EFFECT_OUTLANE_LIT        6
#define SOUND_EFFECT_BUMPER_HIT         7
#define SOUND_EFFECT_7K_BONUS           8
#define SOUND_EFFECT_DROP_TARGET        9
#define SOUND_EFFECT_ADD_CREDIT         10
#define SOUND_EFFECT_ADD_PLAYER_1       11
#define SOUND_EFFECT_ADD_PLAYER_2       SOUND_EFFECT_ADD_PLAYER_1+1
#define SOUND_EFFECT_ADD_PLAYER_3       SOUND_EFFECT_ADD_PLAYER_1+2
#define SOUND_EFFECT_ADD_PLAYER_4       SOUND_EFFECT_ADD_PLAYER_1+3
#define SOUND_EFFECT_PLAYER_1_UP        15
#define SOUND_EFFECT_PLAYER_2_UP        SOUND_EFFECT_PLAYER_1_UP+1
#define SOUND_EFFECT_PLAYER_3_UP        SOUND_EFFECT_PLAYER_1_UP+2
#define SOUND_EFFECT_PLAYER_4_UP        SOUND_EFFECT_PLAYER_1_UP+3
#define SOUND_EFFECT_BALL_OVER          19
#define SOUND_EFFECT_GAME_OVER          20
#define SOUND_EFFECT_2X_BONUS_COUNT     21
#define SOUND_EFFECT_3X_BONUS_COUNT     22
#define SOUND_EFFECT_EXTRA_BALL         23
#define SOUND_EFFECT_MACHINE_START      24
#define SOUND_EFFECT_SKILL_SHOT         25
#define SOUND_EFFECT_HIT_STAR_LEVEL_UP  26
#define SOUND_EFFECT_MISSED_STAR_LEVEL_UP   27
#define SOUND_EFFECT_TILT_WARNING       28
#define SOUND_EFFECT_WIZARD_SCORE       29
#define SOUND_EFFECT_MATCH_SPIN         30
#define SOUND_EFFECT_WIZARD_TIMER       31
#define SOUND_EFFECT_SPINNER_HIGH       32
#define SOUND_EFFECT_SPINNER_LOW        33
#define SOUND_EFFECT_SLING_SHOT         34
#define SOUND_EFFECT_ROLLOVER           35
#define SOUND_EFFECT_10PT_SWITCH        36
#define SOUND_EFFECT_BACKGROUND_1       37
#define SOUND_EFFECT_BACKGROUND_2       38
#define SOUND_EFFECT_BACKGROUND_3       39
#define SOUND_EFFECT_BACKGROUND_WIZ     40

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

byte MusicLevel = 2;
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentScores[4];
byte Bonus;
byte BonusX;
byte StarHit[5][4];
boolean StarLevelValidated[5][4];
boolean StarGoalComplete[4];
int BumperHits[4];
boolean InlaneOutlaneToggle = false;
boolean SamePlayerShootsAgain = false;
boolean Left7kLight = false;
boolean Right7kLight = false;
boolean RolloverLit = false;
byte Spinner400Bonus = 0;
boolean NonLeftSpinnerHit = false;

byte BallSaveNumSeconds = 0;
boolean BallSaveUsed = false;
unsigned long BallFirstSwitchHitTime = 0;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long StarLevelAward = 0;

unsigned long CurrentTime = 0;
unsigned long LastBumperHitTime = 0;
unsigned long BallTimeInTrough = 0;
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
boolean TournamentScoring = false;
boolean ResetScoresToClearVersion = false;
boolean ScrollingScores = true;
byte NumStartingStars = 0;
byte PlayfieldValidation = 0;
byte SpinnerChimeBehavior = 1;
byte LeftSpinnerPhase = 0;
byte RightSpinnerPhase = 0;

byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte RovingStarLight = 0;
unsigned long LastTiltWarningTime = 0;

boolean SkillShotRunning = false;
boolean StarLevelUpMode = false;
byte SkillShotAwardsLevel = 0;
unsigned long WizardSwitchReward = 50000;
byte WizardModeTimeLimit = 30;



byte dipBank0, dipBank1, dipBank2, dipBank3;

void GetDIPSwitches() {
  dipBank0 = BSOS_GetDipSwitches(0);
  dipBank1 = BSOS_GetDipSwitches(1);
  dipBank2 = BSOS_GetDipSwitches(2);
  dipBank3 = BSOS_GetDipSwitches(3);
}

void DecodeDIPSwitchParameters() {
  ScoreAwardReplay = (dipBank0&0x20) ? 7 : 0;
  BallsPerGame = (dipBank0&40)?5:3;
  GameMelodyMinimal = (dipBank0&0x80)?false:true;
  
  HighScoreReplay = (dipBank1&0x40)?true:false;
  
  MaximumCredits = (dipBank2&0x07)*5 + 5;
  CreditDisplay = (dipBank2&0x08)?true:false;
  MatchFeature = (dipBank2&0x10)?true:false;
  BonusCountdown1000Steps = (dipBank2&0x20)?true:false;
  BothTargetSetsFor3X = (dipBank2&80)?true:false;

  MaximumNumber4Players = (dipBank3&0x01)?true:false;
  WowExtraBall = (dipBank3&0x02)?true:false;
  StarSpecialOncePerBall = (dipBank3&0x20)?true:false;
  SpecialLightAward = (dipBank3)>>6;
}

void ReadStoredParameters() {
  HighScore = BSOS_ReadULFromEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = BSOS_ReadByteFromEEProm(BSOS_CREDITS_EEPROM_BYTE);
  if (Credits>MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;
  
  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 16);  
  if (BallSaveNumSeconds>21) BallSaveNumSeconds = 16;
  
  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 2);
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (MusicLevel>5) MusicLevel = 5;
#else 
  if (MusicLevel>3) MusicLevel = 3;
#endif

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0))?true:false;
  SkillShotAwardsLevel = (ReadSetting(EEPROM_SKILL_SHOT_BYTE, 0))?true:false;
  NumStartingStars = (ReadSetting(EEPROM_NUM_STARS_BYTE, 0))?1:0;
  
  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);  
  if (MaxTiltWarnings>2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride!=99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride==3 || ballsOverride==5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride!=99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  SpinnerChimeBehavior = ReadSetting(EEPROM_SPINNER_CHIME_BYTE, 1);
  if (SpinnerChimeBehavior>3) SpinnerChimeBehavior = 1;

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1))?true:false;

  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue%1000 || ExtraBallValue>100000) ExtraBallValue = 20000;

  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue%1000 || SpecialValue>100000) SpecialValue = 40000;

  StarLevelAward = BSOS_ReadULFromEEProm(EEPROM_STAR_LEVEL_AWARD_BYTE);
  if (StarLevelAward%1000 || StarLevelAward>100000) StarLevelAward = 0;

  PlayfieldValidation = ReadSetting(EEPROM_PLAYFIELD_VALID_BYTE, 1);
  if (PlayfieldValidation>3) PlayfieldValidation = 1;

  WizardModeTimeLimit = ReadSetting(EEPROM_WIZARD_DURATION_BYTE, 30);
  if (WizardModeTimeLimit>60) WizardModeTimeLimit = 30;

  WizardSwitchReward = BSOS_ReadULFromEEProm(EEPROM_WIZARD_REWARD_BYTE);
  if (WizardSwitchReward%5000 || WizardSwitchReward>100000 || WizardSwitchReward==0) WizardSwitchReward = 50000;

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);
  
}


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  // Start out with everything tri-state, in case the original
  // CPU is running
  // Set data pins to input
  // Make pins 2-7 input
  DDRD = DDRD & 0x03;
  // Make pins 8-13 input
  DDRB = DDRB & 0xC0;
  // Set up the address lines A0-A5 as input (for now)
  DDRC = DDRC & 0xC0;

  unsigned long startTime = millis();
  boolean sawHigh = false;
  boolean sawLow = false;
  // for three seconds, look for activity on the VMA line (A5)
  // If we see anything, then the MPU is active so we shouldn't run
  while ((millis()-startTime)<1200) {
    if (digitalRead(A5)) sawHigh = true;
    else sawLow = true;
  }
  // If we saw both a high and low signal, then someone is toggling the 
  // VMA line, so we should hang here forever (until reset)
  if (sawHigh && sawLow) {
    while (1);
  }
    
  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_STARS_SWITCHES_WITH_TRIGGERS, NUM_STARS_PRIORITY_SWITCHES_WITH_TRIGGERS, StarsSwitches);
 
  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Use dip switches to set up game variables
  GetDIPSwitches();
  DecodeDIPSwitchParameters();

  // Read parameters from EEProm
  ReadStoredParameters();
  
  CurrentScores[0] = STARS2020_MAJOR_VERSION;
  CurrentScores[1] = STARS2020_MINOR_VERSION;
  CurrentScores[2] = BALLY_STERN_OS_MAJOR_VERSION;
  CurrentScores[3] = BALLY_STERN_OS_MINOR_VERSION;
  ResetScoresToClearVersion = true;

  NumStartingStars = 0;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  delay(10);
  
  // Send a stop-all command and reset the sample-rate offset, in case we have
  //  reset while the WAV Trigger was already playing.
  wTrig.stopAllTracks();
  wTrig.samplerateOffset(0);  
#endif 

  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);
}

byte ReadSetting(byte setting, byte defaultValue) {
    byte value = EEPROM.read(setting);
    if (value == 0xFF) {
        EEPROM.write(setting, defaultValue);
        return defaultValue;
    }
    return value;
}

////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
void SetPlayerLamps(byte numPlayers, byte playerOffset=0, int flashPeriod=0) {
  // For Stars, the "Player Up" lights are all +4 of the "Player" lights
  // so this function covers both sets of lights. Putting a 4 in playerOffset
  // will turn on/off the player up lights.
/*  
  BSOS_SetLampState(PLAYER_1+playerOffset, (numPlayers==1)?1:0, 0, flashPeriod);
  BSOS_SetLampState(PLAYER_2+playerOffset, (numPlayers==2)?1:0, 0, flashPeriod);
  BSOS_SetLampState(PLAYER_3+playerOffset, (numPlayers==3)?1:0, 0, flashPeriod);
  BSOS_SetLampState(PLAYER_4+playerOffset, (numPlayers==4)?1:0, 0, flashPeriod);
*/
  for (int count=0; count<4; count++) {
    BSOS_SetLampState(PLAYER_1+playerOffset+count, (numPlayers==(count+1))?1:0, 0, flashPeriod);
  }
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

void ShowStarGoalCompleteLights(byte on) {
  for (int count=0; count<5; count++) BSOS_SetLampState(SPECIAL_PURPLE_STAR+count, on, 0, 400);
}

void SetLeftSpinnerLights(int bonusX) {

  if (bonusX<6) {
    if (DropTargetSpinnerGoal && ((CurrentTime-DropTargetSpinnerGoal)/1000)<DROP_TARGET_GOAL_TIME) {
      int lightSweep = ((CurrentTime-DropTargetSpinnerGoal)/250)%3;
      BSOS_SetLampState(SPECIAL_FEATURE, (lightSweep==0)?1:0);
      BSOS_SetLampState(D400_1_SPINNER, (lightSweep==2)?1:0);
      BSOS_SetLampState(D400_2_SPINNER, (lightSweep==1)?1:0);    
    } else {
      BSOS_SetLampState(SPECIAL_FEATURE, (bonusX>3)?1:0, (bonusX==4)?1:0, (bonusX>5)?500:0);  
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

void SetBonusIndicator(byte number, byte state, byte dim, int flashPeriod=0) {
  if (number==0 || number>10) return;
  BSOS_SetLampState(D1K_BONUS+(number-1), state, dim, flashPeriod);
}


void FlipInOutLanesLights() {
  boolean lanesFlash = (BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_FLASH) ? true : false;
  boolean lanesOn = (BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_LIGHT) ? true : false;

  InlaneOutlaneToggle = (InlaneOutlaneToggle)?false:true; 

  // InlaneOutlaneToggle = true, then inlanes are lit
  // InlaneOutlaneToggle = false, then outlanes are lit
  BSOS_SetLampState(IN_LANES, (lanesOn && InlaneOutlaneToggle), 0, (lanesFlash)?300:0);
  BSOS_SetLampState(OUT_LANES, (lanesOn && !InlaneOutlaneToggle), 0, (lanesFlash)?300:0);
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

boolean AddPlayer(boolean resetNumPlayers=false) {

  if (Credits<1 && !FreePlayMode) return false;
  if (CurrentNumPlayers>=4 || (CurrentNumPlayers>=2 && !MaximumNumber4Players)) return false;

  if (resetNumPlayers) CurrentNumPlayers = 0;
  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers-1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers-1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1+(CurrentNumPlayers-1));
  SetPlayerLamps(CurrentNumPlayers);

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
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


#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;


int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    wTrig.stopAllTracks();
    wTrig.samplerateOffset(0); 
  }
#endif 

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState>=MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);  
  } else {
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch==SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count=0; count<4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);        
      }
      BSOS_SetDisplayCredits(abs(curState)-4.);
      BSOS_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
        break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 6;
          AdjustmentValues[2] = 11;
          AdjustmentValues[3] = 16;
          AdjustmentValues[4] = 21;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;      
        break;
        case MACHINE_STATE_ADJUST_MUSIC_LEVEL:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
          AdjustmentValues[1] = 5;
#else
          AdjustmentValues[1] = 3;
#endif
          CurrentAdjustmentByte = &MusicLevel;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;
        break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
        break;
        case MACHINE_STATE_ADJUST_REBOOT:
          for (byte count=0; count<4; count++) {
            BSOS_SetDisplay(count, 8007, true);
          }
          CurrentAdjustmentByte = 0;
        break;
        case MACHINE_STATE_ADJUST_SKILL_SHOT_AWARD:
          CurrentAdjustmentByte = (byte *)&SkillShotAwardsLevel;
          CurrentAdjustmentStorageByte = EEPROM_SKILL_SHOT_BYTE;
        break;
        case MACHINE_STATE_ADJUST_NUM_STARS:
          CurrentAdjustmentByte = &NumStartingStars;
          CurrentAdjustmentStorageByte = EEPROM_NUM_STARS_BYTE;
        break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
        break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
        break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 99;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
        break;
        case MACHINE_STATE_ADJUST_SPINNER_CHIME:
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &SpinnerChimeBehavior;
          CurrentAdjustmentStorageByte = EEPROM_SPINNER_CHIME_BYTE;
        break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
        break;
        
        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
        break;
        
        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
        break;
        case MACHINE_STATE_ADJUST_STAR_LEVEL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &StarLevelAward;
          CurrentAdjustmentStorageByte = EEPROM_STAR_LEVEL_AWARD_BYTE;      
        break;
        
        case MACHINE_STATE_ADJUST_PLAYFIELD_VALID:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &PlayfieldValidation;
          CurrentAdjustmentStorageByte = EEPROM_PLAYFIELD_VALID_BYTE;
        break;
       
        case MACHINE_STATE_ADJUST_WIZARD_DURATION:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 15;
          AdjustmentValues[2] = 30;
          AdjustmentValues[3] = 45;
          AdjustmentValues[4] = 60;
          CurrentAdjustmentByte = &WizardModeTimeLimit;
          CurrentAdjustmentStorageByte = EEPROM_WIZARD_DURATION_BYTE;
        break;
        
        case MACHINE_STATE_ADJUST_WIZARD_REWARD:
          AdjustmentType = ADJ_TYPE_SCORE_NO_DEFAULT;
          CurrentAdjustmentUL = &WizardSwitchReward;
          CurrentAdjustmentStorageByte = EEPROM_WIZARD_REWARD_BYTE;
        break;

        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
        break;        
      }      

    }

    // Change value, if the switch is hit
    if (curSwitch==SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType==ADJ_TYPE_MIN_MAX || AdjustmentType==ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal>AdjustmentValues[1]) {
          if (AdjustmentType==ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal>99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
      } else if (CurrentAdjustmentByte && AdjustmentType==ADJ_TYPE_LIST) {
        byte valCount=0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount=0; valCount<(NumAdjustmentValues-1); valCount++) {
          if (curVal==AdjustmentValues[valCount]) newIndex = valCount+1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType==ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType==ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal>100000) curVal = 0;
        if (AdjustmentType==ADJ_TYPE_SCORE_NO_DEFAULT && curVal==0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }
      
      if (curState==MACHINE_STATE_ADJUST_REBOOT) {
        returnState = MACHINE_STATE_ATTRACT;     
      }
    }

    // Show current value
    if (CurrentAdjustmentByte!=NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL!=NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (returnState==MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return 
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    DecodeDIPSwitchParameters();
    ReadStoredParameters();    
  }

  return returnState;
}


#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
byte CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif


void PlayBackgroundSong(byte songNum) {
  
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (MusicLevel>4) {
    if (CurrentBackgroundSong!=songNum) {
      if (CurrentBackgroundSong!=SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);
      if (songNum!=SOUND_EFFECT_NONE) {
#ifdef USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else 
        wTrig.trackPlayPoly(songNum);
#endif        
        wTrig.trackLoop(songNum, true);
      }
      CurrentBackgroundSong = songNum;
    }  
  }
#else
  byte test = songNum;
  songNum = test;
#endif

}


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(byte soundEffectNum) {

  if (MusicLevel==0) return;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (MusicLevel>3) {

#ifndef USE_WAV_TRIGGER_1p3    
    if (  soundEffectNum==SOUND_EFFECT_BUMPER_HIT || soundEffectNum==SOUND_EFFECT_ROLLOVER || 
          soundEffectNum==SOUND_EFFECT_10PT_SWITCH || SOUND_EFFECT_SPINNER_HIGH ||
          SOUND_EFFECT_SPINNER_LOW ) wTrig.trackStop(soundEffectNum);
#endif          
    wTrig.trackPlayPoly(soundEffectNum);
  }
#endif 

#ifdef USE_CHIMES
  // If the user selects electronic sounds, don't do chimes
  if (MusicLevel>3) return;

  // Music level 3 = allow melodies to overlap
  if (CurrentTime>NextSoundEffectTime || MusicLevel==3) {
    NextSoundEffectTime = CurrentTime;
  } else if ( (NextSoundEffectTime-CurrentTime)>2000 ) {
    // if we already have two seconds of sound effects
    // lined up, simply return
    return;
  }
  int count = 0;

  unsigned long soundGapUL = (unsigned long)CHIME_SPACING_CONSTANT;
  
  switch (soundEffectNum) {
    case SOUND_EFFECT_STAR_REWARD:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*3);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*5);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*7);
        for (count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*10+soundGapUL*3*count);
        NextSoundEffectTime += (soundGapUL*10+soundGapUL*3*count);
      }
      break;
    case SOUND_EFFECT_HIT_STAR_LEVEL_UP:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*3);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*5);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*7);
        for (count=0; count<5; count++) {
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*10+soundGapUL*3*count);
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*11+soundGapUL*3*count);
        }
        NextSoundEffectTime += (soundGapUL*11+soundGapUL*3*count);
      }
      break;
    case SOUND_EFFECT_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime);
      break;
    case SOUND_EFFECT_2X_BONUS_COUNT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*2);
        NextSoundEffectTime += (soundGapUL*2);
      }
      break;
    case SOUND_EFFECT_3X_BONUS_COUNT:
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*2);
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*4);
      NextSoundEffectTime += (soundGapUL*4);
      break;
    case SOUND_EFFECT_INLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*2);
      NextSoundEffectTime += (soundGapUL*2);
     break;
    case SOUND_EFFECT_MISSED_STAR_LEVEL_UP:  
    case SOUND_EFFECT_OUTLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*2);
      NextSoundEffectTime += soundGapUL*2;
      break;
    case SOUND_EFFECT_INLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*2);
      if (MusicLevel>1) for (count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*6+soundGapUL*3*count);
      NextSoundEffectTime += soundGapUL*6+soundGapUL*3*count;
      break;
    case SOUND_EFFECT_OUTLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*2);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*4);
      NextSoundEffectTime += soundGapUL*4;
      break;
    case SOUND_EFFECT_SKILL_SHOT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*2);
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*4);
      if (MusicLevel>1) for (count=0; count<10; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*6+soundGapUL*3*count/2);
      NextSoundEffectTime += soundGapUL*6+soundGapUL*3*count/2;
      break;
    case SOUND_EFFECT_7K_BONUS:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, NextSoundEffectTime);
      else {
        for (count=0; count<7; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, NextSoundEffectTime+soundGapUL*2+soundGapUL*5*count);
        NextSoundEffectTime += soundGapUL*2+soundGapUL*5*count;
      }
      break;
    case SOUND_EFFECT_EXTRA_BALL:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, NextSoundEffectTime);
      else {
        for (count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 5, NextSoundEffectTime+soundGapUL*2+soundGapUL*2*count);
        NextSoundEffectTime += soundGapUL*2+soundGapUL*2*count;
      }
      break;
    case SOUND_EFFECT_DROP_TARGET:
      if (MusicLevel==1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime);
      else {
        for (count=0; count<5; count++) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*4*count);
        NextSoundEffectTime += soundGapUL*4*count;
      }
      break;
    case SOUND_EFFECT_ADD_CREDIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL+25, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*3, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*5+25, true);
      NextSoundEffectTime += soundGapUL*5+25;
      break;
    case SOUND_EFFECT_ADD_PLAYER_1:
    case SOUND_EFFECT_ADD_PLAYER_2:
    case SOUND_EFFECT_ADD_PLAYER_3:
    case SOUND_EFFECT_ADD_PLAYER_4:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*4, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*6+25, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*9, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*11+25, true);
        NextSoundEffectTime += soundGapUL*7+25;
      }
      NextSoundEffectTime += soundGapUL*4;
      break;
    case SOUND_EFFECT_PLAYER_1_UP:
    case SOUND_EFFECT_PLAYER_2_UP:
    case SOUND_EFFECT_PLAYER_3_UP:
    case SOUND_EFFECT_PLAYER_4_UP:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*3, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*3, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*6, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*6, true);
        NextSoundEffectTime += soundGapUL*3;
      }
      NextSoundEffectTime += soundGapUL*3;
      break;
    case SOUND_EFFECT_BALL_OVER:   
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*3, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*5, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*10, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*15, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*20, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*23, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*25, true);
        NextSoundEffectTime += soundGapUL*22;
      }
      NextSoundEffectTime += soundGapUL*3;
      break;
    case SOUND_EFFECT_GAME_OVER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*3, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*5, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*10, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*12, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*15, true);
        NextSoundEffectTime += soundGapUL*12;
      }
      NextSoundEffectTime += soundGapUL*3;
      break;
    case SOUND_EFFECT_MACHINE_START:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*3, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*6, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*9, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*12, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*15, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*18, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*25, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*28, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*31, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*34, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, NextSoundEffectTime+soundGapUL*37, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*40, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*43, true);
        NextSoundEffectTime += soundGapUL*37;
      }
      NextSoundEffectTime += soundGapUL*6;
      break;
    case SOUND_EFFECT_TILT_WARNING:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*2);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, NextSoundEffectTime+soundGapUL*4);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, NextSoundEffectTime+soundGapUL*6);
        NextSoundEffectTime += soundGapUL*4;
      }
      NextSoundEffectTime += soundGapUL*2;
      break;
    case SOUND_EFFECT_WIZARD_SCORE:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*4, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, NextSoundEffectTime+soundGapUL*8, true);
      NextSoundEffectTime += soundGapUL*8;
      break;

    // Special cases - immediate sounds      
    case SOUND_EFFECT_BUMPER_HIT:
    case SOUND_EFFECT_MATCH_SPIN:
    case SOUND_EFFECT_WIZARD_TIMER:
    case SOUND_EFFECT_ROLLOVER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime, true);
      break;
    case SOUND_EFFECT_SPINNER_HIGH:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10000, 3, CurrentTime, true);
      break;
    case SOUND_EFFECT_SPINNER_LOW:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime, true);
      break;
    case SOUND_EFFECT_10PT_SWITCH:
    case SOUND_EFFECT_SLING_SHOT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 4, CurrentTime, true);
      break;
  }

  NextSoundEffectTime += soundGapUL*3;
#endif 
  
}


void AddCredit(boolean playSound=false, byte numToAdd=1) {
  if (Credits<MaximumCredits) {
    Credits+=numToAdd;
    if (Credits>MaximumCredits) Credits = MaximumCredits;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetDisplayCredits(Credits);
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
      SetPlayerLamps(0);
  
      for (int count=0; count<4; count++) {
        if (CurrentTime>30000) BSOS_SetDisplay(count, HighScore, true, 2);
        else BSOS_SetDisplay(count, CurrentScores[count], true, 2); 
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
      if (ResetScoresToClearVersion==true && CurrentTime>30000) {
        for (int count=0; count<4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
        ResetScoresToClearVersion = false;
      }
      BSOS_SetLampState(HIGHEST_SCORE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
      for (int count=0; count<4; count++) {
        if (CurrentNumPlayers>0 || CurrentTime<30000) {
          if (count<CurrentNumPlayers || CurrentTime<30000) {
            if (CurrentScores[count]>999999) {
              ScrollScore(CurrentScores[count], count);  
            } else {
              BSOS_SetDisplay(count, CurrentScores[count], true, 2); 
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
    SetPlayerLamps(((CurrentTime/250)%4) + 1);
    AttractLastHeadMode = 2;
  }

  if ((CurrentTime/10000)%3<2) {  
    if (AttractLastPlayfieldMode!=1) {
      BSOS_TurnOffAllLamps();
    }
    if ((CurrentTime-AttractLastSweepTime)>50) {
      AttractLastSweepTime = CurrentTime;
      for (int lightcount=0; lightcount<NUM_STARS_LIGHTS; lightcount++) {

        byte dist = AttractSweepLights - StarsLights[lightcount].row;
        BSOS_SetLampState(StarsLights[lightcount].lightNum, (dist<8), (dist==0/*||dist>5*/)?0:dist/3, (dist>5)?(100+StarsLights[lightcount].lightNum):0);
/*        
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
*/        
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

    AttractLastPlayfieldMode = 2;
  }

  byte switchHit;
  while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
    if (switchHit==SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      AddCoinToAudit(switchHit);
      AddCredit(true, 1);
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}



int InitializeGamePlay() {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  BSOS_EnableSolenoidStack();
  BSOS_SetCoinLockout((Credits>=MaximumCredits)?true:false);
  BSOS_TurnOffAllLamps();

  // Turn back on all lamps that are needed
  SetPlayerLamps(1);

  // When we go back to attract mode, there will be no need to reset scores
  ResetScoresToClearVersion = false;

  // Reset displays & game state variables
  for (int count=0; count<4; count++) {
    BSOS_SetDisplay(count, 0);
    if (count==0) BSOS_SetDisplayBlank(count, 0x30);
    else BSOS_SetDisplayBlank(count, 0x00);      

    CurrentScores[count] = 0;

    for (int starCount=0; starCount<5; starCount++) {
      StarHit[starCount][count] = 0;
    }
    for (int levelCount=0; levelCount<5; levelCount++) {
      StarLevelValidated[levelCount][count] = false;
    }
    StarGoalComplete[count] = false;  
    SamePlayerShootsAgain = false;

    InlaneOutlaneToggle = false;

    BumperHits[count] = 0;
  }

  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;

  Left7kLight = false;
  Right7kLight = false;
    
  return MACHINE_STATE_INIT_NEW_BALL;  
}


int InitializeNewBall(bool curStateChanged, byte playerNum, int ballNum) {  

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    SamePlayerShootsAgain = false;
    BallFirstSwitchHitTime = 0;
    
    BSOS_SetDisableFlippers(false);
    BSOS_EnableSolenoidStack(); 
    BSOS_SetDisplayCredits(Credits, true);
    SetPlayerLamps(playerNum+1, 4);
    
    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(BALL_IN_PLAY, 1);
    ShowStarGoalCompleteLights(0);
    BSOS_SetLampState(TILT, 0);
  
    if (BallSaveNumSeconds>0) {
      BSOS_SetLampState(SHOOT_AGAIN, 1, 0, 500);
    }
    
    Left7kLight = false;
    Right7kLight = false;
    Bonus = 0;
    BonusX = 1;
    Spinner400Bonus = 0;
    SetDropTargetRelatedLights(BonusX);
    DropTargetSpinnerGoal = 0;
    FlipInOutLanesLights();
    if (BumperHits[playerNum]>=BUMPER_HITS_UNTIL_ROLLOVER_LIT) RolloverLit = true;
    else RolloverLit = false;
    BSOS_SetLampState(D2_ADVANCE_BONUS, RolloverLit);
    BallSaveUsed = false;
    RovingStarLight = 1;
    SkillShotRunning = true;
    StarLevelUpMode = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;
    NonLeftSpinnerHit = false;

    if (NumStartingStars) {
      boolean starSet = false;
      for (int count=0; count<5; count++) {
        if (StarHit[count][playerNum]!=0) starSet = true;
      }
      if (!starSet) {
        byte randomStar = random(0, 5);
        StarHit[randomStar][playerNum] = 1;
      }
    }
    
    for (int count=0; count<5; count++) {    
      SetStarLampState(count, (StarHit[count][playerNum])?1:0, (StarHit[count][playerNum]==1?true:false), (StarHit[count][playerNum]==3?500:0));
    }
  
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
    }
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime + 20);
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime + 150);

    byte nextStarLevel = GetNextStarLevel(playerNum);
    if (nextStarLevel==1) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
    else if (nextStarLevel==2) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2);
    else if (nextStarLevel==3) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_3);
    
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


void ShowRovingStarLight() {
  if (RovingStarLight==0) {
    ShowStarGoalCompleteLights(0);
    return;
  }

  if ((CurrentTime-LastRovingStarLightReportTime)>(unsigned long)(RovingStarPeriod/5)) {
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


void AddToBonus(byte bonusAddition) {
  Bonus += bonusAddition;
  if (Bonus>MAX_BONUS) Bonus = MAX_BONUS;
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
      BSOS_SetDisplayFlash(CurrentPlayer, CurrentScores[CurrentPlayer], CurrentTime, 500, 2);
    }
    if (!PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1), 4, 500);
      PlayerUpLightBlinking = true;
    }
  } else {
    if (PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1), 4);
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
        case 5:
          BSOS_SetLampState(SHOOT_AGAIN, (tenthsLeft/5)%2);
          break;
        case 4:
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
    }
  } else if (CurrentlyShowingBallSave) {
    BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
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
    if (StarGoalComplete[CurrentPlayer]) ShowStarGoalCompleteLights(1);
  }

  if (StarLevelUpMode) {
    if (RovingStarLight) ShowRovingStarLight();
  }

  // Drop target completion goal
  if (DropTargetSpinnerGoal) {
    SetLeftSpinnerLights(BonusX);
    if (((CurrentTime-DropTargetSpinnerGoal)/1000)>DROP_TARGET_GOAL_TIME) {
      DropTargetSpinnerGoal = 0;
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime+130);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime);
      Spinner400Bonus = 0; 
      SetDropTargetRelatedLights(BonusX);
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

        if (BallFirstSwitchHitTime==0 && NumTiltWarnings<=MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
        } else {        
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (  !BallSaveUsed && 
                ((CurrentTime-BallFirstSwitchHitTime)/1000)<((unsigned long)BallSaveNumSeconds) ) {
          
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            if (BallFirstSwitchHitTime>0) {
              BallSaveUsed = true;
              BSOS_SetLampState(SHOOT_AGAIN, 0);
            }
            BallTimeInTrough = CurrentTime;
  
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
          } else {
            PlayBackgroundSong(SOUND_EFFECT_NONE);
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }
  

  // Check for Wizard Mode
  if (  WizardModeTimeLimit!=0 && 
        BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_FLASH && 
        BonusX==6 &&
        StarGoalComplete[CurrentPlayer] ) {
    WizardModeStartTime = CurrentTime;
    returnState = MACHINE_STATE_WIZARD_MODE;
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_WIZ);
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

byte GetNextStarLevel(int CurrentPlayer) {

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



boolean WaitForDropTargetsToReset = false;

void HandleDropTargetHit(int switchHit, int curPlayer) {
  boolean requireBothSetsOfDrops = false;

  if (BonusX>2 || (BonusX==2 && BothTargetSetsFor3X)) requireBothSetsOfDrops = true;
    
  //CurrentScores[CurrentPlayer] += 500;

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

        if (BonusX==5) {
          DropTargetSpinnerGoal = CurrentTime;
        } else {
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime+130);
        }
        WaitForDropTargetsToReset = true;
        Spinner400Bonus = 0;
        Right7kLight = false;
        Left7kLight = false;

        if (BonusX<5) BonusX += 1;
        
        if (BonusX==4) {
          if (!TournamentScoring) {            
            if (WowExtraBall) {
              SamePlayerShootsAgain = true;
              BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
            } else {
              CurrentScores[curPlayer] += 25000;
            }
          } else {
            CurrentScores[curPlayer] += ExtraBallValue;
          }
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        } else if (BonusX==5) {
          if (!TournamentScoring) {
            AddCredit(false, 1);
            BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
            BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
          } else {
            CurrentScores[curPlayer] += SpecialValue;
          }
        }
      }
    } else {
      if (  ( BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) ) ||
            ( BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) && 
              BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6) ) ) {
        // The BonusX has to be <3 to get here
//        if (BonusX==5) {
//          DropTargetSpinnerGoal = CurrentTime;
//        } else {
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime+130);
          BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime);
//        }
        WaitForDropTargetsToReset = true;
        Spinner400Bonus = 0;
        Right7kLight = false;
        Left7kLight = false;

//        if (BonusX<5) BonusX += 1;
        BonusX += 1;
      }
    }
  }
  
  SetDropTargetRelatedLights(BonusX);

}


unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
    BSOS_SetLampState(BALL_IN_PLAY, 1, 0, 250);

    CountdownStartTime = CurrentTime;
    ShowBonusOnLadder(Bonus);
    
    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  if ((CurrentTime-LastCountdownReportTime)>300) {
    
    if (Bonus>0) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings<=MaxTiltWarnings) {
        if (BonusX==1) {
          PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT);
          CurrentScores[CurrentPlayer] += 1000;
        } else if (BonusX==2) {
          PlaySoundEffect(SOUND_EFFECT_2X_BONUS_COUNT);
          CurrentScores[CurrentPlayer] += 2000;
        } else if (BonusX>2) {
          PlaySoundEffect(SOUND_EFFECT_3X_BONUS_COUNT);
          CurrentScores[CurrentPlayer] += 3000;
        }
      }
      
      Bonus -= 1;
      ShowBonusOnLadder(Bonus);
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
      AddCredit(false, 3);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteULToEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count=0; count<4; count++) {
      if (count==highScorePlayerNum) {
        BSOS_SetDisplay(count, CurrentScores[count], true, 2);
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
      if (SkillShotAwardsLevel) triggerAllStars = true;
    } else {
      PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
      CurrentScores[CurrentPlayer] += 400 + 100*((unsigned long)nextStarLevel);
    }
  
    AddToBonus(1);
    nextStarLevel = GetNextStarLevel(CurrentPlayer);
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
      if (StarLevelAward==0) {
        CurrentScores[CurrentPlayer] += ((unsigned long)StarHit[0][CurrentPlayer])*25000;
      } else {
        CurrentScores[CurrentPlayer] += StarLevelAward;
      }
      StarLevelValidated[StarHit[0][CurrentPlayer]-1][CurrentPlayer] = true;
      
      byte nextStarLevel = GetNextStarLevel(CurrentPlayer);
      if (nextStarLevel==1) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
      else if (nextStarLevel==2) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2);
      else if (nextStarLevel==3) PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_3);
      
      if (StarHit[0][CurrentPlayer]==3) {
        StarGoalComplete[CurrentPlayer] = true;
        ShowStarGoalCompleteLights(1);
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_MISSED_STAR_LEVEL_UP);
      CurrentScores[CurrentPlayer] += 100;
    }
  }
  

}




unsigned long LastWizardTimeReported = 0;
int WizardMode() {
  int returnState = MACHINE_STATE_WIZARD_MODE;

  // Show timer & play sound for Wizard Mode
  if ((CurrentTime-LastWizardTimeReported)>250) {
//    unsigned long numSeconds = 1 + ((WizardModeTimeLimit - (CurrentTime-WizardModeStartTime))/1000);
    unsigned long numSeconds = 1 + (unsigned long)WizardModeTimeLimit - ((CurrentTime-WizardModeStartTime)/1000);
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

  if ((CurrentTime-WizardModeStartTime)>(1000*(unsigned long)WizardModeTimeLimit)) {
    for (int count=0; count<4; count++) {
      BSOS_SetDisplay(count, CurrentScores[count]);
      if (count<CurrentNumPlayers) {
        if (CurrentScores[count]>0) BSOS_SetDisplay(count, CurrentScores[count], true, 2);
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
    InlaneOutlaneToggle = false;
    BumperHits[CurrentPlayer] = 0;

    // Put star lights and inlane lights back to normal
    ShowStarGoalCompleteLights(0);
    BSOS_SetLampState(STAR_WHITE, 0);
    BSOS_SetLampState(STAR_GREEN, 0);
    BSOS_SetLampState(STAR_AMBER, 0);
    BSOS_SetLampState(STAR_YELLOW, 0);
    BSOS_SetLampState(STAR_PURPLE, 0);
    FlipInOutLanesLights();
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
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
    MatchDigit = random(0,10);
    NumMatchSpins = 0;
    BSOS_SetLampState(MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
    BSOS_SetLampState(BALL_IN_PLAY, 0);
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
        AddCredit(false, 1);
        BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
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


void WizardSwitchHit() {
  CurrentScores[CurrentPlayer] += WizardSwitchReward;
  PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);  
}


int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte bonusAtTop = Bonus;
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
        SetPlayerLamps(0);
        for (int count=0; count<CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count], true, 2);
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


  boolean lanesFlash = (BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_FLASH) ? true : false;
  boolean lanesOn = (BumperHits[CurrentPlayer]>=BUMPER_HITS_UNTIL_INLANES_LIGHT) ? true : false;
  byte switchHit;
  unsigned long numStarsLit = (unsigned long)GetNumStarsLit(CurrentPlayer);

  if (NumTiltWarnings<=MaxTiltWarnings) {
    while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
      if (switchHit!=SW_LEFT_SPINNER && switchHit!=SW_RIGHT_SPINNER) NonLeftSpinnerHit = true;
  
      switch (switchHit) {
        case SW_SLAM:
//          BSOS_DisableSolenoidStack();
//          BSOS_SetDisableFlippers(true);
//          BSOS_TurnOffAllLamps();
//          BSOS_SetLampState(GAME_OVER, 1);
//          delay(1000);
//          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // This should be debounced
          if ((CurrentTime-LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
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
            if ( lanesOn && InlaneOutlaneToggle ) {
              AddToBonus(3);
              CurrentScores[CurrentPlayer] += 3000;
              if (lanesFlash) {
                AddToBonus(3);
                CurrentScores[CurrentPlayer] += 3000;
              }
              PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
            } else {
              CurrentScores[CurrentPlayer] += 500;
              PlaySoundEffect(SOUND_EFFECT_INLANE_UNLIT);
            }
            if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            WizardSwitchHit();
          }
          break;
        case SW_LEFT_OUTLANE:
        case SW_RIGHT_OUTLANE:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            if ( lanesOn && !InlaneOutlaneToggle ) {
              AddToBonus(3);
              CurrentScores[CurrentPlayer] += 3000;
              if (lanesFlash) {
                AddToBonus(3);
                CurrentScores[CurrentPlayer] += 3000;
              }
              PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
            } else {
              CurrentScores[CurrentPlayer] += 500;
              PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
            }
            if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            WizardSwitchHit();
          }
          break;
        case SW_10_PTS:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 10;
            FlipInOutLanesLights();
            if (PlayfieldValidation<2 && BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
            PlaySoundEffect(SOUND_EFFECT_10PT_SWITCH);
          } else {
            WizardSwitchHit();
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
            WizardSwitchHit();
          }
          break;
        case SW_OUTHOLE:
          break;
        case SW_RIGHT_SPINNER:
         
          if (numStarsLit) {
            CurrentScores[CurrentPlayer] += (numStarsLit*200);
          } else {
            CurrentScores[CurrentPlayer] += 10;
          }
          // Allow for right spinner to not start ball save by not setting BallFirstSwitchHitTime
          if (SpinnerChimeBehavior==1) {
            RightSpinnerPhase += 1;
            if (RightSpinnerPhase%2) PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (SpinnerChimeBehavior==2) {
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (SpinnerChimeBehavior==3) {
            RightSpinnerPhase += 1;
            if (RightSpinnerPhase%2) PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
            else PlaySoundEffect(SOUND_EFFECT_SPINNER_LOW);
          }
          if ((NumStartingStars || PlayfieldValidation==0) && BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SPINNER:
          if (NonLeftSpinnerHit==false ) {
            CurrentScores[CurrentPlayer] += 1000;
          } else {
            CurrentScores[CurrentPlayer] += (200 + ((unsigned long)Spinner400Bonus)*400);
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          if (DropTargetSpinnerGoal && ((CurrentTime-DropTargetSpinnerGoal)/1000)<DROP_TARGET_GOAL_TIME && BonusX!=6) {
            BonusX = 6;
            DropTargetSpinnerGoal = 0;
            SetDropTargetRelatedLights(BonusX);
            BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 15, CurrentTime);
            BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 15, CurrentTime+130);
            Spinner400Bonus = 0;
          }
          if (SpinnerChimeBehavior==1) {
            LeftSpinnerPhase += 1;
            if (LeftSpinnerPhase%2) PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (SpinnerChimeBehavior==2) {
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (SpinnerChimeBehavior==3) {
            LeftSpinnerPhase += 1;
            if (LeftSpinnerPhase%2) PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
            else PlaySoundEffect(SOUND_EFFECT_SPINNER_LOW);
          }
          break;
        case SW_ROLLOVER:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 500;
            if (RolloverLit) {
              AddToBonus(1);
            }
            if (PlayfieldValidation<2 && BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
            PlaySoundEffect(SOUND_EFFECT_ROLLOVER);
          } else {
            WizardSwitchHit();
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
              FlipInOutLanesLights();
            } else if (BumperHits[CurrentPlayer]==BUMPER_HITS_UNTIL_ROLLOVER_LIT) {
              RolloverLit = true;
              BSOS_SetLampState(D2_ADVANCE_BONUS, RolloverLit);
            } else if (BumperHits[CurrentPlayer]>BUMPER_HITS_UNTIL_INLANES_FLASH) {
              CurrentScores[CurrentPlayer] += 900;
            }
            LastBumperHitTime = CurrentTime;
            if (PlayfieldValidation<2 && BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          } else {
            WizardSwitchHit();
          }
          break;
        case SW_RIGHT_SLING:
        case SW_LEFT_SLING:
          if (curState!=MACHINE_STATE_WIZARD_MODE) {
            CurrentScores[CurrentPlayer] += 10;
            PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);          
          } else {
            WizardSwitchHit();
          }
          if (PlayfieldValidation<2 && BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
          AddCredit(true, 1);
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay<2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else {
            // If the first ball is over, pressing start again resets the game
            if (Credits>=1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
                BSOS_SetDisplayCredits(Credits);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
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
          AddCredit(true, 1);
          break;
      }
    }
  }
  
  if (bonusAtTop!=Bonus) {
    ShowBonusOnLadder(Bonus);
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer]>999999) {
    CurrentScores[CurrentPlayer] -= 999999;
  }

  if (scoreAtTop!=CurrentScores[CurrentPlayer]) {

    if (!TournamentScoring) {
      for (int awardCount=0; awardCount<3; awardCount++) {
        if (AwardScores[awardCount]!=0 && scoreAtTop<AwardScores[awardCount] && CurrentScores[CurrentPlayer]>=AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay>>awardCount)&0x01)==0x01) {
            AddCredit(false, 1);
            BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
            BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
          } else {
            SamePlayerShootsAgain = true;
            BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
            PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
          }
        }
      }
    }
    
    BSOS_SetDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);
    LastTimeScoreChanged = CurrentTime;
    ResetScoreScroll(CurrentPlayer);
  }
  
  if ((CurrentTime-LastTimeScoreChanged)>5000) {
    if (CurrentScores[CurrentPlayer]>999999) ScrollScore(CurrentScores[CurrentPlayer], CurrentPlayer);
  }


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

  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);

}

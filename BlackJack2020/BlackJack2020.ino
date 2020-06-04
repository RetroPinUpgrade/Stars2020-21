/**************************************************************************
 *     This file is part of BlackJack2020.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    BlackJack2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BlackJack2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#include "BallySternOS.h"
#include "BlackJack2020.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>

#define DEBUG_MESSAGES  0

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
#define MACHINE_STATE_COUNTDOWN_BONUS 90
#define MACHINE_STATE_MATCH_MODE      95
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_GAME_OVER       110

#define MACHINE_STATE_ADJUST_FREEPLAY           -16
#define MACHINE_STATE_ADJUST_BALLSAVE           -17
#define MACHINE_STATE_ADJUST_TILT_WARNING       -18
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -19
#define MACHINE_STATE_ADJUST_MUSIC_LEVEL        -20
#define MACHINE_STATE_ADJUST_REBOOT             -21
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   -22
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      -23
#define MACHINE_STATE_ADJUST_CLEAR_SUITS        -24
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     -25
#define MACHINE_STATE_ADJUST_BALLS_PER_GAME     -26
#define MACHINE_STATE_ADJUST_RANDOMIZE_DECK     -27
#define MACHINE_STATE_ADJUST_HIT_OVER_16        -28
#define MACHINE_STATE_ADJUST_SHOW_DEALER_HITS   -29
#define MACHINE_STATE_ADJUST_PLAYER_LOSES_TIES  -30
#define MACHINE_STATE_ADJUST_ONE_SPECIAL_PER_BALL -31

#define CLUB_BUMPER_INDEX     0
#define SPADE_BUMPER_INDEX    1
#define RED_BUMPER_INDEX      2

#define TILT_WARNING_DEBOUNCE_TIME    1000

#define MAX_DISPLAY_BONUS     69

#define SOUND_EFFECT_ADD_CREDIT           1
#define SOUND_EFFECT_BONUS_ADD            2
#define SOUND_EFFECT_BONUS_SUBTRACT       3
#define SOUND_EFFECT_SKILL_SHOT           4
#define SOUND_EFFECT_TILT_WARNING         5
#define SOUND_EFFECT_OUTLANE_UNLIT        6
#define SOUND_EFFECT_OUTLANE_LIT          7
#define SOUND_EFFECT_INLANE               8
#define SOUND_EFFECT_CHANGE_PLAYER        9
#define SOUND_EFFECT_CHANGE_DEALER        10
#define SOUND_EFFECT_BONUS_ROLLOVER       11
#define SOUND_EFFECT_TOPLANE_LIGHT_PLUS   12
#define SOUND_EFFECT_TOPLANE_LIGHT        13
#define SOUND_EFFECT_TOPLANE_FLASH        14
#define SOUND_EFFECT_BUMPER_10            15
#define SOUND_EFFECT_BUMPER_100           16
#define SOUND_EFFECT_BUMPER_1000          17
#define SOUND_EFFECT_PLAYER_WINS          18
#define SOUND_EFFECT_DEAL_CARD_UP         19
#define SOUND_EFFECT_DEAL_CARD_DOWN       20
#define SOUND_EFFECT_RANDOM_SAUCER        21
#define SOUND_EFFECT_SPINNER_BONUS        22
#define SOUND_EFFECT_ADD_PLAYER           30
#define SOUND_EFFECT_BALL_OVER            35
#define SOUND_EFFECT_GAME_OVER            36
#define SOUND_EFFECT_MACHINE_START        37
#define SOUND_EFFECT_EXTRA_BALL           38
#define SOUND_EFFECT_MATCH_SPIN           50
#define SOUND_EFFECT_BONUS_COLLECT_HURRY_UP 51
#define SOUND_EFFECT_BONUS_COUNTDOWN_BASE 100
#define SOUND_EFFECT_BONUS_COUNTDOWN_1K   101
#define SOUND_EFFECT_BONUS_COUNTDOWN_2K   102
#define SOUND_EFFECT_BONUS_COUNTDOWN_3K   103
#define SOUND_EFFECT_BONUS_COUNTDOWN_5K   105


#define EEPROM_BALL_SAVE_BYTE       100
#define EEPROM_FREE_PLAY_BYTE       101
#define EEPROM_MUSIC_LEVEL_BYTE     102
#define EEPROM_SKILL_SHOT_BYTE      103
#define EEPROM_TILT_WARNING_BYTE    104
#define EEPROM_AWARD_OVERRIDE_BYTE  105
#define EEPROM_BALLS_OVERRIDE_BYTE  106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_EXTRA_BALL_SCORE_BYTE    108
#define EEPROM_SPECIAL_SCORE_BYTE       112
#define EEPROM_CLEAR_SUITS_BYTE         116
#define EEPROM_RANDOMIZE_DECK_BYTE      117
#define EEPROM_HIT_OVER_16_BYTE         118
#define EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE   119
#define EEPROM_PLAYER_LOSES_TIES_BYTE         120
#define EEPROM_ONE_SPECIAL_PER_BALL_BYTE      121




// Game/machine global variables
unsigned long HighScore = 0;
unsigned long AwardScores[3];
byte AwardScoresOverride = 99;
int Credits = 0;
int MaximumCredits = 20;
boolean FreePlayMode = false;
byte CreditsPerCoin1, CreditsPerCoin2, CreditsPerCoin3;

// Game mechanics
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long LastTimeSaucerEmpty = 0;
unsigned long BallFirstSwitchHitTime = 0;
unsigned long TimeToWaitForBall = 100;
unsigned long ShowingSurrenderSpins = 0;
unsigned long ShowBetUntilThisTime = 0;
boolean UsingTreeToShowBet = false;
boolean BallSaveUsed = false;
byte BallSaveNumSeconds = 0;
byte BallsPerGame = 3;
boolean MatchFeature = true;

byte NumTiltWarnings = 0;
unsigned long LastTiltWarningTime = 0;

// Game play parameters (stored)
byte MaxTiltWarnings = 2;
byte MusicLevel = 2;
byte RandomizeDeck = 0;
boolean NoHitsOver16 = true;
boolean ResetSuitsPerBall = false;
boolean HighScoreReplay = true;
boolean PlayerLosesOnTies = false;
byte NoveltyScoring = 0;
boolean CreditDisplay = true;
byte NumberOfDealerHitsToShow = 3;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
boolean OneSpecialPerBall = true;
byte NumSecondsForBonusCollect = 45;

// Game state
byte SuitsComplete[4];
byte PlayersTopCard;
byte DealersTopCard;
byte PlayersTally;
byte DealersTally;
boolean PlayerHasAce = false;
boolean DealerHasAce = false;
byte Deck[26] = {0x4D, 0xD7, 0xBC, 0x6C, 0x6A, 0x53, 0xC7, 0x15, 0x72, 0x43, 0xB1, 0x29, 0x18, 0xAA, 0x26, 0xB5, 0x3D, 0x64, 0x7A, 0xD4, 0x38, 0x92, 0x9C, 0x88, 0x59, 0x1B};
byte PlayersDeckPtr[4];
byte ShowDealersCardCountdown = 3;
boolean BallSaveHurryUp = false;
boolean PlayerHits = false;
byte SurrenderSpins = 0;
byte RegularSpins = 0;
boolean Spinner1kLit = false;
unsigned long CurrentScores[4];
byte Bonus;
byte BonusX;
boolean SamePlayerShootsAgain = false;
boolean LeftOutlaneLit = false;
boolean RightOutlaneLit = false;
boolean SpecialCollectedThisBall = false;

#define BETTING_STAGE_BUILDING_STAKES   0
#define BETTING_STAGE_BET_SWEEP         1
#define BETTING_STAGE_DEAL              2
#define BETTING_STAGE_SHOW_DEAL         3
#define BETTING_STAGE_NATURAL_21        4
#define BETTING_STAGE_PLAYER_BUST       5
#define BETTING_STAGE_WAIT_FOR_COLLECT  6
#define BETTING_STAGE_END_OF_ROUND      7
#define BETTING_STAGE_WAIT_FOR_BONUS_COLLECT  8
#define BETTING_STAGE_BONUS_COLLECT           9

byte BettingStage = BETTING_STAGE_BUILDING_STAKES;

byte dipBank0, dipBank1, dipBank2, dipBank3;

void GetDIPSwitches() {
  dipBank0 = BSOS_GetDipSwitches(0);
  dipBank1 = BSOS_GetDipSwitches(1);
  dipBank2 = BSOS_GetDipSwitches(2);
  dipBank3 = BSOS_GetDipSwitches(3);
}

void DecodeDIPSwitchParameters() {

  CreditsPerCoin1 = ((dipBank0)&0x1F);
  HighScoreReplay = ((dipBank0&0x60)>>5)?true:false;
  MusicLevel = (dipBank0&0x80)?2:0;
  
  CreditsPerCoin3 = ((dipBank1)&0x1F);
  NoveltyScoring = ((dipBank1&60)>>5);
  BallsPerGame = (dipBank1&80)?5:3;
  
  MaximumCredits = (dipBank2&0x07)*5 + 5;
  CreditDisplay = (dipBank2&0x08)?true:false;
  MatchFeature = (dipBank2&0x10)?true:false;

  CreditsPerCoin2 = ((dipBank3)&0x0F);
  ResetSuitsPerBall = ((dipBank3)&0x60)?false:true;
  PlayerLosesOnTies = ((dipBank3)&0x80)?true:false;
  
}

void GetStoredParameters() {

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride!=99) {
    BallsPerGame = ballsOverride;
  }
  
  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;
    
  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 16);
  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 2);
  ResetSuitsPerBall = ReadSetting(EEPROM_CLEAR_SUITS_BYTE, false);
  RandomizeDeck = ReadSetting(EEPROM_RANDOMIZE_DECK_BYTE, 2);
  NoHitsOver16 = ReadSetting(EEPROM_HIT_OVER_16_BYTE, true);
  NumberOfDealerHitsToShow = ReadSetting(EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE, 2);
  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  OneSpecialPerBall = ReadSetting(EEPROM_ONE_SPECIAL_PER_BALL_BYTE, false);

  byte noveltyOverride = ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 99);
  if (noveltyOverride!=99) NoveltyScoring = noveltyOverride;

  byte playerTiesOverride = ReadSetting(EEPROM_PLAYER_LOSES_TIES_BYTE, false);
  if (playerTiesOverride!=99) PlayerLosesOnTies = playerTiesOverride;

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);
  AwardScoresOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);

  HighScore = BSOS_ReadHighScoreFromEEProm();
  Credits = BSOS_ReadCreditsFromEEProm();
  if (Credits>MaximumCredits) Credits = MaximumCredits;

}


byte ReadSetting(byte setting, byte defaultValue) {
    byte value = EEPROM.read(setting);
    if (value == 0xFF) {
        EEPROM.write(setting, defaultValue);
        return defaultValue;
    }
    return value;
}

void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(115200);
  }

  // Start out with everything tri-state, in case the original
  // CPU is running
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  unsigned long startTime = millis();
  unsigned long numberOfClockTicks = 0;
  boolean sawHigh = false;
  boolean sawLow = false;
  byte lastClockReading = digitalRead(4);
  byte curClockReading;
  // for three seconds, look for activity on the VMA line (A5)
  // If we see anything, then the MPU is active so we shouldn't run
  while ((millis()-startTime)<1000) {
    if (digitalRead(A5)) sawHigh = true;
    else sawLow = true;
    curClockReading = digitalRead(4);
    if (curClockReading==1 && lastClockReading==0) numberOfClockTicks += 1;
    lastClockReading = curClockReading;
  }
  // If we saw both a high and low signal, then someone is toggling the 
  // VMA line, so we should hang here forever (until reset)
  if (sawHigh && sawLow) {
    while (1);
  }
    
  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

  if (DEBUG_MESSAGES) {
    Serial.write("Attempting to initialize the MPU\n");
  }
 
  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Use dip switches to set up game variables
  GetDIPSwitches();
  DecodeDIPSwitchParameters();

  // Read parameters from EEProm
  GetStoredParameters();
  
  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);  
  BSOS_PushToSolenoidStack(SOL_SAUCER, 5, true);
}


void PlaySoundEffect(byte soundEffectNum) {
  if (MusicLevel==0) return;

  int count;

  switch (soundEffectNum) {
    case SOUND_EFFECT_ADD_PLAYER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime, true);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+400, true);
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
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+500, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+666, true);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+750, true);
      }
    break;
    case SOUND_EFFECT_MACHINE_START:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 1, CurrentTime+500, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+600, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+900, true);
    break;
    case SOUND_EFFECT_ADD_CREDIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+75, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+150, true);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+225, true);
    break;
    case SOUND_EFFECT_PLAYER_WINS:
      for (count=0; count<(MusicLevel*2+1); count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+count*100);
      }
    break;
    case SOUND_EFFECT_DEAL_CARD_UP:
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 1, CurrentTime);
    break;
    case SOUND_EFFECT_DEAL_CARD_DOWN:
      if (MusicLevel>1) BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 1, CurrentTime);
    break;
    case SOUND_EFFECT_MATCH_SPIN:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BUMPER_10:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BUMPER_100:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COLLECT_HURRY_UP:
    case SOUND_EFFECT_SPINNER_BONUS:
    case SOUND_EFFECT_BUMPER_1000:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_ADD:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_SUBTRACT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
    break;
    case SOUND_EFFECT_OUTLANE_UNLIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
    break;
    case SOUND_EFFECT_OUTLANE_LIT:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+200);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_CHANGE_PLAYER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_BONUS_ROLLOVER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+250);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
      }
    break;
    case SOUND_EFFECT_CHANGE_DEALER:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_INLANE:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_1K:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_2K:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_3K:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_5K:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
    break;
    case SOUND_EFFECT_EXTRA_BALL:
    case SOUND_EFFECT_SKILL_SHOT:
      for (count=0; count<2; count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime + count*150);
        if (MusicLevel>1) {
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+25 + count*150);
          BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+50 + count*150);
        }
      }
    break;
    case SOUND_EFFECT_RANDOM_SAUCER:
    case SOUND_EFFECT_TOPLANE_LIGHT:
      for (count=0; count<(MusicLevel); count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+count*200);
      }
    break;
    case SOUND_EFFECT_TOPLANE_LIGHT_PLUS:
      for (count=0; count<(MusicLevel+1); count++) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+count*200);
      }
    break;
    case SOUND_EFFECT_TOPLANE_FLASH:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+175);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+350);
    break;
    case SOUND_EFFECT_TILT_WARNING:
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      if (MusicLevel>1) {
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        BSOS_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+300);
      }
    break;
  }    
}

void PulseTopLights(byte topLightPulse, byte suitsComplete) {
  byte shiftNum = 1;
  for (byte count=0; count<4; count++) {
    SetTopLampState(CLUBS_TOP_LANE-count, (topLightPulse/4)==count, topLightPulse%4, suitsComplete&shiftNum);
    shiftNum *= 2;
  }
}

void AddToBonus(byte bonusAddition) {
  Bonus += bonusAddition;
  if (Bonus>MAX_DISPLAY_BONUS) Bonus = MAX_DISPLAY_BONUS;
}


void SetPlayerLamps(byte numPlayers, int flashPeriod=0) {
  for (byte count=PLAYER_1_UP; count<(PLAYER_4_UP+1); count++) BSOS_SetLampState(count, (numPlayers==(1+count-PLAYER_1_UP))?1:0, 0, flashPeriod);
}


void ShowSuitsComplete(byte suitsComplete) {
  byte shiftNum = 8;
  for (byte count=0; count<4; count++) {
    BSOS_SetLampState(HEARTS_TOP_LANE+count, (suitsComplete&(shiftNum))?1:0, 0, 400*(int)(((suitsComplete/16)&shiftNum)?1:0));
    shiftNum /= 2;
  }

  BSOS_SetLampState(CLUB_BUMPER, (suitsComplete&0x01)?1:0, 0, 400 * (int)(((suitsComplete/16)&0x01)?1:0));
  BSOS_SetLampState(RED_BUMPER, ((suitsComplete&0x0A)==0x0A)?1:0, 0, 400 * (int)((((suitsComplete/16)&0x0A)==0x0A)?1:0));
  BSOS_SetLampState(SPADE_BUMPER, (suitsComplete&0x04)?1:0, 0, 400 * (int)(((suitsComplete/16)&0x04)?1:0));  

  if ((suitsComplete&0x03)==0x03) LeftOutlaneLit = true;
  else LeftOutlaneLit = false;
  if ((suitsComplete&0x06)==0x06) RightOutlaneLit = true;
  else RightOutlaneLit = false;
  BSOS_SetLampState(LEFT_OUTLANE, LeftOutlaneLit);
  BSOS_SetLampState(RIGHT_OUTLANE, RightOutlaneLit);

  if ((suitsComplete&0x0F)==0x0F) Spinner1kLit = true;
  else Spinner1kLit = false;
  BSOS_SetLampState(SPINNER_1000, Spinner1kLit);
}


void ShowBetOnTree(byte betAmount) {

  if (betAmount>=20) {
    BSOS_SetLampState(BONUS_TREE_20, 1, 1);
    betAmount -= 20;
  }
  else BSOS_SetLampState(BONUS_TREE_20, 0);

  if (betAmount>=10) {
    BSOS_SetLampState(BONUS_TREE_10, 1, 1);
    betAmount -= 10;
  }
  else BSOS_SetLampState(BONUS_TREE_10, 0);
  
  for (byte count=0; count<9; count++) {
    if ((count+1)<=betAmount) BSOS_SetLampState(BONUS_TREE_1+count, 1, 1);
    else BSOS_SetLampState(BONUS_TREE_1+count, 0);
  }


}

void ShowPlayerWinsPulse(boolean turnOff=false) {

  if (turnOff) {
    BSOS_SetLampState(PLAYER_WINS, 0);
  } else {
    int period;
    if (PlayersTally<17) period = 1500;
    if (PlayersTally==17) period = 1000;
    if (PlayersTally==18) period = 500;
    if (PlayersTally==19) period = 200;
    if (PlayersTally==20) period = 100;
    if (PlayersTally==21) period = 50;
    if (DealersTopCard==10) period += 500;
    
    byte phase = (CurrentTime/period)%4;
    BSOS_SetLampState(PLAYER_WINS, (phase!=0), phase%2);
  }
}


void ShowBonusOnTree(byte bonus, byte dim) {
  if (bonus>MAX_DISPLAY_BONUS) bonus = MAX_DISPLAY_BONUS;

  if (bonus>=60) {
    BSOS_SetLampState(BONUS_TREE_10, 1, dim, 250);
    bonus -= 20;
  } else if ( ((bonus/10)%2)==1 ) {
    BSOS_SetLampState(BONUS_TREE_10, 1, dim);
    bonus -= 10;
  } else {
    BSOS_SetLampState(BONUS_TREE_10, 0, dim, 250);
  }

  if (bonus>=40) {
    BSOS_SetLampState(BONUS_TREE_20, 1, dim, 250);
    bonus -= 40;
  } else if (bonus>=20) {
    BSOS_SetLampState(BONUS_TREE_20, 1, dim);
    bonus -= 20;   
  } else {
    BSOS_SetLampState(BONUS_TREE_20, 0, dim);
  }
 
  for (byte count=0; count<9; count++) {
    if (count==(bonus-1)) BSOS_SetLampState(BONUS_TREE_1+count, 1, dim);
    else BSOS_SetLampState(BONUS_TREE_1+count, 0, dim);
  }

}


unsigned long TimeToRevertDisplays = 0;
byte DisplayOverridden = 0;
unsigned long ScrollStartTime = 0;
void ShowPlayerDisplays(boolean pauseScrolls=false, boolean showHighScores=false, byte numDisplaysToShow=0) {
  int count;

  if (pauseScrolls) ScrollStartTime = CurrentTime + 5000;

  if (TimeToRevertDisplays!=0 && CurrentTime>TimeToRevertDisplays) {
    DisplayOverridden = 0;
  }

  if (numDisplaysToShow==0) numDisplaysToShow = CurrentNumPlayers;

  // Loop on player displays
  for (count=0; count<4; count++) {

    // If this display is currently overriden, don't change it here
    if ((DisplayOverridden>>count) & 0x01) continue;

    unsigned long scoreToShow = CurrentScores[count];
    if (showHighScores) scoreToShow = HighScore;
    
    if (!showHighScores && count==CurrentPlayer) {
      // For current player, flash the score if nothing has been hit
      if (BallFirstSwitchHitTime==0) {
        BSOS_SetDisplay(count, scoreToShow);
        BSOS_SetDisplayFlash(count, CurrentTime, 500, (scoreToShow<10)?99:scoreToShow);
      } else {
        BSOS_SetDisplay(count, scoreToShow);
        BSOS_SetDisplayBlankByMagnitude(count, scoreToShow);
      }
    } else {
      // For non-current player, only show display if it's in use
      if (count<numDisplaysToShow) {
        BSOS_SetDisplay(count, scoreToShow);
        BSOS_SetDisplayBlankByMagnitude(count, scoreToShow);        
      } else {
        BSOS_SetDisplayBlank(count, 0x00);        
      }
    }
  }
}

void OverridePlayerDisplays(byte displayNum, unsigned long overrideUntilTime = 0 /* zero = forever */) {
  DisplayOverridden |= 1<<displayNum;
  TimeToRevertDisplays = overrideUntilTime;
}

void ClearOverridePlayerDisplays() {
  DisplayOverridden = 0;
}


void AddCredit() {
  if (Credits<MaximumCredits) {
    Credits += 1;
    BSOS_WriteCreditsToEEProm(Credits);
    PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);    
  } else {
  }

  BSOS_SetDisplayCredits(Credits, CreditDisplay);
  BSOS_SetCoinLockout((Credits<MaximumCredits)?false:true);
}


boolean AddPlayer(boolean resetNumPlayers=false) {

  if (Credits<1 && !FreePlayMode) return false;
  if (CurrentNumPlayers>=4) return false;

  if (resetNumPlayers) CurrentNumPlayers = 0;
  CurrentNumPlayers += 1;
//  BSOS_SetDisplay(CurrentNumPlayers-1, 0);
//  BSOS_SetDisplayBlank(CurrentNumPlayers-1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteCreditsToEEProm(Credits);
  }

  BSOS_SetDisplayCredits(Credits, CreditDisplay);
  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER);

  return true;
}


void ShuffleDeck() {
  for (byte deckPtr=0; deckPtr<52; deckPtr++) {
    byte cardByte1 = Deck[deckPtr/2];
    byte randomPtr = (micros() + random(0, 100))%52;
    byte cardByte2 = Deck[randomPtr/2];
    byte card1, card2;

    if (deckPtr%2) card1 = cardByte1/16;
    else card1 = cardByte1 & 0x0F;
    if (randomPtr%2) card2 = cardByte2/16;
    else card2 = cardByte2 & 0x0F;
    
    if (deckPtr%2) cardByte1 = (cardByte1 & 0x0F) | (card2 * 16);
    else cardByte1 = (cardByte1 & 0xF0) | card2;
    if (randomPtr%2) cardByte2 = (cardByte2 & 0x0F) | (card1 * 16);
    else cardByte2 = (cardByte2 & 0xF0) | card1;

    Deck[deckPtr/2] = cardByte1;
    Deck[randomPtr/2] = cardByte2;
  }
}


int InitGamePlay(boolean curStateChanged) {
  int returnState = MACHINE_STATE_INIT_GAMEPLAY;

  returnState = ClearSwitchBuffer(returnState, false);

  if (curStateChanged) {
    CurrentTime = millis();

    BSOS_SetCoinLockout((Credits>=MaximumCredits)?true:false);
    BSOS_SetDisableFlippers(true);
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);
    BSOS_SetDisplayBallInPlay(1);

    // Set up general game variables
    CurrentPlayer = 0;
    CurrentNumPlayers = 1;
    CurrentBallInPlay = 1;
    SamePlayerShootsAgain = false;
    if (RandomizeDeck>1) ShuffleDeck();
    for (int count=0; count<4; count++) {
      CurrentScores[count] = 0;
      SuitsComplete[count] = 0;
      PlayersDeckPtr[count] = 0;

    }

    // if the ball is in the outhole, then we can move on
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_EnableSolenoidStack();
      BSOS_SetDisableFlippers(false);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      // Otherwise, let's see if it's in a spot where it could get trapped,
      // for instance, a saucer (if the game has one)
      if (BSOS_ReadSingleSwitchState(SW_SAUCER)) {
        BSOS_PushToSolenoidStack(SOL_SAUCER, 5, true);
        // if it was in the saucer - kick it out and wait five seconds
        TimeToWaitForBall = CurrentTime + 8000;
      } else {
        // if it wasn't in the saucer, just wait 100ms
        TimeToWaitForBall = CurrentTime + 100;
      }

    }
  }

  // Wait for TimeToWaitForBall seconds, or until the ball appears
  // The reason to bail out after TIME_TO_WAIT_FOR_BALL is just
  // in case the ball is already in the shooter lane.
  if (CurrentTime>TimeToWaitForBall || BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    BSOS_EnableSolenoidStack();
    BSOS_SetDisableFlippers(false);
    returnState = MACHINE_STATE_INIT_NEW_BALL;
  }
  
  return returnState;  
}




int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {  

  if (curStateChanged) {
    BallFirstSwitchHitTime = 0;

    // if we came here as an extra ball, set the same player lights
    if (SamePlayerShootsAgain) {
      BSOS_SetLampState(SAME_PLAYER, 1);
      BSOS_SetLampState(HEAD_SAME_PLAYER, 1);
    }
    SamePlayerShootsAgain = false;
    BallSaveHurryUp = false;
    BallSaveUsed = false;
    NumTiltWarnings = 0;
    LastTiltWarningTime = CurrentTime;
    SpecialCollectedThisBall = false;
    
    BSOS_SetDisableFlippers(false);
    BSOS_EnableSolenoidStack(); 
    BSOS_SetDisplayCredits(Credits, CreditDisplay);
    SetPlayerLamps(playerNum+1, 500);
    
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
      // Maybe just give it half a second to eject the ball?
    }

    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(BALL_IN_PLAY, 1);
    BSOS_SetLampState(TILT, 0);

    if (BallSaveNumSeconds>0) {
      BSOS_SetLampState(SAME_PLAYER, 1, 0, 500);
//      BSOS_SetLampState(HEAD_SAME_PLAYER, 1, 0, 500);
    }

    if (ResetSuitsPerBall) SuitsComplete[playerNum] = 0;
    
    BettingStage = BETTING_STAGE_BUILDING_STAKES;
    Bonus = 0;
    BonusX = 1;

    PlayersTopCard = 0;
    PlayersTally = 0;
    DealersTopCard = 0;
    DealersTally = 0;
    PlayerHasAce = false;
    DealerHasAce = false;
    ShowSuitsComplete(SuitsComplete[playerNum]);
  }
  
  // We should only consider the ball initialized when 
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }
  
}


#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;

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

      if (curState==MACHINE_STATE_ADJUST_FREEPLAY) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = (byte *)&FreePlayMode;
        CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_BALLSAVE) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 5;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 5;
        AdjustmentValues[2] = 10;
        AdjustmentValues[3] = 15;
        AdjustmentValues[4] = 20;
        CurrentAdjustmentByte = &BallSaveNumSeconds;
        CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_TILT_WARNING) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 2;
        CurrentAdjustmentByte = &MaxTiltWarnings;
        CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_TOURNAMENT_SCORING) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = &NoveltyScoring;
        CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_MUSIC_LEVEL) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 2;
        CurrentAdjustmentByte = &MusicLevel;
        CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_REBOOT) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        for (byte count=0; count<4; count++) {
          BSOS_SetDisplay(count, 8007);
          BSOS_SetDisplayBlankByMagnitude(count, 8007);
        }
        CurrentAdjustmentByte = 0;
      } else if (curState==MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD) {
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &ExtraBallValue;
        CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_SPECIAL_AWARD) {
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &SpecialValue;
        CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_CLEAR_SUITS) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = (byte *)&ResetSuitsPerBall;
        CurrentAdjustmentStorageByte = EEPROM_CLEAR_SUITS_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_AWARD_OVERRIDE) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 7;
        CurrentAdjustmentByte = &AwardScoresOverride;
        CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_BALLS_PER_GAME) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 3;
        AdjustmentValues[0] = 3;
        AdjustmentValues[1] = 5;
        AdjustmentValues[1] = 99;
        CurrentAdjustmentByte = &BallsPerGame;
        CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_RANDOMIZE_DECK) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 3;
        CurrentAdjustmentByte = &RandomizeDeck;
        CurrentAdjustmentStorageByte = EEPROM_RANDOMIZE_DECK_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_HIT_OVER_16) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = (byte *)&NoHitsOver16;
        CurrentAdjustmentStorageByte = EEPROM_HIT_OVER_16_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_SHOW_DEALER_HITS) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 7;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        AdjustmentValues[2] = 2;
        AdjustmentValues[3] = 3;
        AdjustmentValues[4] = 4;
        AdjustmentValues[5] = 5;
        AdjustmentValues[6] = 99;
        CurrentAdjustmentByte = &NumberOfDealerHitsToShow;
        CurrentAdjustmentStorageByte = EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_PLAYER_LOSES_TIES) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = (byte *)&PlayerLosesOnTies;
        CurrentAdjustmentStorageByte = EEPROM_PLAYER_LOSES_TIES_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_ONE_SPECIAL_PER_BALL) {
        AdjustmentType = ADJ_TYPE_MIN_MAX;
        AdjustmentValues[0] = 0;
        AdjustmentValues[1] = 1;
        CurrentAdjustmentByte = (byte *)&OneSpecialPerBall;
        CurrentAdjustmentStorageByte = EEPROM_ONE_SPECIAL_PER_BALL_BYTE;
      } else {
        returnState = MACHINE_STATE_ATTRACT;     
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
      } else if (CurrentAdjustmentUL && AdjustmentType==ADJ_TYPE_SCORE_WITH_DEFAULT) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal>100000) curVal = 0;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }
      
      if (curState==MACHINE_STATE_ADJUST_REBOOT) {
        // If any variables have been set to non-override (99), return 
        // them to dip switch settings
        // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
        DecodeDIPSwitchParameters();
        GetStoredParameters();
        returnState = MACHINE_STATE_ATTRACT;     
      }
    }

    // Show current value
    if (CurrentAdjustmentByte!=NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte));        
      BSOS_SetDisplayBlankByMagnitude(0, (unsigned long)(*CurrentAdjustmentByte));        
    } else if (CurrentAdjustmentUL!=NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL));        
      BSOS_SetDisplayBlankByMagnitude(0, (*CurrentAdjustmentUL)); 
    }

  }

  return returnState;
}





void SetTopLampState(byte lampNum, boolean lampSelected, byte lampPhase, boolean lampBaseLevelOn) {
  byte lampIsOn = (lampSelected || lampBaseLevelOn) ? 1 : 0;
  byte lampIsDim = 1;
  if ( lampSelected && (lampPhase==1 || lampPhase==2) ) lampIsDim = 0;
  BSOS_SetLampState(lampNum, lampIsOn, lampIsDim);
}

boolean PlayerUpLightBlinking = false;
byte CurrentSkillShotSuit = 0;
unsigned long maxSweepBet = 0;
unsigned long BettingModeStart = 0;
byte CurrentSweepingBet = 0;
unsigned long LastTimeInfoUpdated = 0;



int CountdownBonus(boolean curStateChanged) {
  int returnState = MACHINE_STATE_COUNTDOWN_BONUS;

  if (curStateChanged) {
    // Turn off displays & lights we don't want for bonus countdown
    ClearOverridePlayerDisplays();
    ShowPlayerWinsPulse(true);
    SetBonusXLights(BonusX);
    LastTimeInfoUpdated = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (Bonus>1) {
      unsigned long maxBonusX = BonusX;
      if (maxBonusX>5) maxBonusX = 5;
      CurrentScores[CurrentPlayer] += 1000 * maxBonusX;
      Bonus -= 1;
      PlaySoundEffect(SOUND_EFFECT_BONUS_COUNTDOWN_BASE + maxBonusX);
    } else {
      Bonus = 0;
      returnState = MACHINE_STATE_BALL_OVER;
    }
    ShowBonusOnTree(Bonus, 0);
    LastTimeInfoUpdated = CurrentTime;    
  }

  return returnState;
}


void CalculateAndShowBetSweep(byte minBet, byte maxBet) {
  unsigned long gap = (unsigned long)(maxBet-minBet);
  unsigned long timeRunning = ((CurrentTime-BettingModeStart)/1000)%10;
  if (gap!=0) {
    if (timeRunning<5) {
      // bet is climbing up
      gap = (timeRunning*gap)/4;
    } else {
      // bet is ramping down
      timeRunning = 9 - timeRunning;
      gap = (timeRunning*gap)/4;
    }    
  }
  CurrentSweepingBet = minBet + ((byte)gap);
  if (CurrentSweepingBet>20) CurrentSweepingBet = 20;

  // Update the sweep 2x a second
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>500) {
    LastTimeInfoUpdated = CurrentTime;
    ShowBetOnTree(CurrentSweepingBet);
    UsingTreeToShowBet = true;
    ShowBetUntilThisTime = CurrentTime+2000;
  }
}


void SetTallyLamps(byte tally, boolean withAce, byte topLamp) {
  if (tally==0) {
    BSOS_SetLampState(topLamp+4, 0);
    BSOS_SetLampState(topLamp+3, 0);
    BSOS_SetLampState(topLamp+2, 0);
    BSOS_SetLampState(topLamp+1, 0);
    BSOS_SetLampState(topLamp, 0);
  } else if (tally>21) {
    BSOS_SetLampState(topLamp+4, 0);
    BSOS_SetLampState(topLamp+3, 0);
    BSOS_SetLampState(topLamp+2, 0);
    BSOS_SetLampState(topLamp+1, 0);
    BSOS_SetLampState(topLamp, 1, 0, 250);
  } else if (tally>=17) {
    BSOS_SetLampState(topLamp+4, (tally==17));
    BSOS_SetLampState(topLamp+3, (tally==18));
    BSOS_SetLampState(topLamp+2, (tally==19));
    BSOS_SetLampState(topLamp+1, (tally==20));
    BSOS_SetLampState(topLamp, (tally==21));
  } else {
    if (!withAce || tally<7 || tally>11) {
      BSOS_SetLampState(topLamp+4, 1, 0, 250);
      BSOS_SetLampState(topLamp+3, 0);
      BSOS_SetLampState(topLamp+2, 0);
      BSOS_SetLampState(topLamp+1, 0);
      BSOS_SetLampState(topLamp, 0);
    } else {
      BSOS_SetLampState(topLamp+4, (tally==7), 0, 750);
      BSOS_SetLampState(topLamp+3, (tally==8), 0, 750);
      BSOS_SetLampState(topLamp+2, (tally==9), 0, 750);
      BSOS_SetLampState(topLamp+1, (tally==10), 0, 750);
      BSOS_SetLampState(topLamp, (tally==11), 0, 750);
    }
  }
}

void SlideCards(unsigned long slideStart, byte displayNum, byte top, byte tally, boolean hasAce) {
  unsigned long digitMultiplier = 10000;

  if ((CurrentTime-slideStart)<1000) {
    byte blank = 0x03;
    for (unsigned long count=0; count<((CurrentTime-slideStart)/250); count++) {
      digitMultiplier /= 10;
      blank = blank << 1;
    }
    blank |= 0x30;
    unsigned long topDigit = top;
    unsigned long bottomDigit = tally;
    if (digitMultiplier==10) topDigit -= (topDigit%10);

    bottomDigit += topDigit*digitMultiplier;

    OverridePlayerDisplays(displayNum);
    BSOS_SetDisplay(displayNum, bottomDigit);
    BSOS_SetDisplayBlank(displayNum, blank); 
  } else {
    unsigned long newValue = top + tally;
    if (hasAce) {
      if ( ((CurrentTime-slideStart)/500)%2 && newValue<11) newValue += 10;
      OverridePlayerDisplays(displayNum);
      BSOS_SetDisplay(displayNum, newValue);
      BSOS_SetDisplayBlank(displayNum, 0x30); 
    } else {
      OverridePlayerDisplays(displayNum);
      BSOS_SetDisplay(displayNum, newValue);
      BSOS_SetDisplayBlank(displayNum, 0x30); 
    }
  }
  
}

byte cardShown = 0;
boolean Natural21;

boolean ShowCards() {
  boolean allCardsShown = false;
  byte displayForPlayer=0, displayForDealer=2;
  if ((CurrentPlayer%2)==0) {
    displayForPlayer = 1;
    displayForDealer = 3;
  }
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>100) {
    if ((CurrentTime-BettingModeStart)<800) {
      OverridePlayerDisplays(displayForPlayer);
      OverridePlayerDisplays(displayForDealer);

      BSOS_SetDisplay(displayForPlayer, ((unsigned long)PlayersTopCard)*10000 + (unsigned long)PlayersTally);
      if (NumberOfDealerHitsToShow) {
        BSOS_SetDisplay(displayForDealer, ((unsigned long)DealersTopCard)*10000 + (unsigned long)88);
      } else {
        BSOS_SetDisplay(displayForDealer, ((unsigned long)DealersTopCard)*10000 + (unsigned long)DealersTally);
      }
      BSOS_SetDisplayBlank(displayForPlayer, 0x00);
      BSOS_SetDisplayBlank(displayForDealer, 0x00);
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      cardShown = 0;
    } else if ((CurrentTime-BettingModeStart)<1400) {
      if (cardShown==0) {
        // Show player's first card
        BSOS_SetDisplayBlank(displayForPlayer, 0x03);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 1;
      }
    } else if ((CurrentTime-BettingModeStart)<2000) {
      if (cardShown==1) {
        // Show dealer's first card
        BSOS_SetDisplayBlank(displayForDealer, 0x03);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 2;
      }
    } else if ((CurrentTime-BettingModeStart)<2600) {
      if (cardShown==2) {
        // Show player's second card
        BSOS_SetDisplayBlank(displayForPlayer, 0x33);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 3;
      }
    } else if ((CurrentTime-BettingModeStart)<3200) {
      if (cardShown==3) {
        // Show dealer's second card
        BSOS_SetDisplayBlank(displayForDealer, 0x33);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_DOWN);
        cardShown = 4;
      }
    } else if ((CurrentTime-BettingModeStart)<5200) {
      SlideCards(BettingModeStart+3200, displayForPlayer, PlayersTopCard, PlayersTally, PlayerHasAce);
      if ((CurrentTime-BettingModeStart)<4000) {
        byte newTally = PlayersTally+PlayersTopCard;
        SetTallyLamps(newTally, PlayerHasAce, PLAYER_21);
      }
    } else {
      if (PlayersTally==1 || PlayersTopCard==1) PlayerHasAce = true;
      PlayersTally += PlayersTopCard;
      PlayersTopCard = 0;

      // check for immediate win (10 + A)
      if (PlayersTally==11 && PlayerHasAce) {
        Natural21 = true;
      } else {
        Natural21 = false;
      }
      
      SetTallyLamps(PlayersTally, PlayerHasAce, PLAYER_21);
      if (DealersTopCard<6 && DealersTopCard!=0) {
        SetTallyLamps(16, false, DEALER_21);
      }
      allCardsShown = true;
    }
    LastTimeInfoUpdated = CurrentTime;
  }

  return allCardsShown;
}


void SweepSaucerLights(boolean allOff = false) {
  byte SaucerLightIndex = (CurrentTime/100)%4 + 1;
  if (allOff) SaucerLightIndex = 0;

  BSOS_SetLampState(B_2XFEATURE_BONUS, (SaucerLightIndex==1)?1:0);
  BSOS_SetLampState(SPECIAL, (SaucerLightIndex==1)?1:0);
  BSOS_SetLampState(B_2X_3XFEATURE, (SaucerLightIndex==2)?1:0);
  BSOS_SetLampState(B_3X_5XFEATURE, (SaucerLightIndex==3)?1:0);
  BSOS_SetLampState(B_5X_BONUS, (SaucerLightIndex==4)?1:0);
  BSOS_SetLampState(EXTRA_BALL, (SaucerLightIndex==4)?1:0);
  
}

void SetBonusXLights(byte BonusX) {
  BSOS_SetLampState(B_2XFEATURE_BONUS, (BonusX==1)?1:0);
  BSOS_SetLampState(B_2X_3XFEATURE, (BonusX==2)?1:0);
  BSOS_SetLampState(B_3X_5XFEATURE, (BonusX==3)?1:0);
  BSOS_SetLampState(B_5X_BONUS, (BonusX>=5)?1:0);
  BSOS_SetLampState(EXTRA_BALL, (BonusX==5)?1:0);
  BSOS_SetLampState(SPECIAL, (BonusX>=6)?1:0, 0, 150 * ((BonusX>6)?1:0));
}


void SweepSpinnerLights(boolean allOff = false) {
  byte id = (CurrentTime/100)%10 + 1;
  if (allOff) id = 0;

  for (int count=0; count<9; count++) {
    BSOS_SetLampState(BONUS_1+count, (id==(count+1)||id==(count+2)), (id==(count+2)));
  }
  BSOS_SetLampState(BONUS_1+9, (id==(10)||id==1), (id==1));
  
}

void ShowSingleSpinnerLight(byte lightNum) {
  for (int count=0; count<10; count++) {
    BSOS_SetLampState(BONUS_1+count, (count==lightNum)?1:0);
  }
}

void ShowSurrenderLights(byte numLights) {
  for (int count=0; count<10; count++) {
    BSOS_SetLampState(BONUS_1+count, (count<(10-numLights))?1:0, 1);
  }
}

boolean ShowPlayerHit() {
  boolean slideDone = false;
  byte displayForPlayer=0;
  if ((CurrentPlayer%2)==0) {
    displayForPlayer = 1;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (BettingModeStart!=0 && (CurrentTime-BettingModeStart)<2000) {
      SlideCards(BettingModeStart, displayForPlayer, PlayersTopCard, PlayersTally, PlayerHasAce);
    } else {
      if (PlayersTopCard==1) PlayerHasAce = true;
      PlayersTally += PlayersTopCard;
      PlayersTopCard = 0;
      SetTallyLamps(PlayersTally, PlayerHasAce, PLAYER_21);      
      slideDone = true;
    }
    LastTimeInfoUpdated = CurrentTime;
  }

  return slideDone;
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
      BSOS_SetDisplayCredits(Credits, CreditDisplay);
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




byte GetNextCard() {

  byte playerNum;

  if (RandomizeDeck%2) playerNum = 0;
  else playerNum = CurrentPlayer;

  byte deckPtr = PlayersDeckPtr[playerNum];
  byte nextCard;
  byte cardByte = Deck[deckPtr/2];
  if ((deckPtr%2)==0) nextCard = cardByte&0x0F;
  else nextCard = cardByte/16;  
  
  if (nextCard>10) nextCard = 10;

  PlayersDeckPtr[playerNum]+=1;
  if (PlayersDeckPtr[playerNum]==52) PlayersDeckPtr[playerNum] = 0;
  return nextCard;
}




void ToggleAce() {

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>500) {
    byte displayForPlayer=0;
    if ((CurrentPlayer%2)==0) {
      displayForPlayer = 1;
    }

    byte plus10 = 0;
    if (LastTimeInfoUpdated && PlayersTally<11) plus10 = 10*((CurrentTime/500)%2);

    OverridePlayerDisplays(displayForPlayer);
    BSOS_SetDisplay(displayForPlayer, PlayersTally + plus10);
    BSOS_SetDisplayBlank(displayForPlayer, 0x30);

    LastTimeInfoUpdated = CurrentTime;
  }
}


int ClearSwitchBuffer(int curState, boolean ResetNumPlayers) {
  int returnState = curState;
  byte switchHit;
  while ( (switchHit=BSOS_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
    if (switchHit==SW_CREDIT_RESET) {
      if (AddPlayer(ResetNumPlayers)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      AddCredit();
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>500) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
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
    if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);
    if (Credits>=MaximumCredits) BSOS_SetCoinLockout(true);
    else BSOS_SetCoinLockout(false);
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    for (int count=0; count<4; count++) {
      BSOS_SetDisplayBlank(count, 0x00);     
    }
    BSOS_SetDisplayCredits(Credits, CreditDisplay);
    BSOS_SetDisplayBallInPlay(0);
    AttractLastHeadMode = 255;
    AttractLastPlayfieldMode = 255;
  }

  // Alternate displays between high score and blank
  if ((CurrentTime/6000)%2==0) {

    if (AttractLastHeadMode!=1) {
      BSOS_SetLampState(HIGH_SCORE, 1, 0, 500);
      BSOS_SetLampState(GAME_OVER, 0);
      SetPlayerLamps(0);

      ShowPlayerDisplays(false, true, 4);
      BSOS_SetDisplayCredits(Credits, CreditDisplay);
      BSOS_SetDisplayBallInPlay(0, true);
    }
    AttractLastHeadMode = 1;
    
  } else {
    if (AttractLastHeadMode!=2) {
      BSOS_SetLampState(HIGH_SCORE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits, CreditDisplay);
      BSOS_SetDisplayBallInPlay(0, true);
      for (int count=0; count<4; count++) {
        if (CurrentNumPlayers>0) {
          ShowPlayerDisplays();
        } else {
          BSOS_SetDisplayBlank(count, 0x30);
          BSOS_SetDisplay(count, 0);          
        }
      }
    }
    BSOS_SetLampState(GAME_OVER, 1);
    SetPlayerLamps(((CurrentTime/250)%4) + 1);
    AttractLastHeadMode = 2;
  }

  if ((CurrentTime/6000)%3==0) {
    // This attract mode shows the skill shot on top lanes
    // and sweeps the bonus tree  
    if (AttractLastPlayfieldMode!=1) {
      if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);
      for (int count=0; count<5; count++) {
        BSOS_SetLampState(PLAYER_21+count, 0);
        BSOS_SetLampState(DEALER_21+count, 0);
      }
      BSOS_SetLampState(LEFT_OUTLANE, 0);
      BSOS_SetLampState(RIGHT_OUTLANE, 0);
      BSOS_SetLampState(SPINNER_1000, 0);
      BSOS_SetLampState(PLAYER_WINS, 0);
    }

    PulseTopLights((CurrentTime/175)%16, 0);
    ShowBonusOnTree((CurrentTime/100)%39, 0);

    byte bumper = (CurrentTime/500)%3;
    BSOS_SetLampState(RED_BUMPER, bumper==0);
    BSOS_SetLampState(SPADE_BUMPER, bumper==1);
    BSOS_SetLampState(CLUB_BUMPER, bumper==2);
    
    AttractLastPlayfieldMode = 1;
  } else if ((CurrentTime/6000)%3==1) {
    if (AttractLastPlayfieldMode!=2) {
      if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);
      BSOS_SetLampState(RED_BUMPER, 0);
      BSOS_SetLampState(SPADE_BUMPER, 0);
      BSOS_SetLampState(CLUB_BUMPER, 0);
      ShowBonusOnTree(0, 0);
    }

    // Sweep the Spinner lights
    PulseTopLights((CurrentTime/175)%16, 0);
    SweepSpinnerLights();
    SweepSaucerLights();

    AttractLastPlayfieldMode = 2;
  } else {
    if (AttractLastPlayfieldMode!=3) {
      if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);

      BSOS_SetLampState(LEFT_OUTLANE, 1, 0, 400);
      BSOS_SetLampState(RIGHT_OUTLANE, 1, 0, 400);
  
      BSOS_SetLampState(SPINNER_1000, 1, 0, 170);
      BSOS_SetLampState(PLAYER_WINS, 1, 0, 170);
      SweepSpinnerLights(true);
      SweepSaucerLights(true);
    }

    PulseTopLights((CurrentTime/175)%16, 0);
    byte id = (CurrentTime/100)%8;

    for (int count=0; count<4; count++) {
      BSOS_SetLampState(PLAYER_17-count, (id==(count)||id==(count+1)), (id==(count+1)));
      BSOS_SetLampState(DEALER_21+count, (id==(count)||id==(count+1)), (id==(count+1)));
    }
    BSOS_SetLampState(PLAYER_17-4, (id==(4)||id==0), (id==0));
    BSOS_SetLampState(DEALER_21+4, (id==(4)||id==0), (id==0));

    AttractLastPlayfieldMode = 3;
  }

  returnState = ClearSwitchBuffer(returnState, true);

  return returnState;
}


void IncreaseBonusX() {
  switch (BonusX) {
    case 1: BonusX = 2; break;
    case 2: BonusX = 3; break;
    case 3: BonusX = 5; break;
    case 5:
      // Extra ball collect
      if (NoveltyScoring) {
        CurrentScores[CurrentPlayer] += ExtraBallValue;
      } else {
        BSOS_SetLampState(HEAD_SAME_PLAYER, 1);
        SamePlayerShootsAgain = true;
      }
      BonusX = 6;
    break;
    case 6:
      // Special collect
      if (NoveltyScoring) {
        CurrentScores[CurrentPlayer] += SpecialValue;
      } else {
        if (!SpecialCollectedThisBall) {
          BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
          AddCredit();
          if (OneSpecialPerBall) SpecialCollectedThisBall = true;
        } else {
          CurrentScores[CurrentPlayer] += SpecialValue;
        }
      }
      BonusX = 7;
    break;
  }
}


boolean PlayerWinsHasBeenShown = false;

boolean RunEndOfRound() {
  boolean endOfRoundDone = false;
  // Sweep the dealer's score
  // Hit dealer if less than 17
  // Flash player wins light if player wins
  // Add bet to bonus 1k at a time or
  // Subtract bet from bonus 1k at a time
  // return true after all that is done

  byte playersDisplay = 1, dealersDisplay = 3;
  if (CurrentPlayer%2) {
    playersDisplay = 0;
    dealersDisplay = 2;
  }

  if (PlayersTopCard!=0) {
    PlayersTally += PlayersTopCard;
    if (PlayersTopCard==1) PlayerHasAce = true;
    PlayersTopCard = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {

    boolean playerWins = false;
    if (DealersTally>21) playerWins = true;
    if (PlayersTally<22 && PlayersTally>=DealersTally) playerWins = true;
    byte totalHand = DealersTopCard + DealersTally;
    boolean dealerHasToHit = false;
    if (DealerHasAce) {
      if (totalHand<7 || (totalHand>11 && totalHand<17)) dealerHasToHit = true; 
    } else {
      if (totalHand<17) dealerHasToHit = true;      
    }

    if ((CurrentTime-BettingModeStart)<2000) {
      if (PlayersTally<12 && PlayerHasAce) {
        PlayersTally += 10;
        PlayerHasAce = false;
      }
      SetTallyLamps(PlayersTally, false, PLAYER_21);
      SlideCards(BettingModeStart, dealersDisplay, DealersTopCard, DealersTally, DealerHasAce);
      PlayerWinsHasBeenShown = false;
    } else if (dealerHasToHit) {
      // Dealer has to hit
      BettingModeStart = CurrentTime;
      DealersTally += DealersTopCard;
      DealersTopCard = GetNextCard();
      PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
      if (DealersTopCard==1) DealerHasAce = true;
      SetTallyLamps(DealersTally, DealerHasAce, DEALER_21);
    } else if ((CurrentTime-BettingModeStart)<3000) {
      if (DealersTopCard) {
        DealersTally += DealersTopCard;
        DealersTopCard = 0;
        if (DealersTally<17 && DealerHasAce) {
          DealerHasAce = false;
          DealersTally += 10;
        }
        OverridePlayerDisplays(playersDisplay);
        OverridePlayerDisplays(dealersDisplay);
        BSOS_SetDisplay(playersDisplay, PlayersTally);
        BSOS_SetDisplayBlank(playersDisplay, 0x30);
        BSOS_SetDisplay(dealersDisplay, DealersTally);
        BSOS_SetDisplayBlank(dealersDisplay, 0x30);      
        SetTallyLamps(PlayersTally, false, PLAYER_21);
        SetTallyLamps(DealersTally, false, DEALER_21);
      } else {
        if (!PlayerWinsHasBeenShown && playerWins) {
          PlayerWinsHasBeenShown = true;
          BSOS_SetLampState(PLAYER_WINS, playerWins, 0, 220);        
          if (playerWins) {
            PlaySoundEffect(SOUND_EFFECT_PLAYER_WINS);
            IncreaseBonusX();
            SetBonusXLights(BonusX);
          }
        }        
      }
    } else if ((CurrentTime-BettingModeStart)<4000) {      
      if (CurrentSweepingBet>0) {
        if (playerWins) {
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
        } else {
          if (Bonus>1) Bonus -= 1;
          PlaySoundEffect(SOUND_EFFECT_BONUS_SUBTRACT);
        }
        if (CurrentSweepingBet>0) CurrentSweepingBet -= 1;
        BettingModeStart += 250;
      }
    } else {
      if (BSOS_ReadSingleSwitchState(SW_SAUCER)) BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100); 
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      endOfRoundDone = true;     
      UsingTreeToShowBet = false;
      BSOS_SetLampState(PLAYER_WINS, 0);      
    }

    LastTimeInfoUpdated = CurrentTime;
  }
  
  return endOfRoundDone;
}


boolean RunBonusCollect() {
  boolean doneWithCollect = false;
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (Bonus>0) {
      PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
      Bonus -= 1;
      CurrentScores[CurrentPlayer] += 5000;
      ShowBonusOnTree(Bonus, 0);
    } 
    if (Bonus==0) {
      doneWithCollect = true;
      if (BSOS_ReadSingleSwitchState(SW_SAUCER)) BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100); 
      UsingTreeToShowBet = false;
      BSOS_SetLampState(PLAYER_WINS, 0);
      BonusX = 6;
      SetBonusXLights(BonusX);
    }
    LastTimeInfoUpdated = CurrentTime;
  }
  return doneWithCollect;
}


boolean PayoutNatural() {
  boolean payoutDone = false;
  byte playersDisplay = 1;
  if (CurrentPlayer%2) {
    playersDisplay = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {

    if (LastTimeInfoUpdated==0) {
      PlaySoundEffect(SOUND_EFFECT_PLAYER_WINS);
      IncreaseBonusX();
      SetBonusXLights(BonusX);
      SetTallyLamps(21, false, PLAYER_21);
      BSOS_SetDisplay(playersDisplay, 21);
      BSOS_SetLampState(PLAYER_WINS, 1);      
    }

    if ((CurrentTime-BettingModeStart)<3000) {
      if (((CurrentTime-BettingModeStart)/125)%2==0) {
        BSOS_SetDisplayBlank(playersDisplay, 0x00);
      } else {
        BSOS_SetDisplayBlank(playersDisplay, 0x30);        
      }
    } else if ((CurrentTime-BettingModeStart)<3500) {
      if (CurrentSweepingBet>0) {
        AddToBonus(1);
        PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
        CurrentSweepingBet -= 1;
        BettingModeStart += 250;
      }      
    } else {
      payoutDone = true;
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      BSOS_SetLampState(PLAYER_WINS, 0);      
    }
    
    LastTimeInfoUpdated = CurrentTime;
  }

  return payoutDone;
}


//  Round doesn't start until Player bonus is greater than minimum bet
//  [BETTING_STAGE_BUILDING_STAKES]
//  Once minimum bet (MinimumBet) is reached, the bet sweeps from minimum to maximum bet (Bonus-1000) and saucer sweep lights are shown
//  [BETTING_STAGE_BET_SWEEP]
//  Once saucer is hit, cards are shown. Dealer's tally is not shown until the "Changer Dealer" standup is hit
//    [BETTING_STAGE_DEAL]
//    if player has A & 10, player's bonus += bet/2 and BettingStage goes to BETTING_STAGE_NATURAL_21 and then back to BETTING_STAGE_BET_SWEEP
//    Player lights show player tally (when it's in range)
//    Player's tally toggles (2Hz) if the player has an ace
//    [BETTING_STAGE_WAIT_FOR_COLLECT]
//    If (NoHitsOver16) is set, the hit button is disabled for scores>=17
//    "Change Player's Hand" hits (adds a card)
//      If player busts:
//        Bet is lost, and BettingStage goes back to BETTING_STAGE_BUILDING_STAKES 
//      if player hits spinner, SurrenderSpins increases
//        if SurrenderSpins>20, 1/2 of bet is surrendered
//    "Change Dealer's Hand" ShowDealerCount times will show the dealer's hidden card
//  Once saucer is hit during [BETTING_STAGE_WAIT_FOR_COLLECT]
//    [BETTING_STAGE_END_OF_ROUND]
//    if Player's hand >= Dealer's hand
//      +5000
//      Bonus += Bet
//      Advance BonusX
//    If Player's hand < Dealer's hand
//      +5000
//      Bonus -= Bet
//    Return to BETTING_STAGE_BUILDING_STAKES 


int NormalGamePlay() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  // If the playfield hasn't been validated yet, flash score and player up num
  if (BallFirstSwitchHitTime==0) {
    BSOS_SetDisplayFlash(CurrentPlayer, CurrentTime, 500, (CurrentScores[CurrentPlayer]<10)?99:CurrentScores[CurrentPlayer]);
    if (!PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1), 500);
      PlayerUpLightBlinking = true;
    }

    // We're also going to show the skill shot here until a switch is hit
    // Cycle the top lights
    int topLightPulse = (CurrentTime/250)%16;
    PulseTopLights(topLightPulse, SuitsComplete[CurrentPlayer]);
    CurrentSkillShotSuit = 4 - (topLightPulse/4);

    BettingStage = BETTING_STAGE_BUILDING_STAKES;
  } else {
    if (PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1));
      PlayerUpLightBlinking = false;
      ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
    }
    CurrentSkillShotSuit = 0;
  }

  if (BallFirstSwitchHitTime!=0 && Bonus>=1 && BettingStage==BETTING_STAGE_BUILDING_STAKES) {
    // We're entering the betting/hand mode
    BettingStage = BETTING_STAGE_BET_SWEEP;     
    BettingModeStart = CurrentTime;
    LastTimeInfoUpdated = 0;
    ClearOverridePlayerDisplays();
    SetBonusXLights(BonusX);
  }

  if (BettingStage==BETTING_STAGE_BET_SWEEP) {
    // Can only bet up to Bonus-1k
    CalculateAndShowBetSweep(1, (Bonus>1)?(Bonus-1):1);
    if ( BonusX<7 && (((CurrentTime-BettingModeStart)/2000)%2)==0 ) SweepSaucerLights();
    else SetBonusXLights(BonusX);
  }    

  if (BettingStage==BETTING_STAGE_DEAL) {
    SweepSaucerLights(true);
    SetBonusXLights(BonusX);
    PlayersTopCard = GetNextCard();
    DealersTopCard = GetNextCard();    
    PlayersTally = GetNextCard();
    DealersTally = GetNextCard();
    PlayerHasAce = (PlayersTopCard==1 || PlayersTally==1);
    DealerHasAce = (DealersTopCard==1 || DealersTally==1);
    BettingStage = BETTING_STAGE_SHOW_DEAL;
    BettingModeStart = 0;
    LastTimeInfoUpdated = 0;
    SetBonusXLights(BonusX);
  }

  if (BettingStage==BETTING_STAGE_SHOW_DEAL) {
    if (BettingModeStart==0) {
      SetBonusXLights(BonusX);
      BettingModeStart = CurrentTime;
    }
    if (ShowCards()) {
      SetBonusXLights(BonusX);
      ShowDealersCardCountdown = NumberOfDealerHitsToShow;      
      BettingStage = BETTING_STAGE_WAIT_FOR_COLLECT;
      BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100);
      PlayerHits = false;
      BettingModeStart = 0;
      LastTimeInfoUpdated = 0;
      ShowingSurrenderSpins = 0;
      SurrenderSpins = 0;
    }
  }


  if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
    if (BettingModeStart==0) {
      if (!PlayerHits) {
               
        // Player hasn't hit yet, so we need to toggle aces (if any)
        if (PlayerHasAce) {
          ToggleAce();

          // Don't have to wait any longer if it's a natural 21
          if (Natural21) {
            if (CurrentSweepingBet>2) CurrentSweepingBet += (CurrentSweepingBet / 2);
            else CurrentSweepingBet += 1;
            BettingModeStart = 0;
            BettingStage=BETTING_STAGE_NATURAL_21;
          }
        }

        // Need to pulse the Player Wins light based on likelihood of win
        ShowPlayerWinsPulse();
        
        // Alert the player that they can finish the hand or surrender
        if (ShowingSurrenderSpins==0 || (CurrentTime-ShowingSurrenderSpins)>5000) {
          SweepSpinnerLights();
        } else {
          if (SurrenderSpins>10) {
            CurrentSweepingBet /= 2;
            if (CurrentSweepingBet==0) CurrentSweepingBet = 1;
            SurrenderSpins = 0;
          }
          ShowBetUntilThisTime = CurrentTime + 3000;
          ShowBetOnTree(CurrentSweepingBet);
          UsingTreeToShowBet = true;
          ShowSurrenderLights(SurrenderSpins);
        }
        
      } else {
        if (BettingModeStart==0) {
          PlayersTopCard = GetNextCard();
          LastTimeInfoUpdated = 0;
          BettingModeStart = CurrentTime;
        }
      }
    }
    if (BettingModeStart!=0 && ShowPlayerHit()) {
      BettingModeStart = 0;
      PlayerHits = false;
    }
  }


  if (BettingStage==BETTING_STAGE_NATURAL_21) {
    if (BettingModeStart==0) {
      ShowPlayerWinsPulse(true);
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
      SweepSpinnerLights(true);
    }
    if (PayoutNatural()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
    }
  }


  if (BettingStage==BETTING_STAGE_END_OF_ROUND) {
    if (BettingModeStart==0) {
      ShowPlayerWinsPulse(true);
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
      SweepSpinnerLights(true);
    }
    if (RunEndOfRound()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
      if (BonusX==7) {
        BettingStage = BETTING_STAGE_WAIT_FOR_BONUS_COLLECT;
      }
    }
  }

  if (BettingStage==BETTING_STAGE_WAIT_FOR_BONUS_COLLECT) {
    if (BettingModeStart==0) {
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;     
      SweepSpinnerLights(true);
      ShowPlayerWinsPulse(true);
      ClearOverridePlayerDisplays();
      SetBonusXLights(BonusX);
    }

    unsigned long timeToWaitBetweenBeats = 1000;
    if (NumSecondsForBonusCollect!=0) {
      if (((CurrentTime-BettingModeStart)/1000)>((unsigned long)NumSecondsForBonusCollect-10)) timeToWaitBetweenBeats = 500;
    }
    
    if ((CurrentTime-LastTimeInfoUpdated)>timeToWaitBetweenBeats) {
      if (SuitsComplete[CurrentPlayer]==0xFF) BSOS_SetLampState(PLAYER_WINS, 1, 0, 100);
      else BSOS_SetLampState(PLAYER_WINS, 0);
      PlaySoundEffect(SOUND_EFFECT_BONUS_COLLECT_HURRY_UP);
      LastTimeInfoUpdated = CurrentTime;
    }
  }

  if (BettingStage==BETTING_STAGE_BONUS_COLLECT) {
    if (BettingModeStart==0) {
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
    }

    if (RunBonusCollect()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
      SuitsComplete[CurrentPlayer] = 0;
      ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
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
              ((CurrentTime-BallFirstSwitchHitTime)/1000)<((unsigned long)BallSaveNumSeconds) ) {
        
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
          if (BallFirstSwitchHitTime>0) {
            BallSaveUsed = true;
            BSOS_SetLampState(SAME_PLAYER, 0);
//            BSOS_SetLampState(HEAD_SAME_PLAYER, 0);
          }
          BallTimeInTrough = CurrentTime;

          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
        } else {
          SetTallyLamps(0, false, PLAYER_21);
          SetTallyLamps(0, false, DEALER_21);
          SweepSpinnerLights(true);
          SweepSaucerLights(true);
          SetBonusXLights(BonusX);
          returnState = MACHINE_STATE_COUNTDOWN_BONUS;
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  // Update Same player light
  if (BallSaveNumSeconds==0 || (BallFirstSwitchHitTime>0 && ((CurrentTime-BallFirstSwitchHitTime)/1000)>BallSaveNumSeconds) || BallSaveUsed) {
    // Only set this lamp here, if there's no ballsave or
    // the playfield is validated & the ballsave is expired
    BSOS_SetLampState(SAME_PLAYER, SamePlayerShootsAgain);
  } else if ( (BallSaveNumSeconds - (CurrentTime-BallFirstSwitchHitTime)/1000) < 3 ) {
    // if we're in the last 3 seconds of ball save, flash light very fast
    if (!BallSaveHurryUp) BSOS_SetLampState(SAME_PLAYER, 1, 0, 150);
    BallSaveHurryUp = true;
  }

  if (BallFirstSwitchHitTime!=0) {
    BSOS_SetLampState(HEAD_SAME_PLAYER, SamePlayerShootsAgain);
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
    BSOS_SetLampState(BALL_IN_PLAY, 0);
    for (int count=0; count<CurrentNumPlayers; count++) OverridePlayerDisplays(count);
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
        AddCredit();
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
      ClearOverridePlayerDisplays();
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


void HandleBumperSwitch(byte switchNum) {

  byte suitBitMask = 0x01;
  if (switchNum==SW_SPADE_BUMPER) suitBitMask = 0x04;
  if (switchNum==SW_RED_BUMPER) suitBitMask = 0x0A;

  if ((SuitsComplete[CurrentPlayer]&suitBitMask)==suitBitMask) {
    if (suitBitMask<0x08) {
      CurrentScores[CurrentPlayer] += 100;
      PlaySoundEffect(SOUND_EFFECT_BUMPER_100);
      if ( ((SuitsComplete[CurrentPlayer]/16)&suitBitMask)==suitBitMask ) CurrentScores[CurrentPlayer] += 100;
    } else {
      CurrentScores[CurrentPlayer] += 1000;
      PlaySoundEffect(SOUND_EFFECT_BUMPER_1000);
      if ( ((SuitsComplete[CurrentPlayer]/16)&suitBitMask)==suitBitMask ) CurrentScores[CurrentPlayer] += 1000;
    }
  } else {
    CurrentScores[CurrentPlayer] += 10;
    PlaySoundEffect(SOUND_EFFECT_BUMPER_10);
  }
  if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
}

void HandleTopLaneSwitch(byte switchNum) {

  byte suitBitMask = 0x08>>(switchNum-SW_HEARTS);
  if (CurrentSkillShotSuit==((switchNum-SW_HEARTS)+1)) {
    CurrentScores[CurrentPlayer] += 1000;    
    AddToBonus(2);
    PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
    if (SuitsComplete[CurrentPlayer]<0x0F) {
      // If we're working on the first leve of lanes
      if ((switchNum-SW_HEARTS)%2) SuitsComplete[CurrentPlayer] |= 0x05;
      else SuitsComplete[CurrentPlayer] |= 0x0A;
    } else {
      if ((switchNum-SW_HEARTS)%2) SuitsComplete[CurrentPlayer] |= 0x50;
      else SuitsComplete[CurrentPlayer] |= 0xA0;
      CurrentScores[CurrentPlayer] += 1000;    
      AddToBonus(2);
    }
  } else {
    CurrentScores[CurrentPlayer] += 100;
    if (SuitsComplete[CurrentPlayer]&suitBitMask) PlaySoundEffect(SOUND_EFFECT_TOPLANE_LIGHT);
    else PlaySoundEffect(SOUND_EFFECT_TOPLANE_LIGHT_PLUS);
  }
  if (SuitsComplete[CurrentPlayer]<0x0F) SuitsComplete[CurrentPlayer] |= suitBitMask;
  else SuitsComplete[CurrentPlayer] |= (suitBitMask*16);
  AddToBonus(1);
  ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
  if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
              
}


// This function handles state & switches for game play
int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];
  byte bonusAtTop = Bonus;
  
  // Very first time into gameplay loop
  if (curState==MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);    
  } else if (curState==MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState==MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = NormalGamePlay();
  } else if (curState==MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
  } else if (curState==MACHINE_STATE_BALL_OVER) {
    // Clear out lights
    SetBonusXLights(0);
    ShowPlayerWinsPulse(true);
    ShowBonusOnTree(0, 0);
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
        ClearOverridePlayerDisplays();
        ShowPlayerWinsPulse(true);
    
        returnState = MACHINE_STATE_MATCH_MODE;
      } else {
        PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
        returnState = MACHINE_STATE_INIT_NEW_BALL;
      }
    }    
  } else if (curState==MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);    
  } else {
    returnState = MACHINE_STATE_ATTRACT;
  }

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
      break;
      case SW_CLUBS:
      case SW_DIAMONDS:
      case SW_SPADES:
      case SW_HEARTS:
        HandleTopLaneSwitch(switchHit);
      break;
      case SW_SPINNER:
        if (!Spinner1kLit) CurrentScores[CurrentPlayer] += 50;
        else CurrentScores[CurrentPlayer] += 1000;
        if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
          ShowingSurrenderSpins = CurrentTime;
          SurrenderSpins += 1;
        } else {
          RegularSpins += 1;
          if (RegularSpins==5 || RegularSpins==9) {
            AddToBonus(1);
          }
          if (RegularSpins>9) RegularSpins = 0;
          ShowSingleSpinnerLight(RegularSpins);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
      break;
      case SW_SAUCER:
        SweepSaucerLights(true);
        CurrentScores[CurrentPlayer] += 5000;
if (DEBUG_MESSAGES) {
  char buf[16];
  sprintf(buf, "stg=%d\n", BettingStage);
  Serial.write(buf);
}
        // Saucer is hit as part of the skill shot!
        if (BallFirstSwitchHitTime==0) {
          SuitsComplete[CurrentPlayer] |= 0x0F;          
          ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
          PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
          BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime + 1000);
        }

        if (BettingStage==BETTING_STAGE_BUILDING_STAKES) {
          PlaySoundEffect(SOUND_EFFECT_RANDOM_SAUCER);
          BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime + 500);
        } else if (BettingStage==BETTING_STAGE_BET_SWEEP) {
          BettingStage = BETTING_STAGE_DEAL;
          BettingModeStart = CurrentTime;
          // Ball will be ejected from the saucer after the deal.
        } else if (BettingStage==BETTING_STAGE_END_OF_ROUND) {
        } else if (BettingStage==BETTING_STAGE_DEAL) {
        } else if (BettingStage==BETTING_STAGE_SHOW_DEAL) {
        } else if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
          BettingModeStart = 0;
          BettingStage = BETTING_STAGE_END_OF_ROUND;
        } else if (BettingStage==BETTING_STAGE_WAIT_FOR_BONUS_COLLECT) {
          if (SuitsComplete[CurrentPlayer]==0xFF) {
            BettingStage = BETTING_STAGE_BONUS_COLLECT;
            BettingModeStart = 0;
          } else {
            BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime);
          }
        } else if (BettingStage==BETTING_STAGE_NATURAL_21) {
          BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
      break;
      case SW_CLUB_BUMPER:
      case SW_SPADE_BUMPER:
      case SW_RED_BUMPER:
        HandleBumperSwitch(switchHit);
      break;
      case SW_CHANGE_PLAYER:
        if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
          PlayerHits = true;
          if (NoHitsOver16) {
            if (PlayersTally>16 || (PlayersTally<12 && PlayersTally>6 && PlayerHasAce)) {
              PlayerHits = false;
            }
          }
        }
        CurrentScores[CurrentPlayer] += 100;
        AddToBonus(2);
        PlaySoundEffect(SOUND_EFFECT_CHANGE_PLAYER);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
      break;
      case SW_CHANGE_DEALER:
        CurrentScores[CurrentPlayer] += 100;
        PlaySoundEffect(SOUND_EFFECT_CHANGE_DEALER);
        if (ShowDealersCardCountdown!=99 && ShowDealersCardCountdown>0) ShowDealersCardCountdown -= 1;
        if (ShowDealersCardCountdown==0) {
          if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
            BSOS_SetDisplay(2 + (1-(CurrentPlayer%2)), ((unsigned long)DealersTopCard)*10000 + (unsigned long)DealersTally);
          }
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
      break; 
      case SW_LEFT_SLING:
      case SW_RIGHT_SLING:
      case SW_10_PT_SWITCH:
        CurrentScores[CurrentPlayer] += 10;
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
      break;
      case SW_LEFT_OUTLANE:
        if (LeftOutlaneLit) {
          CurrentScores[CurrentPlayer] += 50000;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
        } else {
          CurrentScores[CurrentPlayer] += 100;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
      break;
      case SW_RIGHT_OUTLANE:
        if (RightOutlaneLit) {
          CurrentScores[CurrentPlayer] += 50000;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
        } else {
          CurrentScores[CurrentPlayer] += 100;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
        }
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
      break;
      case SW_INLANE:
        CurrentScores[CurrentPlayer] += 1000;
        AddToBonus(1);
        PlaySoundEffect(SOUND_EFFECT_INLANE);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
      break;
      case SW_BONUS_ROLLOVER:
        CurrentScores[CurrentPlayer] += 1000;
        AddToBonus(1);
        PlaySoundEffect(SOUND_EFFECT_BONUS_ROLLOVER);
        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
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
              BSOS_WriteCreditsToEEProm(Credits);
              BSOS_SetDisplayCredits(Credits, CreditDisplay);
            }
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
          }
        }
        if (DEBUG_MESSAGES) {
          Serial.write("Start game button pressed\n\r");
        }
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
            if (Credits) BSOS_SetLampState(CREDIT_LIGHT, 1);
            BSOS_SetLampState(TILT, 1);
          }
          PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
        }
      break;
    }
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    for (int awardCount=0; awardCount<3; awardCount++) {
      if (AwardScores[awardCount]!=0 && scoreAtTop<AwardScores[awardCount] && CurrentScores[CurrentPlayer]>=AwardScores[awardCount]) {
        // Player has just passed an award score, so we need to award it
        if (((AwardScoresOverride>>awardCount)&0x01)==0x01) {
          AddCredit();
          BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
          BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
        } else {
          SamePlayerShootsAgain = true;
          BSOS_SetLampState(SAME_PLAYER, SamePlayerShootsAgain);
          BSOS_SetLampState(HEAD_SAME_PLAYER, SamePlayerShootsAgain);
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        }
      }
    }
  }

  
  ShowPlayerDisplays(scoreAtTop!=CurrentScores[CurrentPlayer]);

  boolean goBackToShowingBonus = false;
  if (UsingTreeToShowBet) {
    if (CurrentTime>ShowBetUntilThisTime) {
      UsingTreeToShowBet = false;
      goBackToShowingBonus = true;    
    }
  }

  if (!UsingTreeToShowBet && (bonusAtTop!=Bonus||goBackToShowingBonus)) {
    ShowBonusOnTree(Bonus, 0);
  }

  if (!BSOS_ReadSingleSwitchState(SW_SAUCER)) {
    LastTimeSaucerEmpty = CurrentTime;    
  } else if ((CurrentTime-LastTimeSaucerEmpty)>30000) {
    BSOS_PushToSolenoidStack(SOL_SAUCER, 5, true);
    LastTimeSaucerEmpty = CurrentTime;    
  }  

  return returnState;
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

  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);
}

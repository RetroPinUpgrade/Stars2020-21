/**************************************************************************
    Stars2021 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "BSOS_Config.h"
#include "BallySternOS.h"
#include "SternStarsDefinitions.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>


// Wav Trigger defines have been moved to BSOS_Config.h

#define USE_SCORE_OVERRIDES

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
#include "SendOnlyWavTrigger.h"
SendOnlyWavTrigger wTrig;             // Our WAV Trigger object
#endif

#define STARS2021_MAJOR_VERSION  2023
#define STARS2021_MINOR_VERSION  2
#define DEBUG_MESSAGES  0


// Changes:
//  Bug fix to change music after orange star level up
//  New dim level (4) for flashing instead of dim
//  New parameter (23) for music volume (0-99)
//  New parameter (24) for Holdover stars on by default


/*********************************************************************

    Game specific code

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
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#define MACHINE_STATE_ADJUST_FREEPLAY           -17
#define MACHINE_STATE_ADJUST_BALL_SAVE          -18
#define MACHINE_STATE_ADJUST_MUSIC_LEVEL        -19
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -20
#define MACHINE_STATE_ADJUST_TILT_WARNING       -21
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     -22
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     -23
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   -24
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   -25
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      -26
#define MACHINE_STATE_ADJUST_DIM_LEVEL          -27
#define MACHINE_STATE_ADJUST_BACKGROUND_MUSIC_LEVEL  -28
#define MACHINE_STATE_ADJUST_HOLDOVER_STARS     -29
#define MACHINE_STATE_ADJUST_DONE               -30

#define GAME_MODE_SKILL_SHOT                        0
#define GAME_MODE_AWARD_SHOT                        1
#define GAME_MODE_AWARD_SHOT_COLLECTED_ANIMATION    2
#define GAME_MODE_AWARD_SHOT_MISSED_ANIMATION       3
#define GAME_MODE_UNSTRUCTURED_PLAY                 4
#define GAME_MODE_STARS_LEVEL_UP_MODE               5
#define GAME_MODE_STARS_LEVEL_UP_FINISHED           6
#define GAME_MODE_WIZARD_START                      12
#define GAME_MODE_WIZARD                            13
#define GAME_MODE_WIZARD_FINISHED                   14
#define GAME_BASE_MODE                              0x0F

#define GAME_MODE_FLAG_DROP_TARGET_FRENZY           0x10
#define GAME_MODE_FLAG_SPINNER_FRENZY               0x20
#define GAME_MODE_FLAG_POP_BUMPER_FRENZY            0x40


#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_MUSIC_LEVEL_BYTE         102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_BACKGROUND_MUSIC_BYTE    111
#define EEPROM_HOLDOVER_STARS_BYTE      112
#define EEPROM_DIM_LEVEL_BYTE           113
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144


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
#define SOUND_EFFECT_ADD_PLAYER         11
#define SOUND_EFFECT_PLAYER_UP          15
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
#define SOUND_EFFECT_DROP_SEQUENCE_SKILL  21
#define SOUND_EFFECT_ADD_PLAYER_1       50
#define SOUND_EFFECT_ADD_PLAYER_2       (SOUND_EFFECT_ADD_PLAYER_1+1)
#define SOUND_EFFECT_ADD_PLAYER_3       (SOUND_EFFECT_ADD_PLAYER_1+2)
#define SOUND_EFFECT_ADD_PLAYER_4       (SOUND_EFFECT_ADD_PLAYER_1+3)
#define SOUND_EFFECT_PLAYER_1_UP        54
#define SOUND_EFFECT_PLAYER_2_UP        (SOUND_EFFECT_PLAYER_1_UP+1)
#define SOUND_EFFECT_PLAYER_3_UP        (SOUND_EFFECT_PLAYER_1_UP+2)
#define SOUND_EFFECT_PLAYER_4_UP        (SOUND_EFFECT_PLAYER_1_UP+3)
#define SOUND_EFFECT_SHOOT_AGAIN        60
#define SOUND_EFFECT_TILT               61
#define SOUND_EFFECT_HOLDOVER_BONUS_X             62
#define SOUND_EFFECT_HOLDOVER_BONUS               (SOUND_EFFECT_HOLDOVER_BONUS_X+1)
#define SOUND_EFFECT_HOLDOVER_STARS_PROGRESS      (SOUND_EFFECT_HOLDOVER_BONUS_X+2)
#define SOUND_EFFECT_HOLDOVER_SPINNER_PROGRESS    (SOUND_EFFECT_HOLDOVER_BONUS_X+3)
#define SOUND_EFFECT_HOLDOVER_POP_BUMPER_PROGRESS (SOUND_EFFECT_HOLDOVER_BONUS_X+4)
#define SOUND_EFFECT_DROP_TARGET_FRENZY_START   70
#define SOUND_EFFECT_SPINNER_FRENZY_START       71
#define SOUND_EFFECT_POP_BUMPER_FRENZY_START    72
#define SOUND_EFFECT_MULTI_FRENZY_FINISHED      73
#define SOUND_EFFECT_SPINNER_LEFT               74
#define SOUND_EFFECT_SPINNER_FRENZY             75
#define SOUND_EFFECT_DROP_TARGET_FRENZY_FINISHED  78
#define SOUND_EFFECT_SPINNER_FRENZY_FINISHED      79
#define SOUND_EFFECT_POP_BUMPER_FRENZY_FINISHED   80
#define SOUND_EFFECT_VOICE_EXTRA_BALL             81
#define SOUND_EFFECT_BONUS_X_HELD                 82
#define SOUND_EFFECT_BONUS_HELD                   (SOUND_EFFECT_BONUS_X_HELD+1)
#define SOUND_EFFECT_STARS_PROGRESS_HELD          (SOUND_EFFECT_BONUS_X_HELD+2)
#define SOUND_EFFECT_SPINNER_PROGRESS_HELD        (SOUND_EFFECT_BONUS_X_HELD+3)
#define SOUND_EFFECT_POP_BUMPER_PROGRESS_HELD     (SOUND_EFFECT_BONUS_X_HELD+4)
#define SOUND_EFFECT_BONUS_MAX_ACHIEVED           87
#define SOUND_EFFECT_STARS_FINISHED               88
#define SOUND_EFFECT_SPINNER_MAX_FINISHED         89
#define SOUND_EFFECT_BONUSX_MAX_FINISHED          90
#define SOUND_EFFECT_WIZARD_MODE_START            91
#define SOUND_EFFECT_WIZARD_MODE_FINISHED         92
#define SOUND_EFFECT_BONUS_X_INCREASED            22


//#include "Stars2021Chimes.h"


#define SKILL_SHOT_DURATION 15
#define MAX_DISPLAY_BONUS     55
#define TILT_WARNING_DEBOUNCE_TIME      1000
#define BONUS_UNDERLIGHTS_OFF   0
#define BONUS_UNDERLIGHTS_DIM   1
#define BONUS_UNDERLIGHTS_FULL  2



#define JACKPOT_VALUE                   100000
#define SUPER_JACKPOT_VALUE             200000

#define COINS_PER_CREDIT  1

/*********************************************************************

    Machine state and options

*********************************************************************/
unsigned long HighScore = 0;
unsigned long AwardScores[3];
byte Credits = 0;
byte ChuteCoinsInProgress[3] = {0, 0, 0};
boolean FreePlayMode = false;
byte MusicLevel = 3;
byte BallSaveNumSeconds = 0;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte DimLevel = 2;
byte ScoreAwardReplay = 0;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
byte BonusUnderlights = BONUS_UNDERLIGHTS_DIM;
byte BackgroundMusicVolume = 9;
boolean HoldoverStars = false;


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte CurrentBonus;
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean CurrentlyShowingBallSave = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean ShowingModeStats = false;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;



/*********************************************************************

    STARS2021 State

*********************************************************************/
byte StarsHit[4];
byte StarLevel[4];
byte PopBumperHits[4];
byte TotalSpins[4];
byte GoalsCompletedFlags[4];
byte HoldoverAwards[4];
byte AwardPhase;
byte StarToHitForLevelUp;
byte DropTargetStatus;
byte AlternatingSpinnerPhase;
byte LeftDTBankHitOrder;
byte RightDTBankHitOrder;
byte LanePhase;
byte RolloverPhase;
byte TenPointPhase;
byte LastAwardShotCalloutPlayed;
byte LastWizardTimer;

boolean WizardScoring;
boolean RequireAllDropsFor2x = false;

unsigned long StarAnimationFinish[5];
unsigned long BonusXAnimationStart;
unsigned long ResetDropTargetStatusTime;
unsigned long AlternatingSpinnerTime;
unsigned long ScoreMultiplier;
unsigned long LastInlaneHitTime;
unsigned long FrenzyModeCompleteTime;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown; 
unsigned long LastPopBumperHit;
unsigned long LastSpinnerHit;

#define AWARD_SHOT_DURATION               30000
#define STAR_LEVEL_ANIMATION_DURATION     5000
#define INLANE_SPINNER_COMBO_HURRYUP      2000
#define ALTERNATING_SPINNER_DURATION      15000
#define FRENZY_MODE_DURATION              25000
#define WIZARD_START_DURATION             5000
#define WIZARD_DURATION                   39000
#define WIZARD_DURATION_SECONDS           39
#define WIZARD_FINISHED_DURATION          5000
#define WIZARD_SWITCH_SCORE               5000

#define AWARD_SHOT_SCORE                  10000
#define AWARD_SKILL_SHOT_SCORE            30000
#define STAR_LEVEL_UP_REWARD_SCORE        20000
#define WIZARD_MODE_REWARD_SCORE          250000

#define HOLDOVER_AWARD_BONUS_X            0x01
#define HOLDOVER_AWARD_BONUS              0x02
#define HOLDOVER_AWARD_STARS_PROGRESS     0x04
#define HOLDOVER_AWARD_SPINNER_PROGRESS   0x08
#define HOLDOVER_AWARD_POP_PROGRESS       0x10

#define GOAL_BONUS_MAX_FINISHED           0x01
#define GOAL_DROP_TARGET_MAX_FINISHED     0x02
#define GOAL_STARS_LEVEL_THREE_FINISHED   0x04
#define GOAL_SPINNER_MAX_FINISHED         0x08
#define GOAL_POP_BUMPER_FRENZY_FINISHED   0x10
#define GOAL_FLAGS_ALL_GOALS_FINISHED     0x1F

#define SPINNER_MAX_GOAL                  100
#define POP_BUMPER_LIGHT_ROLLOVER_GOAL    25
#define POP_BUMPER_LIGHT_LANE_GOAL        50
#define POP_BUMPER_START_FRENZY_GOAL      75


void ReadStoredParameters() {
  HighScore = BSOS_ReadULFromEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = BSOS_ReadByteFromEEProm(BSOS_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 3);
  if (MusicLevel > 3) MusicLevel = 3;

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  BackgroundMusicVolume = ReadSetting(EEPROM_BACKGROUND_MUSIC_BYTE, 9);
  if (BackgroundMusicVolume>9) BackgroundMusicVolume = 9; 

  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
  if (DimLevel < 2 || DimLevel > 4) DimLevel = 2;
  if (DimLevel<4) BSOS_SetDimDivisor(1, DimLevel);
  else BSOS_SetDimDivisor(1, 2);

  byte tempVal = ReadSetting(EEPROM_HOLDOVER_STARS_BYTE, 0);
  if (tempVal>1) tempVal = 0;
  HoldoverStars = (tempVal) ? true : false;

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);

}


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_STARS_SWITCHES_WITH_TRIGGERS, NUM_STARS_PRIORITY_SWITCHES_WITH_TRIGGERS, StarsSwitches);

  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = STARS2021_MAJOR_VERSION;
  CurrentScores[1] = STARS2021_MINOR_VERSION;
  CurrentScores[2] = BALLY_STERN_OS_MAJOR_VERSION;
  CurrentScores[3] = BALLY_STERN_OS_MINOR_VERSION;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);
#endif

  StopAudio();
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


byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0; 
  for (byte count=0; count<numSwitches; count++) {
    returnSwitches |= (BSOS_ReadSingleSwitchState(startingSwitch+count)<<count);
  }
  return returnSwitches;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
void ShowLampAnimation(byte animationNum, unsigned long divisor, unsigned long baseTime, byte subOffset, boolean dim, boolean reverse=false, byte keepLampOn=99) {
  byte currentStep = (baseTime/divisor)%LAMP_ANIMATION_STEPS;
  if (reverse) currentStep = (LAMP_ANIMATION_STEPS-1) - currentStep;

  byte lampNum = 0;
  for (int byteNum=0; byteNum<8; byteNum++) {
    for (byte bitNum=0; bitNum<8; bitNum++) {

      // if there's a subOffset, turn off lights at that offset
      if (subOffset) {
        byte lampOff;
        lampOff = LampAnimations[animationNum][(currentStep+subOffset)%LAMP_ANIMATION_STEPS][byteNum] & (1<<bitNum);
        if (lampOff && lampNum!=keepLampOn) BSOS_SetLampState(lampNum, 0);
      }
      
      byte lampOn = LampAnimations[animationNum][currentStep][byteNum] & (1<<bitNum);
      if (lampOn) BSOS_SetLampState(lampNum, 1, dim);
      
      lampNum += 1;
    }
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)    
    if (byteNum%2) BSOS_DataRead(0);
#endif
  }    
}


void ShowBonusLamps() {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT && !(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_BONUS)) {
    for (byte count=0; count<10; count++) BSOS_SetLampState(D1K_BONUS+count, ((AwardPhase>=16 && AwardPhase<32)&&(AwardPhase%4)), AwardPhase%2);
  } else {
    byte bonus = CurrentBonus;
    if (bonus>MAX_DISPLAY_BONUS) bonus = MAX_DISPLAY_BONUS;
    byte cap = 10;
  
    while (bonus>cap) {
      BSOS_SetLampState(D1K_BONUS+(cap-1), 1, 1, 100);
      bonus -= cap;
      cap -= 1;
      if (cap==0) bonus = 0;
    }

    if (bonus==0) {
      for (byte count=1; count<=cap; count++) BSOS_SetLampState(D1K_BONUS+(count-1), 0);
      return;
    }

    byte bottom; 
    for (bottom=1; bottom<bonus; bottom++){
      BSOS_SetLampState(D1K_BONUS+(bottom-1), (BonusUnderlights!=BONUS_UNDERLIGHTS_OFF), (BonusUnderlights==BONUS_UNDERLIGHTS_DIM));
    }
  
    if (bottom<=cap) {
      BSOS_SetLampState(D1K_BONUS+(bottom-1), 1, 0);
      for (byte count=(bottom+1); count<=cap; count++) {
        BSOS_SetLampState(D1K_BONUS+(count-1), 0);
      }
    } 
  }
}

byte StarLamps[5] = {STAR_WHITE, STAR_GREEN, STAR_AMBER, STAR_YELLOW, STAR_PURPLE};
byte StarLampsSpinnerOrder[5] = {STAR_WHITE, STAR_GREEN, STAR_AMBER, STAR_PURPLE, STAR_YELLOW};
byte StarSpecialLamps[5] = {SPECIAL_WHITE_STAR, SPECIAL_GREEN_STAR, SPECIAL_AMBER_STAR, SPECIAL_YELLOW_STAR, SPECIAL_PURPLE_STAR};
byte StarSwitches[5] = {SW_STAR_4, SW_STAR_2, SW_STAR_5, SW_STAR_3, SW_STAR_1};

void ShowStarsLamps() {
  boolean starsDisplayed = false;
  boolean starSpecialsDisplayed = false;
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT) {
    byte specialLight = AwardPhase/16;
    starSpecialsDisplayed = true;
    for (byte count=0; count<5; count++) {
      BSOS_SetLampState(StarSpecialLamps[count], ((count==specialLight)&&(AwardPhase%4)), AwardPhase%2);
      if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_STARS_PROGRESS)) {
        BSOS_SetLampState(StarLamps[count], ((AwardPhase>=32 && AwardPhase<48)&&(AwardPhase%4)), AwardPhase%2);
        starsDisplayed = true;
      } else {
        if (!(AwardPhase>=32 && AwardPhase<48)) {
          BSOS_SetLampState(StarLamps[count], 0);
          starsDisplayed = true;
        }
      }
    }
  } else if ((GameMode&GAME_BASE_MODE)==GAME_MODE_STARS_LEVEL_UP_MODE) {
    byte starPhase = ((CurrentTime-GameModeStartTime)/225)%4;
    for (byte count=0; count<5; count++) {
      BSOS_SetLampState(StarSpecialLamps[count], count==StarToHitForLevelUp, 0, 100);
      BSOS_SetLampState(StarLamps[count], starPhase, starPhase%2);
    }
    starsDisplayed = true;
  }
  if (!starsDisplayed) {
    if ((AlternatingSpinnerPhase%2)==1) {
      // We have a hurry-up on the right spinner, so use the lights to show that
      byte hurryUpLamp = (CurrentTime/100)%5;
      for (byte count=0; count<5; count++) {
        BSOS_SetLampState(StarLampsSpinnerOrder[count], count==hurryUpLamp);
      }
    } else {
      for (byte count=0; count<5; count++) {
        if (StarAnimationFinish[count]) {
          if (CurrentTime>StarAnimationFinish[count]) StarAnimationFinish[count] = 0;
          else BSOS_SetLampState(StarLamps[count], 1, 0, ((StarAnimationFinish[count]-CurrentTime)<1000)?75:150);
        } else {
          byte curStarLevel = StarLevel[CurrentPlayer];
          if (curStarLevel<3 && (StarsHit[CurrentPlayer] & (1<<count))) curStarLevel += 1;
          
          int lampFlash = 0;
          if (curStarLevel==3) lampFlash = 1000;          
          if (DimLevel==4) {
            if (curStarLevel==2) lampFlash = 1500;
            BSOS_SetLampState(StarLamps[count], curStarLevel, 0, lampFlash/2);
          } else {
            BSOS_SetLampState(StarLamps[count], curStarLevel, curStarLevel%2, lampFlash);
          }          
        }
      }
    }
    if (!starSpecialsDisplayed) {
      for (byte count=0; count<5; count++) {
        BSOS_SetLampState(StarSpecialLamps[count], 0);
      }
    }
  }
  
}


#define DOUBLE_BONUS_LAMP_MASK        0xDA
#define DOUBLE_BONUS_FLASH_MASK       0x48
#define TRIPLE_BONUS_LAMP_MASK        0xF4
#define TRIPLE_BONUS_FLASH_MASK       0xA0
/*
 * BONUSX   DOUBLE    TRIPLE
 * 1        0, 0      0, 0
 * 2        1, 0      0, 0
 * 3        0, 0      1, 0
 * 4        1, 500    0, 0
 * 5        1, 0      1, 0
 * 6        0, 0      1, 500
 * 7        1, 500    1, 0
 * 8        1, 0      1, 500
 */


void ShowBonusXLamps() {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT && !(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_BONUS_X)) {
    BSOS_SetLampState(DOUBLE_BONUS, ((AwardPhase<16)&&(AwardPhase%4)), AwardPhase%2); 
    BSOS_SetLampState(TRIPLE_BONUS, ((AwardPhase<16)&&(AwardPhase%4)), AwardPhase%2); 
  } else {
    if (BonusXAnimationStart==0) {
      byte bonusXBit = (1<<(BonusX[CurrentPlayer]-1));
      BSOS_SetLampState(DOUBLE_BONUS, DOUBLE_BONUS_LAMP_MASK&bonusXBit, 0, (DOUBLE_BONUS_FLASH_MASK&bonusXBit)?500:0); 
      BSOS_SetLampState(TRIPLE_BONUS, TRIPLE_BONUS_LAMP_MASK&bonusXBit, 0, (TRIPLE_BONUS_FLASH_MASK&bonusXBit)?500:0); 
    } else {
      if (CurrentTime>BonusXAnimationStart) {
        BonusXAnimationStart = 0;
      }
      BSOS_SetLampState(DOUBLE_BONUS, 1, 0, 110); 
      BSOS_SetLampState(TRIPLE_BONUS, 1, 0, 110); 
    }
  }
}


void ShowLaneAndRolloverLamps() {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT && !(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_POP_PROGRESS)) {
    BSOS_SetLampState(IN_LANES, ((AwardPhase>=64)&&(AwardPhase%4)), AwardPhase%2);
    BSOS_SetLampState(OUT_LANES, ((AwardPhase>=64)&&(AwardPhase%4)), AwardPhase%2);
    BSOS_SetLampState(D2_ADVANCE_BONUS, ((AwardPhase>=64)&&(AwardPhase%4)), AwardPhase%2);
  } else if ((GameMode & GAME_MODE_FLAG_POP_BUMPER_FRENZY)) {
    byte frenzyPhase = (CurrentTime/300)%4;
    BSOS_SetLampState(IN_LANES, frenzyPhase==1);
    BSOS_SetLampState(OUT_LANES, frenzyPhase==2);
    BSOS_SetLampState(D2_ADVANCE_BONUS, frenzyPhase==3);
  } else {
    BSOS_SetLampState(IN_LANES, LanePhase%2, 0, (LanePhase>2)?200:0);
    BSOS_SetLampState(OUT_LANES, LanePhase && !(LanePhase%2), 0, (LanePhase>2)?200:0);
    BSOS_SetLampState(D2_ADVANCE_BONUS, RolloverPhase%2);
  }
}

void ShowSpinnerLamps() {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT && !(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_SPINNER_PROGRESS)) {
    BSOS_SetLampState(D400_1_SPINNER, ((AwardPhase>=48 && AwardPhase<64)&&(AwardPhase%4)), AwardPhase%2);
    BSOS_SetLampState(D400_2_SPINNER, ((AwardPhase>=48 && AwardPhase<64)&&(AwardPhase%4)), AwardPhase%2);
  } else if (GameMode&GAME_MODE_FLAG_SPINNER_FRENZY) {
    // Show spinner frenzy lights
    byte frenzyPhase = ((CurrentTime-GameModeStartTime)/250)%16;
    byte light1=0, light2=0;
    int flash1=0, flash2=0;
    if (frenzyPhase<4) {
      light1 = 1;
      flash1 = 100;
    } else if (frenzyPhase<8) {
      light2 = 1;
      flash2 = 100;
    } else {
      light1 = frenzyPhase%2;
      light2 = 1-light1;
    }
    BSOS_SetLampState(D400_1_SPINNER, light1, 0, flash1);
    BSOS_SetLampState(D400_2_SPINNER, light2, 0, flash2);
  } else {
    if (AlternatingSpinnerTime!=0 && (AlternatingSpinnerPhase%2)==0) {
      // Show flashing lights to encourage alternating spinner hurry-up
      byte hurryUpPhase = ((CurrentTime-GameModeStartTime)/150)%2;
      BSOS_SetLampState(D400_1_SPINNER, hurryUpPhase);
      BSOS_SetLampState(D400_2_SPINNER, 1-hurryUpPhase);
    } else {
      // Show value of spinner
      BSOS_SetLampState(D400_1_SPINNER, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2));
      BSOS_SetLampState(D400_2_SPINNER, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5));
    }
  }
}


void ShowDropTargetLamps() {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_AWARD_SHOT) {
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_BONUS_X)) {
      BSOS_SetLampState(TRIPLE_BONUS_FEATURE, ((AwardPhase<16)&&(AwardPhase%4)), AwardPhase%2); 
      BSOS_SetLampState(DOUBLE_BONUS_FEATURE, ((AwardPhase<16)&&(AwardPhase%4)), AwardPhase%2); 
    } else {
      BSOS_SetLampState(TRIPLE_BONUS_FEATURE, 0); 
      BSOS_SetLampState(DOUBLE_BONUS_FEATURE, 0); 
    }
    BSOS_SetLampState(SPECIAL_FEATURE, 0);
    BSOS_SetLampState(WOW, 0);
    BSOS_SetLampState(D7000_LEFT, 0);
    BSOS_SetLampState(D7000_RIGHT, 0);
  } else if (GameMode&GAME_MODE_FLAG_DROP_TARGET_FRENZY) {
    byte frenzyPhase = (CurrentTime/200)%5;
    BSOS_SetLampState(SPECIAL_FEATURE, frenzyPhase==1);
    BSOS_SetLampState(TRIPLE_BONUS_FEATURE, frenzyPhase==2);
    BSOS_SetLampState(WOW, frenzyPhase==3);
    BSOS_SetLampState(DOUBLE_BONUS_FEATURE, frenzyPhase==3);
    BSOS_SetLampState(D7000_LEFT, frenzyPhase==4);
    BSOS_SetLampState(D7000_RIGHT, frenzyPhase==4);
  } else {
    if (LeftDTBankHitOrder) {
      BSOS_SetLampState(D7000_LEFT, 1, 0, (LeftDTBankHitOrder==1)?500:250);
    } else {
      BSOS_SetLampState(D7000_LEFT, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) && BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) && !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2));
    }

    if (RightDTBankHitOrder) {
      BSOS_SetLampState(D7000_RIGHT, 1, 0, (RightDTBankHitOrder==1)?500:250);
    } else {
      BSOS_SetLampState(D7000_RIGHT, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) && BSOS_ReadSingleSwitchState(SW_DROP_TARGET_6) && !BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5));
    }

    if (BonusX[CurrentPlayer]<8) {
      byte bonusXBit = (1<<(BonusX[CurrentPlayer]));
      BSOS_SetLampState(DOUBLE_BONUS_FEATURE, DOUBLE_BONUS_LAMP_MASK&bonusXBit, 0, (DOUBLE_BONUS_FLASH_MASK&bonusXBit)?500:0); 
      BSOS_SetLampState(TRIPLE_BONUS_FEATURE, TRIPLE_BONUS_LAMP_MASK&bonusXBit, 0, (TRIPLE_BONUS_FLASH_MASK&bonusXBit)?500:0); 
    }

    // At BonusX==3, we light WOW
    BSOS_SetLampState(WOW, BonusX[CurrentPlayer]==3, 0, 175); 
    BSOS_SetLampState(SPECIAL_FEATURE, BonusX[CurrentPlayer]==5, 0, 175);     
  }
}



void ShowShootAgainLamps() {

  if (!BallSaveUsed && BallSaveNumSeconds>0 && (CurrentTime-BallFirstSwitchHitTime)<((unsigned long)(BallSaveNumSeconds-1)*1000)) {
    unsigned long msRemaining = ((unsigned long)(BallSaveNumSeconds-1)*1000)-(CurrentTime-BallFirstSwitchHitTime);
    BSOS_SetLampState(SHOOT_AGAIN, 1, 0, (msRemaining<1000)?100:500);
  } else {
    BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}




////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeOverrideAnimated = 0;
unsigned long LastFlashOrDash = 0;
#ifdef USE_SCORE_OVERRIDES
unsigned long ScoreOverrideValue[4]= {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#endif
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

#ifdef USE_SCORE_OVERRIDES
void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  if (displayNum>3) return;
  ScoreOverrideStatus |= (0x10<<displayNum);
  if (animate) ScoreOverrideStatus |= (0x01<<displayNum);
  else ScoreOverrideStatus &= ~(0x01<<displayNum);
  ScoreOverrideValue[displayNum] = value;
}
#endif

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount=0; digitCount<numDigits; digitCount++) {
    displayMask |= (0x20>>digitCount);
  }  
  return displayMask;
}


void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue=0) {

#ifdef USE_SCORE_OVERRIDES      
  if (displayToUpdate==0xFF) ScoreOverrideStatus = 0;
#endif  

  byte displayMask = 0x3F;
  unsigned long displayScore = 0;
  unsigned long overrideAnimationSeed = CurrentTime/250;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime-LastTimeScoreChanged)/250)%16;
  if (scrollPhase!=LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean updateLastTimeAnimated = false;

  for (byte scoreCount=0; scoreCount<4; scoreCount++) {

#ifdef USE_SCORE_OVERRIDES      
    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue==0 && (ScoreOverrideStatus & (0x10<<scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore!=DISPLAY_OVERRIDE_BLANK_SCORE) {
        byte numDigits = MagnitudeOfScore(displayScore);
        if (numDigits==0) numDigits = 1;
        if (numDigits<(BALLY_STERN_OS_NUM_DIGITS-1) && (ScoreOverrideStatus & (0x01<<scoreCount))) {
          // This score is going to be animated (back and forth)
          if (overrideAnimationSeed!=LastTimeOverrideAnimated) {
            updateLastTimeAnimated = true;
            byte shiftDigits = (overrideAnimationSeed)%(((BALLY_STERN_OS_NUM_DIGITS+1)-numDigits)+((BALLY_STERN_OS_NUM_DIGITS-1)-numDigits));
            if (shiftDigits>=((BALLY_STERN_OS_NUM_DIGITS+1)-numDigits)) shiftDigits = (BALLY_STERN_OS_NUM_DIGITS-numDigits)*2-shiftDigits;
            byte digitCount;
            displayMask = GetDisplayMask(numDigits);
            for (digitCount=0; digitCount<shiftDigits; digitCount++) {
              displayScore *= 10;
              displayMask = displayMask>>1;
            }
            BSOS_SetDisplayBlank(scoreCount, 0x00);
            BSOS_SetDisplay(scoreCount, displayScore, false);
            BSOS_SetDisplayBlank(scoreCount, displayMask);
          }
        } else {
          BSOS_SetDisplay(scoreCount, displayScore, true, 1);
        }
      } else {
        BSOS_SetDisplayBlank(scoreCount, 0);  
      }
      
    } else {
#endif      
      // No override, update scores designated by displayToUpdate
      //CurrentScores[CurrentPlayer] = CurrentScoreOfCurrentPlayer;
      if (allScoresShowValue==0) displayScore = CurrentScores[scoreCount];
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate==0xFF || displayToUpdate==scoreCount || displayScore>BALLY_STERN_OS_MAX_DISPLAY_SCORE) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate==0xFF && (scoreCount>=CurrentNumPlayers&&CurrentNumPlayers!=0) && allScoresShowValue==0) {
          BSOS_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore>BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime-LastTimeScoreChanged)<4000) {
            BSOS_SetDisplay(scoreCount, displayScore%(BALLY_STERN_OS_MAX_DISPLAY_SCORE+1), false);  
            BSOS_SetDisplayBlank(scoreCount, BALLY_STERN_OS_ALL_DIGITS_MASK);
          } else {

            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase<11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);
              
              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase<BALLY_STERN_OS_NUM_DIGITS) {
                displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;
                for (byte scrollCount=0; scrollCount<scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE+1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0; 
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits+scrollPhase)>10) {
                byte numDigitsNeeded = (numDigits+scrollPhase)-10;
                for (byte scrollCount=0; scrollCount<(numDigits-numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              BSOS_SetDisplayBlank(scoreCount, displayMask);
              BSOS_SetDisplay(scoreCount, displayScore);
            }
          }          
        } else {
          if (flashCurrent) {
            unsigned long flashSeed = CurrentTime/250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime/250)%2)==0) BSOS_SetDisplayBlank(scoreCount, 0x00);
              else BSOS_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent) {
            unsigned long dashSeed = CurrentTime/50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime/60)%36;
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase<12) { 
                displayMask = GetDisplayMask((numDigits==0)?2:numDigits);
                if (dashPhase<7) {
                  for (byte maskCount=0; maskCount<dashPhase; maskCount++) {
                    displayMask &= ~(0x01<<maskCount);
                  }
                } else {
                  for (byte maskCount=12; maskCount>dashPhase; maskCount--) {
                    displayMask &= ~(0x20>>(maskCount-dashPhase-1));
                  }
                }
                BSOS_SetDisplay(scoreCount, displayScore);
                BSOS_SetDisplayBlank(scoreCount, displayMask);
              } else {
                BSOS_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            BSOS_SetDisplay(scoreCount, displayScore, true, 2);          
          }
        }
      } // End if this display should be updated
#ifdef USE_SCORE_OVERRIDES            
    } // End on non-overridden
#endif    
  } // End loop on scores

  if (updateLastTimeAnimated) {
    LastTimeOverrideAnimated = overrideAnimationSeed;
  }

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime-timeBase)/120;
  byte rightSideBlank = 0;
  
  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count=0; count<shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count>2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = BSOS_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum==0) curMask = 0;
  BSOS_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));  
}

/*

  XXdddddd---
           10
          100 
         1000
        10000
       10x000
      10xx000
     10xxx000
    10xxxx000
   10xxxxx000
  10xxxxxx000   
 */

////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers - 1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1 + (CurrentNumPlayers - 1));

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit==SW_COIN_2) chuteNum = 1;
  else if (switchHit==SW_COIN_3) chuteNum = 2;
  return chuteNum;   
}

unsigned short ChuteAuditByte[] = {BSOS_CHUTE_1_COINS_START_BYTE, BSOS_CHUTE_2_COINS_START_BYTE, BSOS_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum>2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  BSOS_WriteULToEEProm(coinAuditStartByte, BSOS_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
    }
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    BSOS_SetCoinLockout(true);
  }

}


boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum>2) return false;

#ifdef ENABLE_CPC_SETTINGS  
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse=0; chuteNumToUse<=chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse)==cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse]==cpcCoins) {
    if (cpcCredits>cpcCoins) AddCredit(false, cpcCredits - (coinProgressBefore));
    else AddCredit(false, cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits>cpcCoins) {
      AddCredit(false, 1);
      creditAdded = true;
    } else {
    }
  }
#else
  ChuteCoinsInProgress[0] += 1;
  if (ChuteCoinsInProgress[0]==COINS_PER_CREDIT) {
    PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
    AddCredit(false, 1);
    creditAdded = true;
    ChuteCoinsInProgress[0] = 0;
  }    
#endif  

  return creditAdded;
}


void AddSpecialCredit() {
  AddCredit(false, 1);
  BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
  BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);  
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue;
  } else {
    AddSpecialCredit();
  }
}

void AwardExtraBall() {
  if (ExtraBallCollected) return;
  ExtraBallCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ExtraBallValue;
  } else {
    SamePlayerShootsAgain = true;
    BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
    StopAudio();
    PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
    PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
    //ResumeBackgroundSong();
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

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    StopAudio();
  }

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_DONE) {
//    byte cpcSelection = 0xFF;
//    byte chuteNum = 0xFF;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
//    if (chuteNum!=0xFF) cpcSelection = GetCPCSelection(chuteNum);
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
//    if (chuteNum!=0xFF) {
//      if (cpcSelection != GetCPCSelection(chuteNum)) {
//        byte newCPC = GetCPCSelection(chuteNum);
//        Audio.StopAllAudio();
//        Audio.PlaySound(SOUND_EFFECT_SELF_TEST_CPC_START+newCPC, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
//      }
//    }  
  } else {    
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);
      }
      BSOS_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
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
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_MUSIC_LEVEL:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &MusicLevel;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
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

        case MACHINE_STATE_ADJUST_DIM_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 2;
          AdjustmentValues[1] = 3;
          AdjustmentValues[2] = 4;
          CurrentAdjustmentByte = &DimLevel;
          CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
//          for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, 1);
          break;

        case MACHINE_STATE_ADJUST_BACKGROUND_MUSIC_LEVEL:
          AdjustmentValues[1] = 9;
          CurrentAdjustmentByte = &BackgroundMusicVolume;
          CurrentAdjustmentStorageByte = EEPROM_BACKGROUND_MUSIC_BYTE;
          break;

        case MACHINE_STATE_ADJUST_HOLDOVER_STARS:
          CurrentAdjustmentByte = (byte *)&HoldoverStars;
          CurrentAdjustmentStorageByte = EEPROM_HOLDOVER_STARS_BYTE;
          break;
          
        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }

    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal > AdjustmentValues[1]) {
          if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal > 99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
          if (curVal == AdjustmentValues[valCount]) newIndex = valCount + 1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
        if (DimLevel<4) BSOS_SetDimDivisor(1, DimLevel);
        else BSOS_SetDimDivisor(1, 2);
      }
    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
//    for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, (CurrentTime / 1000) % 2);
  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
//    DecodeDIPSwitchParameters();
    BSOS_SetDisplayCredits(Credits, true);
    ReadStoredParameters();
  }

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
byte CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void StopAudio() {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif
}

void ResumeBackgroundSong() {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  byte curSong = CurrentBackgroundSong;
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
  PlayBackgroundSong(curSong);
#endif
}

void PlayBackgroundSong(byte songNum) {

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (MusicLevel > 1) {
    if (CurrentBackgroundSong != songNum) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);
      if (songNum != SOUND_EFFECT_NONE) {
#ifdef USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else
        wTrig.trackPlayPoly(songNum);
#endif
        wTrig.trackLoop(songNum, true);
        wTrig.trackGain(songNum, ((int)BackgroundMusicVolume)-9);
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

  if (MusicLevel == 0) return;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)

#ifndef USE_WAV_TRIGGER_1p3
  if (  soundEffectNum==SW_BUMPER || 
        soundEffectNum==SOUND_EFFECT_SPINNER_LOW ||
        soundEffectNum==SOUND_EFFECT_SPINNER_HIGH ||
        soundEffectNum==SOUND_EFFECT_SPINNER_LEFT ) wTrig.trackStop(soundEffectNum);
#endif
  wTrig.trackPlayPoly(soundEffectNum);
//  char buf[128];
//  sprintf(buf, "s=%d\n", soundEffectNum);
//  Serial.write(buf);
#endif

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf,"Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }

}

inline void StopSoundEffect(byte soundEffectNum) {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  wTrig.trackStop(soundEffectNum);
#endif  
}

////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
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
    BSOS_SetDisplayCredits(Credits, true);
  }

  // Alternate displays between high score and blank
  if (CurrentTime<16000) {
    if (AttractLastHeadMode!=1) {
      ShowPlayerScores(0xFF, false, false);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
    }    
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      BSOS_SetLampState(HIGHEST_SCORE, 1, 0, 250);
      BSOS_SetLampState(GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime<32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      BSOS_SetLampState(HIGHEST_SCORE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
//      BSOS_SetDisplayCredits(Credits, true);
//      BSOS_SetDisplayBallInPlay(0, true);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);
    
    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime/5000)%5); 

  if (attractPlayfieldPhase!=AttractLastPlayfieldMode) {
    BSOS_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase==2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
    if (attractPlayfieldPhase==2) {
      for (int count=0; count<5; count++) {
//        BSOS_SetLampState(FlashyLights[count], 1, count%2, 220+(count*100));
      }
    }
  }

  if (attractPlayfieldPhase < 2) {
    ShowLampAnimation(1, 40, CurrentTime, 14, false, false);
  } else if (attractPlayfieldPhase==3) {
    ShowLampAnimation(0, 40, CurrentTime, 11, false, false);
  } else if (attractPlayfieldPhase==2) {
    if ((CurrentTime-AttractLastLadderTime)>200) {
      CurrentBonus = AttractLastLadderBonus;
      ShowBonusLamps();
      AttractLastLadderBonus += 1;
      if (AttractLastLadderBonus>20) AttractLastLadderBonus = 0;
      AttractLastLadderTime = CurrentTime;      
    }
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)    
    BSOS_DataRead(0);
#endif    
  } else {
    ShowLampAnimation(2, 40, CurrentTime, 14, false, false);
  }

  byte switchHit;
  while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(byte byteToBeCounted) {
  byte numBits = 0;

  for (byte count=0; count<8; count++) {
    numBits += (byteToBeCounted&0x01);
    byteToBeCounted = byteToBeCounted>>1;
  }

  return numBits;
}


void AddToBonus(byte amountToAdd=1) {
  CurrentBonus += amountToAdd;
  if (CurrentBonus>=MAX_DISPLAY_BONUS) {
    CurrentBonus = MAX_DISPLAY_BONUS;

    if ((GoalsCompletedFlags[CurrentPlayer]&GOAL_BONUS_MAX_FINISHED)==0) {
      PlaySoundEffect(SOUND_EFFECT_BONUS_MAX_ACHIEVED);
    }
    GoalsCompletedFlags[CurrentPlayer] |= GOAL_BONUS_MAX_FINISHED;
  }
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode | (GameMode&~GAME_BASE_MODE);
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf,"Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}

void StartScoreAnimation(unsigned long scoreToAnimate) {
  if (ScoreAdditionAnimation!=0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation = scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
}


void LevelUpStars(boolean holdoverShot=false) {
  if (holdoverShot==false) {
    SetGameMode(GAME_MODE_STARS_LEVEL_UP_FINISHED);
    StartScoreAnimation(STAR_LEVEL_UP_REWARD_SCORE*((unsigned long)StarLevel[CurrentPlayer]));
  }

  if (StarLevel[CurrentPlayer]==3) {
    PlaySoundEffect(SOUND_EFFECT_STARS_FINISHED);
    GoalsCompletedFlags[CurrentPlayer] |= GOAL_STARS_LEVEL_THREE_FINISHED;
  } else {
    PlaySoundEffect(SOUND_EFFECT_HIT_STAR_LEVEL_UP);
  }
  
  StarLevel[CurrentPlayer] += 1;
  PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
  StarsHit[CurrentPlayer] = 0;
}


boolean HandleStarHit(byte switchHit) {
  byte starNum = 255;
  for (byte count=0; count<5; count++) {
    if (StarSwitches[count]==switchHit) {
      starNum = count;
    }
  }

  if (starNum>4) return false;

  // Turn on the appropriate bit in the player's star ledger
  byte starsHitBefore = StarsHit[CurrentPlayer];
  StarsHit[CurrentPlayer] |= (1<<starNum);
    
  // If all the stars in the current level have been hit
  if (StarsHit[CurrentPlayer] == 0x1F && StarLevel[CurrentPlayer]<3) {

    // If we're just getting to level-up mode, start it
    if (starsHitBefore!=0x1F) {
      // Star level up mode
      for (byte count=0; count<5; count++) {
        StarAnimationFinish[count] = CurrentTime + STAR_LEVEL_ANIMATION_DURATION;
      }
      SetGameMode(GAME_MODE_STARS_LEVEL_UP_MODE);   
      StartScoreAnimation(500*(unsigned long)(StarLevel[CurrentPlayer]+1));
      PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
    } else {
      // check to see if we've hit the level up star
      if (starNum==StarToHitForLevelUp) {
        // Level-up star was hit
        LevelUpStars();
      } else {
        // Wrong star was hit - give the normal award
        PlaySoundEffect(SOUND_EFFECT_MISSED_STAR_LEVEL_UP);
      }
    }
  } else {
    StarAnimationFinish[starNum] = CurrentTime + STAR_LEVEL_ANIMATION_DURATION;
    PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
  }

  if (GameMode==GAME_MODE_AWARD_SHOT) {
    byte litStarNum = AwardPhase/16;
    boolean promptPlayed = false;
    
    if (!(HoldoverAwards[CurrentPlayer]&(HOLDOVER_AWARD_BONUS_X<<starNum))) {
      // Play sound effect for new holdover
      PlaySoundEffect(SOUND_EFFECT_BONUS_X_HELD+starNum);
      promptPlayed = true;
    } else {
      // Special case for subsequent hit on holding over stars (get a level for subsequent hit)
      if ((HOLDOVER_AWARD_BONUS_X<<starNum)==HOLDOVER_AWARD_STARS_PROGRESS) {
        if (StarLevel[CurrentPlayer]<2) {
          LevelUpStars(true);
        }
      }
    }

    // Award shot & a skill shot
    if (litStarNum==starNum) {
      if (!promptPlayed) PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
      StartScoreAnimation(AWARD_SKILL_SHOT_SCORE);
    } else {
      if (!promptPlayed) PlaySoundEffect(SOUND_EFFECT_STAR_REWARD);
      StartScoreAnimation(AWARD_SHOT_SCORE);
    }

    HoldoverAwards[CurrentPlayer] |= (HOLDOVER_AWARD_BONUS_X<<starNum);

    ShowPlayerScores(0xFF, false, false);
    SetGameMode(GAME_MODE_AWARD_SHOT_COLLECTED_ANIMATION);
    GameModeStartTime = CurrentTime;
    GameModeEndTime = CurrentTime + 1000;    
  }

  // Is there a case where we don't want to add the normal award ?
  CurrentScores[CurrentPlayer] += 500*((unsigned long)StarLevel[CurrentPlayer]+1);
  AddToBonus();
  
  return true;
}


void IncreaseBonusX() {
  boolean soundPlayed = false;
  if (BonusX[CurrentPlayer]<8) {
    BonusX[CurrentPlayer]+=1;
    BonusXAnimationStart = CurrentTime;

    if (BonusX[CurrentPlayer]==8) {
      if ((GoalsCompletedFlags[CurrentPlayer]&GOAL_DROP_TARGET_MAX_FINISHED)==0) {
        PlaySoundEffect(SOUND_EFFECT_BONUSX_MAX_FINISHED);
        soundPlayed = true;
      }
      GoalsCompletedFlags[CurrentPlayer] |= GOAL_DROP_TARGET_MAX_FINISHED;
    }
  } 

  if (BonusX[CurrentPlayer]==4) {
    GameMode |= GAME_MODE_FLAG_DROP_TARGET_FRENZY;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FRENZY_START);
    soundPlayed = true;
    // Add to or set the Frenzy Mode completion
    if (FrenzyModeCompleteTime) {
      FrenzyModeCompleteTime += FRENZY_MODE_DURATION;
    } else {
      FrenzyModeCompleteTime = CurrentTime + FRENZY_MODE_DURATION;
    }
  } else if (BonusX[CurrentPlayer]==6) {
    AwardSpecial();
  }

  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_BONUS_X_INCREASED);
  }
  
}

void HandleDropTargetHit(byte switchHit) {

  byte currentStatus = CheckSequentialSwitches(SW_DROP_TARGET_5, 6);  
  boolean frenzyReset = false;
  boolean awardGiven = false;
  boolean soundPlayed = false;

  byte targetBit = (1<<(switchHit-SW_DROP_TARGET_5));
  // If this is a legit switch hit (not a repeat)
  if ( (targetBit & DropTargetStatus)==0 ) {

    if (LeftDTBankHitOrder==0 && ((currentStatus&0x32)==0x20)) {
      LeftDTBankHitOrder = 1;
    } else if (LeftDTBankHitOrder==1) {
      if ((currentStatus&0x32)==0x22) LeftDTBankHitOrder = 2;
      else LeftDTBankHitOrder = 0;
    } else if (LeftDTBankHitOrder==2 && switchHit==SW_DROP_TARGET_3) {
      StartScoreAnimation(15000);
      PlaySoundEffect(SOUND_EFFECT_DROP_SEQUENCE_SKILL);
      soundPlayed = true;
      awardGiven = true;
      LeftDTBankHitOrder = 0;
    }

    if (RightDTBankHitOrder==0 && ((currentStatus&0x0D)==0x08)) {
      RightDTBankHitOrder = 1;
    } else if (RightDTBankHitOrder==1) {
      if ((currentStatus&0x0D)==0x09) RightDTBankHitOrder = 2;
      else RightDTBankHitOrder = 0;
    } else if (RightDTBankHitOrder==2 && switchHit==SW_DROP_TARGET_6) {
      StartScoreAnimation(15000);
      PlaySoundEffect(SOUND_EFFECT_DROP_SEQUENCE_SKILL);
      soundPlayed = true;
      awardGiven = true;
      RightDTBankHitOrder = 0;
    }

    if (switchHit==SW_DROP_TARGET_5 && (DropTargetStatus&0x0C)==0x0C) {
      // 7k reward
      awardGiven = true;
      PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
      soundPlayed = true;
      StartScoreAnimation(7000);
    }
    if (switchHit==SW_DROP_TARGET_2 && (DropTargetStatus&0x30)==0x30) {
      // 7k reward
      awardGiven = true;
      PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
      soundPlayed = true;
      StartScoreAnimation(7000);
    }

    if (GameMode&GAME_MODE_FLAG_DROP_TARGET_FRENZY) {
      PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
      soundPlayed = true;
      StartScoreAnimation(7000);
      frenzyReset = true;
    }

    // Default scoring for a drop target
    if (!awardGiven) {
      CurrentScores[CurrentPlayer] += 500;
    }

    DropTargetStatus |= targetBit;
  }

  // If targets need to be reset
  boolean bank1Down = ((currentStatus&0x32)==0x32);
  boolean bank2Down = ((currentStatus&0x0D)==0x0D);
  boolean easyReset = (BonusX[CurrentPlayer]==1 && !RequireAllDropsFor2x && (bank1Down || bank2Down));
  if (easyReset || currentStatus>=0x3F || frenzyReset) {
    if (ResetDropTargetStatusTime==0) {
      unsigned long extraDelay = 0;
      if (frenzyReset) {
        extraDelay = 2000;
      } else {
        IncreaseBonusX();
        soundPlayed = true;
        AddToBonus(2);
      }
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 12, CurrentTime + 500 + extraDelay);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 12, CurrentTime + 750 + extraDelay);
      ResetDropTargetStatusTime = CurrentTime + 850 + extraDelay;
    }
  }

  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET);
  }

}



int InitGamePlay() {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  BSOS_EnableSolenoidStack();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
  BSOS_TurnOffAllLamps();
  StopAudio();

  // Reset displays & game state variables
  for (int count = 0; count < 4; count++) {
    // Initialize game-specific variables
    BonusX[count] = 1; 

    StarsHit[count] = 0;
    PopBumperHits[count] = 0;
    TotalSpins[count] = 0;
    Bonus[count] = 0;
    GoalsCompletedFlags[count] = 0;
    HoldoverAwards[count] = 0;
    if (HoldoverStars) HoldoverAwards[count] = HOLDOVER_AWARD_STARS_PROGRESS;
    StarLevel[count] = 0;
  }
  memset(CurrentScores, 0, 4*sizeof(unsigned long));
  
  SamePlayerShootsAgain = false;
  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;
  ShowPlayerScores(0xFF, false, false);

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    BSOS_TurnOffAllLamps();
    BallFirstSwitchHitTime = 0;

    BSOS_SetDisableFlippers(false);
    BSOS_EnableSolenoidStack();
    BSOS_SetDisplayCredits(Credits, true);
    if (CurrentNumPlayers>1 && (ballNum!=1 || playerNum!=0) && !SamePlayerShootsAgain) PlaySoundEffect(SOUND_EFFECT_PLAYER_1_UP+playerNum);
    SamePlayerShootsAgain = false;

    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(TILT, 0);

    if (BallSaveNumSeconds > 0) {
      BSOS_SetLampState(SHOOT_AGAIN, 1, 0, 500);
    }

    for (byte count=0; count<5; count++) {
      StarAnimationFinish[count] = 0;
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_AWARD_SHOT;

    ExtraBallCollected = false;
    SpecialCollected = false;

    // Start appropriate mode music
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 600);
    }

    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_LEFT, 12, CurrentTime + 100);
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RIGHT, 12, CurrentTime + 200);
    ResetDropTargetStatusTime = CurrentTime + 300;

    // Reset progress unless holdover awards
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_BONUS)) Bonus[CurrentPlayer] = 0;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_BONUS_X)) BonusX[CurrentPlayer] = 1;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_STARS_PROGRESS)) StarsHit[CurrentPlayer] = 0;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_SPINNER_PROGRESS)) TotalSpins[CurrentPlayer] = 0;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_AWARD_POP_PROGRESS)) PopBumperHits[CurrentPlayer] = 0;

    // If we drained with holdover stars & we were leveling up...
    if (StarsHit[CurrentPlayer] == 0x1F && StarLevel[CurrentPlayer]<3) {
      GameMode = GAME_MODE_STARS_LEVEL_UP_MODE;
    }

    StarToHitForLevelUp = 0;
    AwardPhase = 0;
    ScoreMultiplier = 1;
    LanePhase = 0;
    LastInlaneHitTime = 0;
    FrenzyModeCompleteTime = 0;
    CurrentBonus = Bonus[CurrentPlayer];
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    AlternatingSpinnerPhase = 0;
    AlternatingSpinnerTime = 0;
    BonusXAnimationStart = 0;
    LastPopBumperHit = 0;
    LastSpinnerHit = 0;
    LastAwardShotCalloutPlayed = 255;
    PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
    RolloverPhase = 0;
    TenPointPhase = 0;
    WizardScoring = false;
  }

  // We should only consider the ball initialized when
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

}


void PlayBackgroundSongBasedOnBall(byte ballNum) {
  if (ballNum==1) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
  } else if (ballNum==BallsPerGame) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_3);
  } else {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2);
  }
}

void PlayBackgroundSongBasedOnLevel(byte level) {
  if (level>2) level = 2;
  PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1 + level);
}


// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  boolean specialAnimationRunning = false;

  if (ResetDropTargetStatusTime && CurrentTime>ResetDropTargetStatusTime) {
    DropTargetStatus = CheckSequentialSwitches(SW_DROP_TARGET_5, 6);
    LeftDTBankHitOrder = 0;
    RightDTBankHitOrder = 0;
    ResetDropTargetStatusTime = 0;
  }

  if (PopBumperHits[CurrentPlayer]>POP_BUMPER_START_FRENZY_GOAL) {
    if (LanePhase<3) LanePhase = 3;
  } else if (PopBumperHits[CurrentPlayer]>POP_BUMPER_LIGHT_LANE_GOAL) {
    if (!LanePhase) LanePhase = 1;
  }
  if (PopBumperHits[CurrentPlayer]>POP_BUMPER_LIGHT_ROLLOVER_GOAL) {
    if (!RolloverPhase) RolloverPhase = 1;
  }
  
  boolean frenzyModeJustFinished = false;
  if (FrenzyModeCompleteTime!=0 && CurrentTime>FrenzyModeCompleteTime) {
    if (CountBits(GameMode&0x70)>1) {
      PlaySoundEffect(SOUND_EFFECT_MULTI_FRENZY_FINISHED);
    } else {
      if (GameMode&GAME_MODE_FLAG_DROP_TARGET_FRENZY) PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FRENZY_FINISHED);
      if (GameMode&GAME_MODE_FLAG_SPINNER_FRENZY) PlaySoundEffect(SOUND_EFFECT_SPINNER_FRENZY_FINISHED);
      if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) PlaySoundEffect(SOUND_EFFECT_POP_BUMPER_FRENZY_FINISHED);
    }
    frenzyModeJustFinished = true;
    FrenzyModeCompleteTime = 0;
//    if (GameMode&GAME_MODE_FLAG_SPINNER_FRENZY) GoalsCompletedFlags[CurrentPlayer] |= GOAL_SPINNER_MAX_FINISHED;
    if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) GoalsCompletedFlags[CurrentPlayer] |= GOAL_POP_BUMPER_FRENZY_FINISHED;
    GameMode &= GAME_BASE_MODE;
  }

  // There are two ways to finish the spinner goal -- 
  // Finish the frenzy (above), or get a total number of spins
  // greater than 
  if (TotalSpins[CurrentPlayer]>SPINNER_MAX_GOAL) {
    if ((GoalsCompletedFlags[CurrentPlayer]&GOAL_SPINNER_MAX_FINISHED)==0) {
      PlaySoundEffect(SOUND_EFFECT_SPINNER_MAX_FINISHED);
    }
    GoalsCompletedFlags[CurrentPlayer] |= GOAL_SPINNER_MAX_FINISHED;
  }

  byte currentWizardTimer;

  switch ( (GameMode&GAME_BASE_MODE) ) {
    case GAME_MODE_AWARD_SHOT:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
      }

      AwardPhase = ((CurrentTime-GameModeStartTime)/125)%80;

/*
      if ((CurrentTime-GameModeStartTime)>10000 && (CurrentTime-GameModeStartTime)<20000) {
        if ((AwardPhase/16)!=LastAwardShotCalloutPlayed) {
          LastAwardShotCalloutPlayed = AwardPhase/16;
          PlaySoundEffect(SOUND_EFFECT_HOLDOVER_BONUS_X+LastAwardShotCalloutPlayed);
        }
      }
*/
      if (BallFirstSwitchHitTime!=0) {
        if (GameModeEndTime==0) {
          GameModeEndTime = CurrentTime + AWARD_SHOT_DURATION;
        }

        // Should play hurry-up sounds here
        
        for (byte count=0; count<4; count++) {
          if (count!=CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, true);
        }
      }
      
      if (GameModeEndTime!=0 && CurrentTime>GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_AWARD_SHOT_MISSED_ANIMATION);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 1000;
      }
    break;

    case GAME_MODE_AWARD_SHOT_MISSED_ANIMATION:
      specialAnimationRunning = true;
      ShowLampAnimation(2, 40, (CurrentTime-GameModeStartTime), 5, true, true);
      
      if (CurrentTime>GameModeEndTime) {
        specialAnimationRunning = false;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
    break;
    
    case GAME_MODE_AWARD_SHOT_COLLECTED_ANIMATION:
      specialAnimationRunning = true;
      ShowLampAnimation(1, 40, (CurrentTime-GameModeStartTime), 3, false, false);
      
      if (CurrentTime>GameModeEndTime) {
        specialAnimationRunning = false;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
    break;

    case GAME_MODE_STARS_LEVEL_UP_MODE:
      // If this is the first time in this mode
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
      }

      if (StarLevel[CurrentPlayer]==0) {
        StarToHitForLevelUp = TenPointPhase%5;
      } else if (StarLevel[CurrentPlayer]==1) {
        StarToHitForLevelUp = ((CurrentTime-GameModeStartTime)/((1+BallsPerGame-CurrentBallInPlay)*500))%5;
      } else {
        StarToHitForLevelUp = 4 - ((CurrentTime-GameModeStartTime)/((1+BallsPerGame-CurrentBallInPlay)*500))%5;
      }

      if ((GameModeEndTime!=0) && CurrentTime>GameModeEndTime) {
        // This should never happen
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
    break;

    case GAME_MODE_STARS_LEVEL_UP_FINISHED:
      // If this is the first time in this mode
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 5000;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 40, (CurrentTime-GameModeStartTime), 14, false, false);

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_AWARD_SHOT);
      }
    break;
    
    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
      }

      if (AlternatingSpinnerTime==0 && LastInlaneHitTime!=0 && (CurrentTime-LastInlaneHitTime)<INLANE_SPINNER_COMBO_HURRYUP) {
        AlternatingSpinnerTime = CurrentTime + INLANE_SPINNER_COMBO_HURRYUP;
        AlternatingSpinnerPhase = 1;
      } else if (AlternatingSpinnerTime!=0 && CurrentTime>AlternatingSpinnerTime) {
        AlternatingSpinnerTime = 0;
        AlternatingSpinnerPhase = 0;        
      }
      if (AlternatingSpinnerPhase>=3) {
        // Star spinner frenzy mode
        PlaySoundEffect(SOUND_EFFECT_SPINNER_FRENZY_START);
        GameMode |= GAME_MODE_FLAG_SPINNER_FRENZY;
        // Add to or set the Frenzy Mode completion
        if (FrenzyModeCompleteTime) {
          FrenzyModeCompleteTime += FRENZY_MODE_DURATION;
        } else {
          FrenzyModeCompleteTime = CurrentTime + FRENZY_MODE_DURATION;
        }
        AlternatingSpinnerTime = 0;
        AlternatingSpinnerPhase = 0;        
      }

//      if ((GoalsCompletedFlags[CurrentPlayer]&GOAL_FLAGS_ALL_GOALS_FINISHED)==GOAL_FLAGS_ALL_GOALS_FINISHED) {
      if (GoalsCompletedFlags[CurrentPlayer]==GOAL_FLAGS_ALL_GOALS_FINISHED) {
        SetGameMode(GAME_MODE_WIZARD_START);
      }

      if (CurrentTime<FrenzyModeCompleteTime) {
        for (byte count=0; count<4; count++) {
          if (count!=CurrentPlayer) OverrideScoreDisplay(count, (FrenzyModeCompleteTime-CurrentTime)/1000, true);
        }
      } else if (frenzyModeJustFinished) {
        ShowPlayerScores(0xFF, false, false);
      }
      
    break;    

    case GAME_MODE_WIZARD_START:
      if (GameModeStartTime==0) {
        BSOS_TurnOffAllLamps();
        StopAudio();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_START_DURATION;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_MODE_START);
        StartScoreAnimation(WIZARD_MODE_REWARD_SCORE);
        WizardScoring = true;
        GoalsCompletedFlags[CurrentPlayer] = GOAL_BONUS_MAX_FINISHED | GOAL_DROP_TARGET_MAX_FINISHED;
        StarsHit[CurrentPlayer] = 0;
        StarLevel[CurrentPlayer] = 0;
        PopBumperHits[CurrentPlayer] = 0;
        TotalSpins[CurrentPlayer] = 0;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 80, CurrentTime, 1, false, false);

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD);
        LastWizardTimer = 0xFF;
      }
    break;

    case GAME_MODE_WIZARD:
      if (GameModeStartTime==0) {
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_WIZ);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_DURATION;
      }

      currentWizardTimer = (byte)((CurrentTime-GameModeStartTime)/1000);
      if (currentWizardTimer!=LastWizardTimer) {
        LastWizardTimer = currentWizardTimer;
        for (byte count=0; count<4; count++) {
          if (count!=CurrentPlayer) OverrideScoreDisplay(count, WIZARD_DURATION_SECONDS-LastWizardTimer, true);
        }
        PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      }
      
      specialAnimationRunning = true;
      ShowLampAnimation(0, 80, CurrentTime, 14, false, false);
      
      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD_FINISHED);
      }
    break;

    case GAME_MODE_WIZARD_FINISHED:
      if (GameModeStartTime==0) {
        BSOS_TurnOffAllLamps();
        StopAudio();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_FINISHED_DURATION;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_MODE_FINISHED);
        ShowPlayerScores(0xFF, false, false);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 80, CurrentTime, 15, false, false);

      if (CurrentTime>GameModeEndTime) {
        PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        WizardScoring = false;
      }
    break;

  }

  if ( !specialAnimationRunning && NumTiltWarnings<=MaxTiltWarnings ) {
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowStarsLamps();
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)    
    BSOS_DataRead(0);
#endif
    ShowSpinnerLamps();
    ShowShootAgainLamps();
    ShowLaneAndRolloverLamps();
    ShowDropTargetLamps();
  }

  if (ScoreAdditionAnimationStartTime!=0) {
    // Score animation   
    if ((CurrentTime-ScoreAdditionAnimationStartTime)<2000) {
      byte displayPhase = (CurrentTime-ScoreAdditionAnimationStartTime)/60;
      byte digitsToShow = 1 + displayPhase/6;
      if (digitsToShow>6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count=0; count<(6-digitsToShow); count++) {
        scoreToShow = scoreToShow/10;
      }
      if (scoreToShow==0 || displayPhase%2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer)%4;
      
      for (byte count=0; count<4; count++) {
        if (count==countdownDisplay) OverrideScoreDisplay(count, scoreToShow, false);
        else if (count!=CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer)%4;
      unsigned long remainingScore = 0; 
      if ( (CurrentTime-ScoreAdditionAnimationStartTime)<5000 ) {
        remainingScore = (((CurrentTime-ScoreAdditionAnimationStartTime)-2000)*ScoreAdditionAnimation)/3000;
        if ((remainingScore/1000)!=(LastRemainingAnimatedScoreShown/1000)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }
      
      for (byte count=0; count<4; count++) {
        if (count==countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, false);
        else if (count!=CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer]+remainingScore, false);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false);
  } else if (LastPopBumperHit!=0 || LastSpinnerHit!=0) {
    byte numberToShow = TotalSpins[CurrentPlayer];
    if (LastPopBumperHit>LastSpinnerHit) numberToShow = PopBumperHits[CurrentPlayer] + 25;

    if (numberToShow<100) {
      ShowFlybyValue(100 - numberToShow, (LastPopBumperHit>LastSpinnerHit)?LastPopBumperHit:LastSpinnerHit);
    } else {
      ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime==0)?true:false, (BallFirstSwitchHitTime>0 && ((CurrentTime-LastTimeScoreChanged)>2000))?true:false);    
    }  
    
    if ((CurrentTime-LastPopBumperHit)>1000) LastPopBumperHit = 0;
    if ((CurrentTime-LastSpinnerHit)>1000) LastSpinnerHit = 0;
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime==0)?true:false, (BallFirstSwitchHitTime>0 && ((CurrentTime-LastTimeScoreChanged)>2000))?true:false);    
  }

  // Check to see if ball is in the outhole
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
          ShowPlayerScores(0xFF, false, false);
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (!BallSaveUsed && ((CurrentTime - BallFirstSwitchHitTime)) < ((unsigned long)BallSaveNumSeconds*1000)) {
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            BallSaveUsed = true;
//            PlaySoundEffect(SOUND_EFFECT_SHOOT_AGAIN);
            BSOS_SetLampState(SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {
            ShowPlayerScores(0xFF, false, false);
            PlayBackgroundSong(SOUND_EFFECT_NONE);
            StopAudio();

            if (CurrentBallInPlay<BallsPerGame) PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    Bonus[CurrentPlayer] = CurrentBonus;
    CountdownStartTime = CurrentTime;
    ShowBonusLamps();

    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  unsigned long countdownDelayTime = 250 - (CurrentBonus*3);

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (CurrentBonus) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        PlaySoundEffect(SOUND_EFFECT_2X_BONUS_COUNT + (BonusX[CurrentPlayer]/5));
        CurrentScores[CurrentPlayer] += 1000*((unsigned long)BonusX[CurrentPlayer]);
      }

      CurrentBonus -= 1;
      
      ShowBonusLamps();
    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      BonusCountDownEndTime = CurrentTime + 1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteULToEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        BSOS_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }

    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
  }
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
    MatchDigit = CurrentTime%10;
    NumMatchSpins = 0;
    BSOS_SetLampState(MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      BSOS_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      BSOS_SetLampState(MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        BSOS_SetLampState(MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        BSOS_SetLampState(MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime / 200) % 2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x0F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}


void AddPopBumperHit(boolean addToBonus) {
  if (PopBumperHits[CurrentPlayer]<255) {
    PopBumperHits[CurrentPlayer] += 1;
    if (addToBonus && (PopBumperHits[CurrentPlayer]%5)==4) AddToBonus(1);
  }
  if (PopBumperHits[CurrentPlayer]==POP_BUMPER_START_FRENZY_GOAL) {
    PlaySoundEffect(SOUND_EFFECT_POP_BUMPER_FRENZY_START);
    GameMode |= GAME_MODE_FLAG_POP_BUMPER_FRENZY;
    // Add to or set the Frenzy Mode completion
    if (FrenzyModeCompleteTime) {
      FrenzyModeCompleteTime += FRENZY_MODE_DURATION;
    } else {
      FrenzyModeCompleteTime = CurrentTime + FRENZY_MODE_DURATION;
    }
  }
  if (LastPopBumperHit==0) LastPopBumperHit = CurrentTime;
}



int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay();
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    BSOS_SetDisplayCredits(Credits);

    if (GameMode&GAME_MODE_FLAG_SPINNER_FRENZY) GoalsCompletedFlags[CurrentPlayer] |= GOAL_SPINNER_MAX_FINISHED;
    if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) GoalsCompletedFlags[CurrentPlayer] |= GOAL_POP_BUMPER_FRENZY_FINISHED;

    if (SamePlayerShootsAgain) {
      PlaySoundEffect(SOUND_EFFECT_SHOOT_AGAIN);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {
      
      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];
      
      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  byte switchHit;
  unsigned long spinnerValue;
  ScoreMultiplier = 1 + CountBits(GameMode&0xF0);

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Switch Hit = %d\n", switchHit);
        Serial.write(buf);
      }

      if (WizardScoring) {
        if (switchHit!=SW_SLAM && switchHit!=SW_TILT) {
          PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          CurrentScores[CurrentPlayer] += WIZARD_SWITCH_SCORE;
        }
      }
      
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
          if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings > MaxTiltWarnings) {
              BSOS_DisableSolenoidStack();
              BSOS_SetDisableFlippers(true);
              BSOS_TurnOffAllLamps();
              StopAudio();
              PlaySoundEffect(SOUND_EFFECT_TILT);
              BSOS_SetLampState(TILT, 1);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_STAR_1:
        case SW_STAR_2:
        case SW_STAR_3:
        case SW_STAR_4:
        case SW_STAR_5:
          HandleStarHit(switchHit);          
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_10_PTS:
          TenPointPhase += 1;
          PlaySoundEffect(SOUND_EFFECT_10PT_SWITCH);
          CurrentScores[CurrentPlayer] += (ScoreMultiplier)*10;
          if (LanePhase) {
            LanePhase += 1;
            if (LanePhase==3) LanePhase = 1;
            else if (LanePhase==5) LanePhase = 3;
          }
          AddPopBumperHit(false);
          if (RolloverPhase && !(RolloverPhase%2)) RolloverPhase = 1;
          break;
        case SW_RIGHT_OUTLANE:
        case SW_LEFT_OUTLANE:
          if (LanePhase==2) {
            PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
            CurrentScores[CurrentPlayer] += (ScoreMultiplier)*(3500);
            AddToBonus(3);
          } else {
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
            CurrentScores[CurrentPlayer] += ScoreMultiplier*(500);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_ROLLOVER:        
          if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) {
            PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
            CurrentScores[CurrentPlayer] += ScoreMultiplier*(1000);
            AddToBonus(3);
          } else if (RolloverPhase%2) {
            AddToBonus(RolloverPhase);
            PlaySoundEffect(SOUND_EFFECT_SPINNER_LOW);
            RolloverPhase += 1;
          } else {
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
          }
          CurrentScores[CurrentPlayer] += ScoreMultiplier*(500);
          break;
        case SW_RIGHT_INLANE:
        case SW_LEFT_INLANE:
          if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) {
            PlaySoundEffect(SOUND_EFFECT_7K_BONUS);
            CurrentScores[CurrentPlayer] += ScoreMultiplier*(5000);
            AddToBonus(6);
          } else if (LanePhase%2) {
            PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
            CurrentScores[CurrentPlayer] += ScoreMultiplier*(3500);
            AddToBonus(3);
            LanePhase += 1;
          } else {
            PlaySoundEffect(SOUND_EFFECT_INLANE_UNLIT);
            CurrentScores[CurrentPlayer] += ScoreMultiplier*(500);
          }
          LastInlaneHitTime = CurrentTime; 
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SPINNER:
          if (TotalSpins[CurrentPlayer]<254) TotalSpins[CurrentPlayer] += 1;
          if (LastSpinnerHit==0) LastSpinnerHit = CurrentTime;
          if (TotalSpins[CurrentPlayer]%5==4) AddToBonus(1);
          CurrentScores[CurrentPlayer] += ScoreMultiplier*(200 + 400*(unsigned long)BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) + 400*(unsigned long)BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4));
          if (GameMode & GAME_MODE_FLAG_SPINNER_FRENZY) {
            TotalSpins[CurrentPlayer] += 2;
            CurrentScores[CurrentPlayer] += (unsigned long)5000;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_FRENZY);
          } else {
            if (CurrentTime<AlternatingSpinnerTime) {
              if ((AlternatingSpinnerPhase%2)==0) {
                AlternatingSpinnerPhase += 1;
                AlternatingSpinnerTime = CurrentTime + ALTERNATING_SPINNER_DURATION;
              }
            }
            PlaySoundEffect(SOUND_EFFECT_SPINNER_LEFT);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_SPINNER:
          if (TotalSpins[CurrentPlayer]<254) TotalSpins[CurrentPlayer] += 1;
          if (LastSpinnerHit==0) LastSpinnerHit = CurrentTime;
          if (TotalSpins[CurrentPlayer]%5==4) AddToBonus(1);
          spinnerValue = 1000*((unsigned long)StarLevel[CurrentPlayer]);
          spinnerValue += 200*((unsigned long)CountBits(StarsHit[CurrentPlayer]));
          CurrentScores[CurrentPlayer] += ScoreMultiplier*spinnerValue;
          if (GameMode & GAME_MODE_FLAG_SPINNER_FRENZY) {
            TotalSpins[CurrentPlayer] += 2;
            CurrentScores[CurrentPlayer] += 5000;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_FRENZY);
          } else {
            if (CurrentTime<AlternatingSpinnerTime) {
              if ((AlternatingSpinnerPhase%2)==1) {
                AlternatingSpinnerPhase += 1;
                AlternatingSpinnerTime = CurrentTime + ALTERNATING_SPINNER_DURATION;
              }
            }
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_DROP_TARGET_1:
        case SW_DROP_TARGET_2:
        case SW_DROP_TARGET_3:
        case SW_DROP_TARGET_4:
        case SW_DROP_TARGET_5:
        case SW_DROP_TARGET_6:
          HandleDropTargetHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_BUMPER:
          if (GameMode&GAME_MODE_FLAG_POP_BUMPER_FRENZY) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * (unsigned long)500;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_FRENZY);
          } else {
            CurrentScores[CurrentPlayer] += (unsigned long)100;
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
          }
          AddPopBumperHit(true);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SLING:
        case SW_RIGHT_SLING:
          CurrentScores[CurrentPlayer] += 10;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
          AddPopBumperHit(false);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay < 2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
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
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
      }
    }
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= BALLY_STERN_OS_MAX_DISPLAY_SCORE;
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            AwardExtraBall();
          }
        }
      }
    }
  
  }

  return returnState;
}


void loop() {

  BSOS_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);

}

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

#define NUMBER_OF_LAMPS        8

// Lamp Numbers (defines for lamps)
#define DEALER_21               00
#define DEALER_20               01
#define DEALER_19               02
#define DEALER_18               03
#define DEALER_17               04
#define SPADE_BUMPER            5
#define RED_BUMPER              6
#define CLUB_BUMPER             7
#define BONUS_1                 8
#define BONUS_2                 9
#define BONUS_3                 10
#define BONUS_4                 11
#define BONUS_5                 12
#define BONUS_ADVANCE_1         13
#define BONUS_6                 14
#define BONUS_7                 15
#define BONUS_8                 16
#define BONUS_ADVANCE_2         17
#define B_3X_5XFEATURE          18
#define B_2X_3XFEATURE          19
#define PLAYER_21               20
#define PLAYER_20               21
#define PLAYER_19               22
#define PLAYER_18               23
#define PLAYER_17               24
#define RIGHT_OUTLANE           25
#define LEFT_OUTLANE            26
#define SPINNER_1000            27
#define BONUS_TREE_1            28
#define BONUS_TREE_2            29
#define BONUS_TREE_3            30
#define BONUS_TREE_4            31
#define BONUS_TREE_5            32
#define BONUS_TREE_6            33
#define BONUS_TREE_7            34
#define BONUS_TREE_8            35
#define BONUS_TREE_9            36
#define BONUS_TREE_10           37
#define BONUS_TREE_20           38
#define PLAYER_WINS             39
#define HEAD_SAME_PLAYER        40
#define MATCH                   41
#define SAME_PLAYER             42
#define CREDIT_LIGHT            43
#define EXTRA_BALL              44
#define SPECIAL                 45
#define B_2XFEATURE_BONUS       46
#define B_5X_BONUS              47
#define BALL_IN_PLAY            48
#define HIGH_SCORE              49
#define GAME_OVER               50
#define TILT                    51
#define HEARTS_TOP_LANE         52
#define SPADES_TOP_LANE         53
#define DIAMONDS_TOP_LANE       54
#define CLUBS_TOP_LANE          55
#define PLAYER_1_UP             56
#define PLAYER_2_UP             57
#define PLAYER_3_UP             58
#define PLAYER_4_UP             59

#define NUM_OF_SWITCHES     27

// Defines for switches
#define SW_CREDIT_RESET   5
#define SW_TILT           6
#define SW_OUTHOLE        7
#define SW_COIN_3         8
#define SW_COIN_2         9
#define SW_COIN_1         10
//#define SW_BUMPER_1       11
//#define SW_LEFT_SLING     13
#define SW_LEFT_OUTLANE   33
#define SW_RIGHT_OUTLANE  32
#define SW_INLANE         30
#define SW_CLUBS          29
#define SW_DIAMONDS       28
#define SW_SPADES         27
#define SW_HEARTS         26
#define SW_SAUCER         31
#define SW_RED_BUMPER     37
#define SW_CLUB_BUMPER    39
#define SW_SPADE_BUMPER   38
#define SW_10_PT_SWITCH   34
#define SW_LEFT_SLING     36
#define SW_RIGHT_SLING    35
#define SW_SPINNER        4
#define SW_CHANGE_PLAYER  25
#define SW_CHANGE_DEALER  24
#define SW_BONUS_ROLLOVER 3
#define SW_SLAM           15

// Defines for solenoids
#define SOL_CHIME_10        1
#define SOL_CHIME_100       2
#define SOL_CHIME_1000      3
#define SOL_KNOCKER         5
#define SOL_OUTHOLE         6
#define SOL_SAUCER          7
#define SOL_CLUB_BUMPER     8
#define SOL_SPADE_BUMPER    9
#define SOL_RED_BUMPER      10
#define SOL_LEFT_SLING      11
#define SOL_RIGHT_SLING     12

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS         7

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 5

// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
struct PlayfieldAndCabinetSwitch TriggeredSwitches[] = {
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4 },
  { SW_RIGHT_SLING, SOL_RIGHT_SLING, 4 },
  { SW_SPADE_BUMPER, SOL_SPADE_BUMPER, 4 },
  { SW_CLUB_BUMPER, SOL_CLUB_BUMPER, 4 },
  { SW_RED_BUMPER, SOL_RED_BUMPER, 4 },
  { SW_10_PT_SWITCH, SOL_CHIME_10, 3 },
  { SW_SPINNER, SOL_CHIME_100, 3 }
};

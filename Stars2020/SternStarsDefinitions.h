
#define LIGHTS_ROWS 27
#define LIGHTS_COLS 16
#define NUM_STARS_LIGHTS        28


#define SPECIAL_PURPLE_STAR     0
#define SPECIAL_AMBER_STAR      4
#define D1K_BONUS               8
#define D5K_BONUS               12
#define D9K_BONUS               16
#define STAR_PURPLE             20
#define STAR_YELLOW             24
#define D2_ADVANCE_BONUS        28
#define DOUBLE_BONUS_FEATURE    36
#define SHOOT_AGAIN             40
#define D400_1_SPINNER          44
#define SPECIAL_GREEN_STAR      1
#define D2K_BONUS               9
#define D6K_BONUS               13
#define D10K_BONUS              17
#define SPECIAL_WHITE_STAR      3
#define OUT_LANES               29
#define IN_LANES                33
#define TRIPLE_BONUS_FEATURE    37
#define D400_2_SPINNER          45
#define BALL_IN_PLAY            48
#define PLAYER_1                52
#define PLAYER_1_UP             56
#define MATCH                   41
#define HIGHEST_SCORE           49
#define PLAYER_2                53
#define PLAYER_2_UP             57
#define GAME_OVER               50
#define PLAYER_3                54
#define PLAYER_3_UP             58
#define TILT                    51
#define PLAYER_4                55
#define PLAYER_4_UP             59
#define SPECIAL_YELLOW_STAR     2     
#define D3K_BONUS               10
#define D7K_BONUS               14
#define STAR_GREEN              22
#define WOW                     38
#define D7000_RIGHT             42
#define DOUBLE_BONUS            46
#define STAR_WHITE              21
#define D4K_BONUS               11
#define D8K_BONUS               15
#define STAR_AMBER              23
#define SPECIAL_FEATURE         39
#define D7000_LEFT              43
#define TRIPLE_BONUS            47


struct PlayfieldLight StarsLights[] = {
    {SPECIAL_PURPLE_STAR, 14, 3},
    {SPECIAL_AMBER_STAR, 22, 13},
    {D1K_BONUS, 3, 9},
    {D5K_BONUS, 7, 9},
    {D9K_BONUS, 11, 9},
//    {STAR_PURPLE, 0, 0},
//    {STAR_YELLOW, 0, 0},
//    {D2_ADVANCE_BONUS, 0, 0},
    {DOUBLE_BONUS_FEATURE, 17, 6},
    {SHOOT_AGAIN, 1, 8},
    {D400_1_SPINNER, 22, 5},
    {SPECIAL_GREEN_STAR, 25, 11},
    {D2K_BONUS, 4, 7},
    {D6K_BONUS, 8, 7},
    {D10K_BONUS, 12, 7},
    {SPECIAL_WHITE_STAR, 25, 7}, 
    {OUT_LANES, 7, 0}, 
    {IN_LANES, 8, 0},
    {TRIPLE_BONUS_FEATURE, 18, 7},
    {D400_2_SPINNER, 21, 5},
//    {BALL_IN_PLAY, 27, 0},
//    {PLAYER_1, 27, 0},
//    {PLAYER_1_UP, 27, 0},
//    {MATCH, 27, 0},
//    {HIGHEST_SCORE, 27, 0},
//    {PLAYER_2, 27, 0},
//    {PLAYER_2_UP, 27, 0},
//    {GAME_OVER, 27, 0},
//    {PLAYER_3, 27, 0},
//    {PLAYER_3_UP, 27, 0},
//    {TILT, 27, 0},
//    {PLAYER_4, 27, 0},
//    {PLAYER_4_UP, 27, 0},
    {SPECIAL_YELLOW_STAR, 14, 13},
    {D3K_BONUS, 5, 9},
    {D7K_BONUS, 9, 9},
//    {STAR_GREEN, 0, 0},
    {WOW, 19, 8},
    {D7000_RIGHT, 20, 8},
    {DOUBLE_BONUS, 2, 6},
//    {STAR_WHITE, 0, 0},
    {D4K_BONUS, 6, 7},
    {D8K_BONUS, 10, 7},
//    {STAR_AMBER, 0, 0},
    {SPECIAL_FEATURE, 17, 8},
    {D7000_LEFT, 18, 5},
    {TRIPLE_BONUS, 2, 10}
};

#define NUM_STARS_SWITCHES  29


#define SW_LEFT_SPINNER   0
#define SW_LEFT_OUTLANE   3
#define SW_LEFT_INLANE    4
#define SW_CREDIT_RESET   5
#define SW_TILT           6
#define SW_OUTHOLE        7
#define SW_COIN_3         8
#define SW_COIN_2         9
#define SW_COIN_1         10
#define SW_RIGHT_OUTLANE  11
#define SW_RIGHT_INLANE   12
#define SW_ROLLOVER       14
#define SW_SLAM           15
#define SW_RIGHT_SPINNER  16
#define SW_STAR_5         19
#define SW_STAR_4         20
#define SW_STAR_3         21
#define SW_STAR_2         22
#define SW_STAR_1         23
#define SW_10_PTS         24
#define SW_DROP_TARGET_5  26
#define SW_DROP_TARGET_2  27
#define SW_DROP_TARGET_6  28
#define SW_DROP_TARGET_4  29
#define SW_DROP_TARGET_3  30
#define SW_DROP_TARGET_1  31
#define SW_RIGHT_SLING    35
#define SW_LEFT_SLING     36
#define SW_BUMPER         37

#define SOL_CHIME_10      0
#define SOL_CHIME_10000   1
#define SOL_CHIME_1000    2
#define SOL_CHIME_100     3
#define SOL_KNOCKER       5
#define SOL_OUTHOLE       6
#define SOL_DROP_TARGET_LEFT  8
#define SOL_DROP_TARGET_RIGHT 9
#define SOL_BUMPER        12
#define SOL_RIGHT_SLING   13
#define SOL_LEFT_SLING    14
//#define SOL_NONE          15
#define SOLCONT_FLIPPERS      0x80
#define SOLCONT_COIN_LOCKOUT  0x01

#define NUM_STARS_SWITCHES_WITH_TRIGGERS          7
#define NUM_STARS_PRIORITY_SWITCHES_WITH_TRIGGERS 3

struct PlayfieldAndCabinetSwitch StarsSwitches[] = {
  { SW_RIGHT_SLING, SOL_RIGHT_SLING, 4},
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4},
  { SW_BUMPER, SOL_BUMPER, 3},  
  { SW_LEFT_SPINNER, SOL_CHIME_10000, 3},
  { SW_ROLLOVER, SOL_CHIME_100, 3},
  { SW_RIGHT_SPINNER, SOL_CHIME_10000, 3},
  { SW_10_PTS, SOL_CHIME_10, 4}
};

// These are exmaple values - fill these in with your machine's definitions

#define NUMBER_OF_LAMPS        8

// Lamp Numbers (defines for lamps)

#define MATCH                   41
#define SAME_PLAYER             42
#define EXTRA_BALL              44
#define BALL_IN_PLAY            48
#define HIGH_SCORE              49
#define GAME_OVER               50
#define TILT                    51
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
#define SW_LEFT_SLING     36 
#define SW_RIGHT_SLING    35
#define SW_10_PT_SWITCH   34 
#define SW_SLAM           15

// Defines for solenoids
#define SOL_CHIME_10        1
#define SOL_OUTHOLE         6
#define SOL_LEFT_SLING      11
#define SOL_RIGHT_SLING     12

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS         3

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 2

// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
struct PlayfieldAndCabinetSwitch TriggeredSwitches[] = {
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4 },
  { SW_RIGHT_SLING, SOL_RIGHT_SLING, 4 },
  { SW_10_PT_SWITCH, SOL_CHIME_10, 3 },
};

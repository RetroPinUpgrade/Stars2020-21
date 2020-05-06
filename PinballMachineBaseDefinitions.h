#define NUMBER_OF_LAMPS        8

// Lamp Numbers (defines for lamps)
#define PLAYER_1                52
#define PLAYER_1_UP             56
#define PLAYER_2                53
#define PLAYER_2_UP             57
#define PLAYER_3                54
#define PLAYER_3_UP             58
#define PLAYER_4                55
#define PLAYER_4_UP             59

#define NUM_OF_SWITCHES     6

// Defines for switches
#define SW_CREDIT_RESET   5
#define SW_TILT           6
#define SW_OUTHOLE        7
#define SW_COIN_3         8
#define SW_COIN_2         9
#define SW_COIN_1         10
#define SW_BUMPER_1       11
#define SW_LEFT_SLING     13

// Defines for solenoids
#define SOL_BUMPER_1      11
#define SOL_LEFT_SLING    13
#define SOL_OUTHOLE       14

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS          2

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 2

// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
const struct PlayfieldAndCabinetSwitch TriggeredSwitches[] = {
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4 },
  { SW_BUMPER_1, SOL_BUMPER_1, 3 }
};

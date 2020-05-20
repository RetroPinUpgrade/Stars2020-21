#ifndef SELF_TEST_H

#define MACHINE_STATE_TEST_LIGHTS         -1
#define MACHINE_STATE_TEST_DISPLAYS       -2
#define MACHINE_STATE_TEST_SOLENOIDS      -3
#define MACHINE_STATE_TEST_SWITCHES       -4
#define MACHINE_STATE_TEST_SCORE_LEVEL_1  -5
#define MACHINE_STATE_TEST_SCORE_LEVEL_2  -6
#define MACHINE_STATE_TEST_SCORE_LEVEL_3  -7
#define MACHINE_STATE_TEST_HISCR          -8
#define MACHINE_STATE_TEST_CREDITS        -9
#define MACHINE_STATE_TEST_TOTAL_PLAYS    -10
#define MACHINE_STATE_TEST_TOTAL_REPLAYS  -11
#define MACHINE_STATE_TEST_HISCR_BEAT     -12
#define MACHINE_STATE_TEST_CHUTE_2_COINS  -13
#define MACHINE_STATE_TEST_CHUTE_1_COINS  -14
#define MACHINE_STATE_TEST_CHUTE_3_COINS  -15
#define MACHINE_STATE_TEST_DONE           -30

unsigned long GetLastSelfTestChangedTime();
void SetLastSelfTestChangedTime(unsigned long setSelfTestChange);
int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch);

unsigned long GetAwardScore(byte level);

#endif

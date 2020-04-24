#include <arduino.h>
#define DEBUG_MESSAGES    1
#define BALLY_STERN_CPP_FILE
#include "BallySternOS.h"


// Global variables
volatile byte DisplayDigits[5][6];
volatile byte DisplayDigitEnable[5];
volatile boolean DisplayDim[5];
volatile boolean DisplayOffCycle = false;
volatile byte CurrentDisplayDigit=0;
volatile byte LampStates[16], LampDim0[16], LampDim1[16];
volatile int LampFlashPeriod[64];
volatile int SelfTestMode = 1;
volatile boolean AttractMode = false;

volatile byte SwitchesMinus2[5];
volatile byte SwitchesMinus1[5];
volatile byte SwitchesNow[5];
byte DipSwitches[4];


#define SOLENOID_STACK_SIZE 100
#define SOLENOID_STACK_EMPTY 0xFF
volatile int SolenoidStackFirst;
volatile int SolenoidStackLast;
volatile byte SolenoidStack[SOLENOID_STACK_SIZE];
boolean SolenoidStackEnabled = true;
volatile byte CurrentSolenoidByte = 0xFF;

#define TIMED_SOLENOID_STACK_SIZE 32
struct TimedSolenoidEntry {
  byte inUse;
  unsigned long pushTime;
  byte solenoidNumber;
  byte numPushes;
  byte disableOverride;
};
TimedSolenoidEntry TimedSolenoidStack[32];

#define SWITCH_STACK_SIZE   100
#define SWITCH_STACK_EMPTY  0xFF
volatile int SwitchStackFirst;
volatile int SwitchStackLast;
volatile byte SwitchStack[SWITCH_STACK_SIZE];

#define ADDRESS_U10_A           0x14
#define ADDRESS_U10_A_CONTROL   0x15
#define ADDRESS_U10_B           0x16
#define ADDRESS_U10_B_CONTROL   0x17
#define ADDRESS_U11_A           0x18
#define ADDRESS_U11_A_CONTROL   0x19
#define ADDRESS_U11_B           0x1A
#define ADDRESS_U11_B_CONTROL   0x1B

void BSOS_DataWrite(int address, byte data) {
  
  // Set data pins to output
  // Make pins 5-7 output (and pin 3 for R/W)
  DDRD = DDRD | 0xE8;
  // Make pins 8-12 output
  DDRB = DDRB | 0x1F;

  // Set R/W to LOW
  PORTD = (PORTD & 0xF7);

  // Put data on pins
  // Put lower three bits on 5-7
  PORTD = (PORTD&0x1F) | ((data&0x07)<<5);
  // Put upper five bits on 8-12
  PORTB = (PORTB&0xE0) | (data>>3);

  // Set up address lines
  PORTC = (PORTC & 0xE0) | address;

  // Wait for a falling edge of the clock
  while((PIND & 0x10));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTC = PORTC | 0x20;
  
  // Wait while clock is low
  while(!(PIND & 0x10));
  // Wait while clock is high
// Doesn't seem to help --  while((PIND & 0x10));

  // Set VMA OFF
  PORTC = PORTC & 0xDF;

  // Unset address lines
  PORTC = PORTC & 0xE0;
  
  // Set R/W back to HIGH
  PORTD = (PORTD | 0x08);

  // Set data pins to input
  // Make pins 5-7 input
  DDRD = DDRD & 0x1F;
  // Make pins 8-12 input
  DDRB = DDRB & 0xE0;
}



byte BSOS_DataRead(int address) {
  
  // Set data pins to input
  // Make pins 5-7 input
  DDRD = DDRD & 0x1F;
  // Make pins 8-12 input
  DDRB = DDRB & 0xE0;

  // Set R/W to HIGH
  DDRD = DDRD | 0x08;
  PORTD = (PORTD | 0x08);

  // Set up address lines
  PORTC = (PORTC & 0xE0) | address;

  // Wait for a falling edge of the clock
  while((PIND & 0x10));

  // Pulse VMA over one clock cycle
  // Set VMA ON
  PORTC = PORTC | 0x20;
  
  // Wait while clock is low
  while(!(PIND & 0x10));

  byte inputData = (PIND>>5) | (PINB<<3);

  // Set VMA OFF
  PORTC = PORTC & 0xDF;

  // Wait for a falling edge of the clock
// Doesn't seem to help  while((PIND & 0x10));

  // Set R/W to LOW
  PORTD = (PORTD & 0xF7);

  // Clear address lines
  PORTC = (PORTC & 0xE0);

  return inputData;
}


void WaitOneClockCycle() {
    // Wait while clock is low
  while(!(PIND & 0x10));

  // Wait for a falling edge of the clock
  while((PIND & 0x10));
}


void TestLightOn() {
  BSOS_DataWrite(ADDRESS_U11_A_CONTROL, BSOS_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
}

void TestLightOff() {
  BSOS_DataWrite(ADDRESS_U11_A_CONTROL, BSOS_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
}



void InitializeU10PIA() {
  // CA1 - Self Test Switch
  // CB1 - zero crossing detector
  // CA2 - NOR'd with display latch strobe
  // CB2 - lamp strobe 1
  // PA0-7 - output for switch bank, lamps, and BCD
  // PB0-7 - switch returns
  
  BSOS_DataWrite(ADDRESS_U10_A_CONTROL, 0x38);
  // Set up U10A as output
  BSOS_DataWrite(ADDRESS_U10_A, 0xFF);
  // Set bit 3 to write data
  BSOS_DataWrite(ADDRESS_U10_A_CONTROL, BSOS_DataRead(ADDRESS_U10_A_CONTROL)|0x04);
  // Store F0 in U10A Output
  BSOS_DataWrite(ADDRESS_U10_A, 0xF0);
  
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, 0x33);
  // Set up U10B as input
  BSOS_DataWrite(ADDRESS_U10_B, 0x00);
  // Set bit 3 so future reads will read data
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL)|0x04);

}

void ReadDipSwitches() {
  byte backupU10A = BSOS_DataRead(ADDRESS_U10_A);
  byte backupU10BControl = BSOS_DataRead(ADDRESS_U10_B_CONTROL);

  // Turn on Switch strobe 5 & Read Switches
  BSOS_DataWrite(ADDRESS_U10_A, 0x20);
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  for (int count=0; count<40; count++) WaitOneClockCycle();
  DipSwitches[0] = BSOS_DataRead(ADDRESS_U10_B);

  // Turn on Switch strobe 6 & Read Switches
  BSOS_DataWrite(ADDRESS_U10_A, 0x40);
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  for (int count=0; count<40; count++) WaitOneClockCycle();
  DipSwitches[1] = BSOS_DataRead(ADDRESS_U10_B);

  // Turn on Switch strobe 7 & Read Switches
  BSOS_DataWrite(ADDRESS_U10_A, 0x80);
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
  for (int count=0; count<40; count++) WaitOneClockCycle();
  DipSwitches[2] = BSOS_DataRead(ADDRESS_U10_B);

  // Turn on U10 CB2 (strobe 8) and read switches
  BSOS_DataWrite(ADDRESS_U10_A, 0x00);
  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
  for (int count=0; count<40; count++) WaitOneClockCycle();
  DipSwitches[3] = BSOS_DataRead(ADDRESS_U10_B);

  BSOS_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
  BSOS_DataWrite(ADDRESS_U10_A, backupU10A);
}


void InitializeU11PIA() {
  // CA1 - Display interrupt generator
  // CB1 - test connector pin 32
  // CA2 - lamp strobe 2
  // CB2 - solenoid bank select
  // PA0-7 - display digit enable
  // PB0-7 - solenoid data

  BSOS_DataWrite(ADDRESS_U11_A_CONTROL, 0x31);
  // Set up U11A as output
  BSOS_DataWrite(ADDRESS_U11_A, 0xFF);
  // Set bit 3 to write data
  BSOS_DataWrite(ADDRESS_U11_A_CONTROL, BSOS_DataRead(ADDRESS_U11_A_CONTROL)|0x04);
  // Store 00 in U11A Output
  BSOS_DataWrite(ADDRESS_U11_A, 0x00);
  
  BSOS_DataWrite(ADDRESS_U11_B_CONTROL, 0x30);
  // Set up U11B as output
  BSOS_DataWrite(ADDRESS_U11_B, 0xFF);
  // Set bit 3 so future reads will read data
  BSOS_DataWrite(ADDRESS_U11_B_CONTROL, BSOS_DataRead(ADDRESS_U11_B_CONTROL)|0x04);
  // Store 9F in U11B Output
  BSOS_DataWrite(ADDRESS_U11_B, 0x9F);
  CurrentSolenoidByte = 0x9F;
  
}


int SpaceLeftOnSwitchStack() {
  if (SwitchStackFirst<0 || SwitchStackLast<0 || SwitchStackFirst>=SWITCH_STACK_SIZE || SwitchStackLast>=SWITCH_STACK_SIZE) return 0;
  if (SwitchStackLast>=SwitchStackFirst) return ((SWITCH_STACK_SIZE-1) - (SwitchStackLast-SwitchStackFirst));
  return (SwitchStackFirst - SwitchStackLast) - 1;
}

void PushToSwitchStack(byte switchNumber) {
  if (switchNumber<0 || (switchNumber>39 && switchNumber!=SW_SELF_TEST_SWITCH)) return;

  // If the switch stack last index is out of range, then it's an error - return
  if (SpaceLeftOnSwitchStack()==0) return;

  // Self test is a special case - there's no good way to debounce it
  // so if it's already first on the stack, ignore it
  if (switchNumber==SW_SELF_TEST_SWITCH) {
    if (SwitchStackLast!=SwitchStackFirst && SwitchStack[SwitchStackFirst]==SW_SELF_TEST_SWITCH) return;
  }

  SwitchStack[SwitchStackLast] = switchNumber;
  
  SwitchStackLast += 1;
  if (SwitchStackLast==SWITCH_STACK_SIZE) {
    // If the end index is off the end, then wrap
    SwitchStackLast = 0;
  }
}


byte BSOS_PullFirstFromSwitchStack() {
  // If first and last are equal, there's nothing on the stack
  if (SwitchStackFirst==SwitchStackLast) return SWITCH_STACK_EMPTY;

  byte retVal = SwitchStack[SwitchStackFirst];

  SwitchStackFirst += 1;
  if (SwitchStackFirst>=SWITCH_STACK_SIZE) SwitchStackFirst = 0;

  return retVal;
}


boolean BSOS_ReadSingleSwitchState(byte switchNum) {
  if (switchNum>39 || switchNum<0) return false;

  int switchByte = switchNum/8;
  int switchBit = switchNum%8;
  if ( ((SwitchesNow[switchByte])>>switchBit) & 0x01 ) return true;
  else return false;
}


int SpaceLeftOnSolenoidStack() {
  if (SolenoidStackFirst<0 || SolenoidStackLast<0 || SolenoidStackFirst>=SOLENOID_STACK_SIZE || SolenoidStackLast>=SOLENOID_STACK_SIZE) return 0;
  if (SolenoidStackLast>=SolenoidStackFirst) return ((SOLENOID_STACK_SIZE-1) - (SolenoidStackLast-SolenoidStackFirst));
  return (SolenoidStackFirst - SolenoidStackLast) - 1;
}


void BSOS_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride = false) {
  if (solenoidNumber<0 || solenoidNumber>14) return;

  // if the solenoid stack is disabled and this isn't an override push, then return
  if (!disableOverride && !SolenoidStackEnabled) return;

  // If the solenoid stack last index is out of range, then it's an error - return
  if (SpaceLeftOnSolenoidStack()==0) return;

  for (int count=0; count<numPushes; count++) {
    SolenoidStack[SolenoidStackLast] = solenoidNumber;
    
    SolenoidStackLast += 1;
    if (SolenoidStackLast==SOLENOID_STACK_SIZE) {
      // If the end index is off the end, then wrap
      SolenoidStackLast = 0;
    }
    // If the stack is now full, return
    if (SpaceLeftOnSolenoidStack()==0) return;
  }
}

void PushToFrontOfSolenoidStack(byte solenoidNumber, byte numPushes) {
  // If the stack is full, return
  if (SpaceLeftOnSolenoidStack()==0  || !SolenoidStackEnabled) return;

  for (int count=0; count<numPushes; count++) {
    if (SolenoidStackFirst==0) SolenoidStackFirst = SOLENOID_STACK_SIZE-1;
    else SolenoidStackFirst -= 1;
    SolenoidStack[SolenoidStackFirst] = solenoidNumber;
    if (SpaceLeftOnSolenoidStack()==0) return;
  }
  
}

byte PullFirstFromSolenoidStack() {
  // If first and last are equal, there's nothing on the stack
  if (SolenoidStackFirst==SolenoidStackLast) return SOLENOID_STACK_EMPTY;
  
  byte retVal = SolenoidStack[SolenoidStackFirst];

  SolenoidStackFirst += 1;
  if (SolenoidStackFirst>=SOLENOID_STACK_SIZE) SolenoidStackFirst = 0;

  return retVal;
}


boolean BSOS_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride = false) {
  for (int count=0; count<TIMED_SOLENOID_STACK_SIZE; count++) {
    if (!TimedSolenoidStack[count].inUse) {
      TimedSolenoidStack[count].inUse = true;
      TimedSolenoidStack[count].pushTime = whenToFire;
      TimedSolenoidStack[count].disableOverride = disableOverride;
      TimedSolenoidStack[count].solenoidNumber = solenoidNumber;
      TimedSolenoidStack[count].numPushes = numPushes;
      return true;
    }
  }
  return false;
}


void BSOS_UpdateTimedSolenoidStack(unsigned long curTime) {
  for (int count=0; count<TIMED_SOLENOID_STACK_SIZE; count++) {
    if (TimedSolenoidStack[count].inUse && TimedSolenoidStack[count].pushTime<curTime) {
      BSOS_PushToSolenoidStack(TimedSolenoidStack[count].solenoidNumber, TimedSolenoidStack[count].numPushes, TimedSolenoidStack[count].disableOverride);
      TimedSolenoidStack[count].inUse = false;
    }
  }
}



volatile int numberOfU10Interrupts = 0;
volatile int numberOfU11Interrupts = 0;
volatile byte InsideZeroCrossingInterrupt = 0;


void InterruptService2() {
  byte u10AControl = BSOS_DataRead(ADDRESS_U10_A_CONTROL);
  if (u10AControl & 0x80) {
    // self test switch
    if (BSOS_DataRead(ADDRESS_U10_A_CONTROL) & 0x80) PushToSwitchStack(SW_SELF_TEST_SWITCH);
    BSOS_DataRead(ADDRESS_U10_A);
  }

  byte u11AControl = BSOS_DataRead(ADDRESS_U11_A_CONTROL);
  byte u10BControl = BSOS_DataRead(ADDRESS_U10_B_CONTROL);

  // If the interrupt bit on the display interrupt is on, do the display refresh
  if (u11AControl & 0x80) {
    // Backup U10A
    byte backupU10A = BSOS_DataRead(ADDRESS_U10_A);
    
    // Disable lamp decoders & strobe latch
    BSOS_DataWrite(ADDRESS_U10_A, 0xFF);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

// I think this should go before 10A is blasted with FF above
    // Backup U10A
//    byte backupU10A = BSOS_DataRead(ADDRESS_U10_A);

    // Blank Displays
    BSOS_DataWrite(ADDRESS_U10_A_CONTROL, BSOS_DataRead(ADDRESS_U10_A_CONTROL) & 0xF7);
    BSOS_DataWrite(ADDRESS_U11_A, (BSOS_DataRead(ADDRESS_U11_A) & 0x03) | 0x01);
    BSOS_DataWrite(ADDRESS_U10_A, 0x0F);

    // Write current display digits to 5 displays
    for (int displayCount=0; displayCount<5; displayCount++) {

      if (CurrentDisplayDigit<6) {
        // The BCD for this digit is in b4-b7, and the display latch strobes are in b0-b3 (and U11A:b0)
        byte displayDataByte = ((DisplayDigits[displayCount][CurrentDisplayDigit])<<4) | 0x0F;
        byte displayEnable = ((DisplayDigitEnable[displayCount])>>CurrentDisplayDigit)&0x01;
  
        // if this digit shouldn't be displayed, then set data lines to 0xFX so digit will be blank
        if (!displayEnable) displayDataByte = 0xFF;
        if (DisplayDim[displayCount] && DisplayOffCycle) displayDataByte = 0xFF;
  
        // Set low the appropriate latch strobe bit
        if (displayCount<4) {
          displayDataByte &= ~(0x01<<displayCount);
        }
        BSOS_DataWrite(ADDRESS_U10_A, displayDataByte);
        if (displayCount==4) {
            
          // Strobe #5 latch on U11A:b0
          BSOS_DataWrite(ADDRESS_U11_A, BSOS_DataRead(ADDRESS_U11_A) & 0xFE);
        }
        
        // Put the latch strobe bits back high
        if (displayCount<4) {
          displayDataByte |= 0x0F;
          BSOS_DataWrite(ADDRESS_U10_A, displayDataByte);
        } else {
          BSOS_DataWrite(ADDRESS_U11_A, BSOS_DataRead(ADDRESS_U11_A) | 0x01);
          
          // Set proper display digit enable
          byte displayDigitsMask = (0x04<<CurrentDisplayDigit) | 0x01;
          BSOS_DataWrite(ADDRESS_U11_A, displayDigitsMask);
        }
      }
    }

    // Stop Blanking (current digits are all latched and ready)
    BSOS_DataWrite(ADDRESS_U10_A_CONTROL, BSOS_DataRead(ADDRESS_U10_A_CONTROL) | 0x08);

    // Restore 10A from backup
    BSOS_DataWrite(ADDRESS_U10_A, backupU10A);    

    CurrentDisplayDigit = CurrentDisplayDigit + 1;
    if (CurrentDisplayDigit>5) {
      CurrentDisplayDigit = 0;
      DisplayOffCycle ^= true;
    }
    numberOfU11Interrupts+=1;
  }

  // If the IRQ bit of U10BControl is set, do the Zero-crossing interrupt handler
  if (u10BControl & 0x80 && (InsideZeroCrossingInterrupt==0)) {
    InsideZeroCrossingInterrupt = InsideZeroCrossingInterrupt + 1;

    byte u10BControlLatest = BSOS_DataRead(ADDRESS_U10_B_CONTROL);

    // Backup contents of U10A
    byte backup10A = BSOS_DataRead(ADDRESS_U10_A);

    // Latch 0xFF separately without interrupt clear
    BSOS_DataWrite(ADDRESS_U10_A, 0xFF);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
    // Read U10B to clear interrupt
    BSOS_DataRead(ADDRESS_U10_B);

    // Turn off U10BControl interrupts
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);

    int waitCount = 0;
    
    // Copy old switch values
    byte switchCount;
    byte startingClosures;
    byte validClosures;
    for (switchCount=0; switchCount<5; switchCount++) {
      SwitchesMinus2[switchCount] = SwitchesMinus1[switchCount];
      SwitchesMinus1[switchCount] = SwitchesNow[switchCount];

      // Enable playfield strobe
      BSOS_DataWrite(ADDRESS_U10_A, 0x01<<switchCount);
      BSOS_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);    
      for (waitCount=0; waitCount<40; waitCount++) WaitOneClockCycle();

      // Read the switches
      SwitchesNow[switchCount] = BSOS_DataRead(ADDRESS_U10_B);

      // Some switches need to trigger immediate closures (bumpers & slings)
      startingClosures = (SwitchesNow[switchCount]) & (~SwitchesMinus1[switchCount]);
      boolean immediateSolenoidFired = false;
      // If one of the switches is starting to close (off, on)
      if (startingClosures) {
        // Loop on bits of switch byte
        for (byte bitCount=0; bitCount<8 && immediateSolenoidFired==false; bitCount++) {
          // If this switch bit is closed
          if (startingClosures&0x01) {
            byte startingSwitchNum = switchCount*8 + bitCount;
            // Loop on immediate switch data
            for (int immediateSwitchCount=0; immediateSwitchCount<NumGamePrioritySwitches && immediateSolenoidFired==false; immediateSwitchCount++) {
              // If this switch requires immediate action
              if (GameSwitches && startingSwitchNum==GameSwitches[immediateSwitchCount].switchNum) {
                // Start firing this solenoid (just one until the closure is validate
                PushToFrontOfSolenoidStack(GameSwitches[immediateSwitchCount].solenoid, 1);
                immediateSolenoidFired = true;
              }
            }
          }
          startingClosures = startingClosures>>1;
        }
      }

      immediateSolenoidFired = false;
      validClosures = (SwitchesNow[switchCount] & SwitchesMinus1[switchCount]) & ~SwitchesMinus2[switchCount];
      // If there is a valid switch closure (off, on, on)
      if (validClosures) {
        // Loop on bits of switch byte
        for (byte bitCount=0; bitCount<8; bitCount++) {
          // If this switch bit is closed
          if (validClosures&0x01) {
            byte validSwitchNum = switchCount*8 + bitCount;
            // Loop through all switches and see what's triggered
            for (int validSwitchCount=0; validSwitchCount<NumGameSwitches; validSwitchCount++) {

              // If we've found a valid closed switch
              if (GameSwitches && GameSwitches[validSwitchCount].switchNum==validSwitchNum) {

                // If we're supposed to trigger a solenoid, then do it
                if (GameSwitches[validSwitchCount].solenoid!=SOL_NONE) {
                  if (validSwitchCount<NumGamePrioritySwitches && immediateSolenoidFired==false) {
                    PushToFrontOfSolenoidStack(GameSwitches[validSwitchCount].solenoid, GameSwitches[validSwitchCount].solenoidHoldTime);
                  } else {
                    BSOS_PushToSolenoidStack(GameSwitches[validSwitchCount].solenoid, GameSwitches[validSwitchCount].solenoidHoldTime);
                  }
                } // End if this is a real solenoid
              } // End if this is a switch in the switch table
            } // End loop on switches in switch table
            // Push this switch to the game rules stack
            PushToSwitchStack(validSwitchNum);
          }
          validClosures = validClosures>>1;
        }        
      }

      interrupts();
      for (waitCount=0; waitCount<30; waitCount++) WaitOneClockCycle();
      noInterrupts();
    }
    BSOS_DataWrite(ADDRESS_U10_A, backup10A);

    // If we need to turn off momentary solenoids, do it first
    byte momentarySolenoidAtStart = PullFirstFromSolenoidStack();
    if (momentarySolenoidAtStart!=SOLENOID_STACK_EMPTY) {
      CurrentSolenoidByte = (CurrentSolenoidByte&0xF0) | momentarySolenoidAtStart;
      BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
    } else {
      CurrentSolenoidByte = (CurrentSolenoidByte&0xF0) | SOL_NONE;
      BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
    }

    // Have to wait 350 cycles if we're looking at rising edge of zero crossing
    // REMOVED because now I waste this time reading the switches
//    interrupts();
//    for (waitCount=0; waitCount<350; waitCount++) WaitOneClockCycle();
//    noInterrupts();

    for (int lampBitCount = 0; lampBitCount<15; lampBitCount++) {
      byte lampData = 0xF0 + lampBitCount;
      // Latch address & strobe
      BSOS_DataWrite(ADDRESS_U10_A, lampData);
      BSOS_DataWrite(ADDRESS_U10_B_CONTROL, 0x38);
      BSOS_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);
      // Use the inhibit lines to set the actual data to the lamp SCRs 
      // (here, we don't care about the lower nibble because the address was already latched)
      byte lampOutput = LampStates[lampBitCount];
      // Every other time through the cycle, we OR in the dim variable
      // in order to dim those lights
      if (numberOfU10Interrupts&0x00000001) lampOutput |= LampDim0[lampBitCount];
      if (numberOfU10Interrupts&0x00000002) lampOutput |= LampDim1[lampBitCount];
      BSOS_DataWrite(ADDRESS_U10_A, lampOutput);
    }

    // Latch 0xFF separately without interrupt clear
    BSOS_DataWrite(ADDRESS_U10_A, 0xFF);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, BSOS_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

    // If we need to start any solenoids, do them now
    // (we know we need to start if we weren't already firing any solenoids
    // and there's currently something on the stack)
    if (0 && momentarySolenoidAtStart==SOLENOID_STACK_EMPTY) {
      byte startingMomentarySolenoid = PullFirstFromSolenoidStack();
      if (startingMomentarySolenoid!=SOL_NONE) {
        CurrentSolenoidByte = (CurrentSolenoidByte&0xF0) | startingMomentarySolenoid;
        BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
      }
    }

    InsideZeroCrossingInterrupt = 0;
    BSOS_DataWrite(ADDRESS_U10_A, backup10A);
    BSOS_DataWrite(ADDRESS_U10_B_CONTROL, u10BControlLatest);

    // Read U10B to clear interrupt
    BSOS_DataRead(ADDRESS_U10_B);
    numberOfU10Interrupts+=1;
  }
}




void BSOS_SetDisplay(int displayNumber, unsigned long value) {
  if (displayNumber<0 || displayNumber>4) return;

  DisplayDigits[displayNumber][0] = (value%1000000) / 100000;
  DisplayDigits[displayNumber][1] = (value%100000) / 10000;
  DisplayDigits[displayNumber][2] = (value%10000) / 1000;
  DisplayDigits[displayNumber][3] = (value%1000) / 100;
  DisplayDigits[displayNumber][4] = (value%100) / 10;
  DisplayDigits[displayNumber][5] = (value%10) / 1;
}

void BSOS_SetDisplayBlank(int displayNumber, byte bitMask) {
  if (displayNumber<0 || displayNumber>4) return;
  
  DisplayDigitEnable[displayNumber] = bitMask;
}

// This is confusing -
// Digit mask is like this
//   bit=   b7 b6 b5 b4 b3 b2 b1 b0
//   digit=  x  x  6  5  4  3  2  1
//   (with digit 6 being the least-significant, 1's digit
//  
// so, looking at it from left to right on the display
//   digit=  1  2  3  4  5  6
//   bit=   b0 b1 b2 b3 b4 b5


void BSOS_SetDisplayBlankByMagnitude(int displayNumber, unsigned long value) {
  if (displayNumber<0 || displayNumber>4) return;

  DisplayDigitEnable[displayNumber] = 0x20;
  if (value>9) DisplayDigitEnable[displayNumber] |= 0x10;
  if (value>99) DisplayDigitEnable[displayNumber] |= 0x08;
  if (value>999) DisplayDigitEnable[displayNumber] |= 0x04;
  if (value>9999) DisplayDigitEnable[displayNumber] |= 0x02;
  if (value>99999) DisplayDigitEnable[displayNumber] |= 0x01;
}

void BSOS_SetDisplayBlankForCreditMatch(boolean creditsOn, boolean matchOn) {
  DisplayDigitEnable[4] = 0;
  if (creditsOn) DisplayDigitEnable[4] |= 0x03;
  if (matchOn) DisplayDigitEnable[4] |= 0x18;
}

void BSOS_SetDisplayFlash(int displayNumber, unsigned long curTime, int period=500, unsigned long magnitude=999999) {
  // A period of zero toggles display every other time
  if (period) {
    if ((curTime/period)%2) {
      BSOS_SetDisplayBlankByMagnitude(displayNumber, magnitude);
    } else {
      BSOS_SetDisplayBlank(displayNumber, 0);
    }
  }
  
}


void BSOS_SetDisplayFlashCredits(unsigned long curTime, int period=100) {
  if (period) {
    if ((curTime/period)%2) {
      DisplayDigitEnable[4] |= 0x06;
    } else {
      DisplayDigitEnable[4] &= 0x39;
    }
  }
}


void BSOS_SetDisplayCredits(int value, boolean displayOn) {
  DisplayDigits[4][1] = (value%100) / 10;
  DisplayDigits[4][2] = (value%10) / 1;

  if (displayOn) {
    if (value>9) DisplayDigitEnable[4] |= 0x06;
    else DisplayDigitEnable[4] |= 0x04;
  } else {
    DisplayDigitEnable[4] &= 0x39;
  }
}

void BSOS_SetDisplayMatch(int value, boolean displayOn) {
  DisplayDigits[4][1] = (value%100) / 10;
  DisplayDigits[4][2] = (value%10) / 1;

  if (displayOn) DisplayDigitEnable[4] |= 0x06;
  else DisplayDigitEnable[4] &= 0x39;
}

void BSOS_SetDisplayBallInPlay(int value, boolean displayOn) {
  DisplayDigits[4][4] = (value%100) / 10;
  DisplayDigits[4][5] = (value%10) / 1;  

  if (displayOn) DisplayDigitEnable[4] |= 0x30;
  else DisplayDigitEnable[4] &= 0x0F;
}


void BSOS_SetDisplayBIPBlank(byte digitsOn=1) {
  if (digitsOn==0) DisplayDigitEnable[4] &= 0x0F;
  else if (digitsOn==1) DisplayDigitEnable[4] = (DisplayDigitEnable[4] & 0x0F)|0x20;
  else if (digitsOn==2) DisplayDigitEnable[4] = (DisplayDigitEnable[4] & 0x0F)|0x30;  
}



void BSOS_SetLampState(int lampNum, byte s_lampState, byte s_lampDim=0, int s_lampFlashPeriod=0) {
  if (lampNum>59 || lampNum<0) return;
  
  if (s_lampState) {
    LampStates[lampNum/4] &= ~(0x10<<(lampNum%4));
    LampFlashPeriod[lampNum] = s_lampFlashPeriod;
  } else {
    LampStates[lampNum/4] |= (0x10<<(lampNum%4));
    LampFlashPeriod[lampNum] = 0;
  }

  if (s_lampDim & 0x01) {    
    LampDim0[lampNum/4] |= (0x10<<(lampNum%4));
  } else {
    LampDim0[lampNum/4] &= ~(0x10<<(lampNum%4));
  }

  if (s_lampDim & 0x02) {    
    LampDim1[lampNum/4] |= (0x10<<(lampNum%4));
  } else {
    LampDim1[lampNum/4] &= ~(0x10<<(lampNum%4));
  }

}


void BSOS_ApplyFlashToLamps(unsigned long curTime) {
  for (int count=0; count<60; count++) {
    if ( LampFlashPeriod[count]!=0 ) {
      if ((curTime/LampFlashPeriod[count])%2) {
        LampStates[count/4] &= ~(0x10<<(count%4));
      } else {
        LampStates[count/4] |= (0x10<<(count%4));
      }
    } // end if this light should flash
  } // end loop on lights
}


void BSOS_FlashAllLamps(unsigned long curTime) {
  for (int count=0; count<60; count++) {
    BSOS_SetLampState(count, 1, 0, 500);  
  }

  BSOS_ApplyFlashToLamps(curTime);
}

void BSOS_TurnOffAllLamps() {
  for (int count=0; count<60; count++) {
    BSOS_SetLampState(count, 0, 0, 0);  
  }
}


void BSOS_InitializeMPU() {
  // Wait for board to boot
  delay(100);
  
  // Arduino A0 = MPU A0
  // Arduino A1 = MPU A1
  // Arduino A2 = MPU A3
  // Arduino A3 = MPU A4
  // Arduino A4 = MPU A7
  // Arduino A5 = MPU VMA
  // Set up the address lines A0-A7 as output
  DDRC = DDRC | 0x3F;

  // Set up A6 as output
  pinMode(A6, OUTPUT); // /HLT

  // Arduino 2 = /IRQ (input)
  // Arduino 3 = R/W (output)
  // Arduino 4 = Clk (input)
  // Arduino 5 = D0
  // Arduino 6 = D1
  // Arduino 7 = D3
  // Set up control lines & data lines
  DDRD = DDRD & 0xEB;
  DDRD = DDRD | 0xE8;

  digitalWrite(3, HIGH);  // Set R/W line high (Read)
  digitalWrite(A5, LOW);  // Set VMA line LOW
  digitalWrite(A6, HIGH); // Set

  pinMode(2, INPUT);

  // Prep the address bus (all lines zero)
  BSOS_DataRead(0);
  // Set up the PIAs
  InitializeU10PIA();
  InitializeU11PIA();

  // Read values from MPU dip switches
  ReadDipSwitches();
  
  // Reset address bus
  BSOS_DataRead(0);
  
  // Hook up the interrupt
  attachInterrupt(digitalPinToInterrupt(2), InterruptService2, LOW);
  BSOS_DataRead(0);  // Reset address bus

  // Cleary all possible interrupts by reading the registers
  byte dataRegister;
  dataRegister = BSOS_DataRead(ADDRESS_U11_A);
  dataRegister = BSOS_DataRead(ADDRESS_U11_B);
  dataRegister = BSOS_DataRead(ADDRESS_U10_A);
  dataRegister = BSOS_DataRead(ADDRESS_U10_B);
  BSOS_DataRead(0);  // Reset address bus

  // Set default values for the displays
  for (int displayCount=0; displayCount<5; displayCount++) {
    DisplayDigits[displayCount][0] = 0;
    DisplayDigits[displayCount][1] = 0;
    DisplayDigits[displayCount][2] = 0;
    DisplayDigits[displayCount][3] = 0;
    DisplayDigits[displayCount][4] = 0;
    DisplayDigits[displayCount][5] = 0;
    DisplayDigitEnable[displayCount] = 0x03;
    DisplayDim[displayCount] = false;
  }

  // Turn off all lamp states
  for (int lampNibbleCounter=0; lampNibbleCounter<16; lampNibbleCounter++) {
    LampStates[lampNibbleCounter] = 0xFF;
    LampDim0[lampNibbleCounter] = 0x00;
    LampDim1[lampNibbleCounter] = 0x00;
  }

  for (int lampFlashCount=0; lampFlashCount<64; lampFlashCount++) {
    LampFlashPeriod[lampFlashCount] = 0;
  }

  // Reset all the switch values 
  // (set them as closed so that if they're stuck they don't register as new events)
  byte switchCount;
  for (switchCount=0; switchCount<5; switchCount++) {
    SwitchesMinus2[switchCount] = 0xFF;
    SwitchesMinus1[switchCount] = 0xFF;
    SwitchesNow[switchCount] = 0xFF;
  }

  // Reset solenoid stack
  SolenoidStackFirst = 0;
  SolenoidStackLast = 0;

  // Reset switch stack
  SwitchStackFirst = 0;
  SwitchStackLast = 0;
}


byte BSOS_GetDipSwitches(byte index) {
  if (index<0 || index>3) return 0x00;
  return DipSwitches[index];
}


void BSOS_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, PlayfieldAndCabinetSwitch *s_gameSwitchArray) {
  NumGameSwitches = s_numSwitches;
  NumGamePrioritySwitches = s_numPrioritySwitches;
  GameSwitches = s_gameSwitchArray;
}


void BSOS_SetupGameLights(int s_numLights, PlayfieldLight *s_gameLightArray) {
  NumGameLights = s_numLights;
  GameLights = s_gameLightArray;
}

/*
void BSOS_SetContinuousSolenoids(byte continuousSolenoidMask = CONTSOL_DISABLE_FLIPPERS | CONTSOL_DISABLE_COIN_LOCKOUT) {
  CurrentSolenoidByte = (CurrentSolenoidByte&0x0F) | continuousSolenoidMask;
  BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}
*/


void BSOS_SetCoinLockout(boolean lockoutOn = false, byte solbit = CONTSOL_DISABLE_COIN_LOCKOUT) {
  if (lockoutOn) {
    CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
  } else {
    CurrentSolenoidByte = CurrentSolenoidByte | solbit;
  }
  BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}


void BSOS_SetDisableFlippers(boolean disableFlippers = true, byte solbit = CONTSOL_DISABLE_FLIPPERS) {
  if (disableFlippers) {
    CurrentSolenoidByte = CurrentSolenoidByte | solbit;
  } else {
    CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
  }
  
  BSOS_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}


byte BSOS_ReadContinuousSolenoids() {
  return BSOS_DataRead(ADDRESS_U11_B);
}


void BSOS_DisableSolenoidStack() {
  SolenoidStackEnabled = false;
}


void BSOS_EnableSolenoidStack() {
  SolenoidStackEnabled = true;
}



void BSOS_CycleAllDisplays(unsigned long curTime) {
  int displayDigit = (curTime/250)%10;
  unsigned long value;
  value = displayDigit*111111;

  for (int count=0; count<5; count++) {
    BSOS_SetDisplay(count, value);
  }
}


void BSOS_PlaySound(byte soundByte) {

  byte oldSolenoidControlByte, soundControlByte;

  // mask further zero-crossing interrupts during this 
  InsideZeroCrossingInterrupt += 1;

  // Get the current value of U11:PortB
  oldSolenoidControlByte = BSOS_DataRead(ADDRESS_U11_B);
  soundControlByte = oldSolenoidControlByte; 
  
  // Mask off momentary solenoids
  soundControlByte &= 0xF0;
  // Add in lower nibble
  soundControlByte |= (soundByte&0x0F);
  // put the new byte on U11:PortB
  BSOS_DataWrite(ADDRESS_U11_B, soundControlByte);
  
  // Strobe sound latch
  BSOS_DataWrite(ADDRESS_U11_B_CONTROL, BSOS_DataRead(ADDRESS_U11_B_CONTROL)|0x04);

  // wait 200 microseconds
  delayMicroseconds(200);

  // remove lower nibble
  soundControlByte &= 0xF0;
  // Put upper nibble on lines
  soundControlByte |= (soundByte/16);
  // put the new byte on U11:PortB
  BSOS_DataWrite(ADDRESS_U11_B, soundControlByte);

  // wait 200 microseconds
  delayMicroseconds(200);

  // Turn off sound latch
  BSOS_DataWrite(ADDRESS_U11_B_CONTROL, BSOS_DataRead(ADDRESS_U11_B_CONTROL)&0xF7);

  InsideZeroCrossingInterrupt -= 1;
}

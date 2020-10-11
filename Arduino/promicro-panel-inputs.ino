#include <Key.h>
#include <Keypad.h>
#include <Joystick.h>
#include <Keyboard.h>

// Based on my own circuitboard design.
// Row pins are 1,0,2,3,4
// col pins are 5,6,7,8
// 9,10 for encoder 1
// 14,16 for encoder 2
// 15, a0(18) for encoder 3
// a1, a2, a3(19, 20, 21) for 3 analog inputs
#define BAUD_RATE 9600

// With this flag, "HatSwitch" signals will send serial data
// to the PC, instead of emulating a joystick button.
// This is used for trim wheel input: we use serial data to tell SimConnect
// client that we want to trim up/down, and the client sends the command
// to the game.
// See "SetHatSwitch" see how it's used.
#define SERIAL_SIGNAL

#define HAT_UP 0
#define HAT_DOWN 180 
#define HAT_LEFT 270
#define HAT_RIGHT 90

#define ENABLE_PULLUPS
#define NUMROTARIES 3
#define NUMBUTTONS 26
#define NUMROWS 5
#define NUMCOLS 4
#define DUMMY 33
#define NUM_AXIS 3
byte axes[NUM_AXIS] = {1,2,3};

// byte buttons[NUMROWS][NUMCOLS] = {
//   {'q','w','e','r','t','y'},
//   {'u','i','o','p','a','s'},
//   {'d','f','g','h','j','k'},
//   {'l','z','x','c',KEY_PAGE_UP,KEY_PAGE_DOWN},
//   {'n','m','b','.',' ','='},
// };


byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3},
  {4,5,6,7},
  {8,9,10,11},
  {12,13,14,15},
  {16,17,18,19},
};


// Rows are 1,0,2,3,4
// cols are 5,6,7,8.
byte rowPins[NUMROWS] = {1,0,2,3,4}; 
byte colPins[NUMCOLS] = {5,6,7,8}; 

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;
  int cwchar;
  // If is not -1, the rotary would be used as a hatch switch.
  int hatswitch;
  // degree angle of hatswitch
  int ccwdeg;
  int cwdeg;
  volatile unsigned char state;
};

rotariesdef rotaries[NUMROTARIES] {
  {9,10,20,21,-1,0,180,0},
  {14,16,22,23,-1,0,180,0},
  // Hatswitch > 0, and send Serial signals for the buttons. 
  // See SetHatSwitch
  {15,18,24,25,1,0,180,0},
};


#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#ifdef HALF_STEP
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6
const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif



Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS); 

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, /* total buttons=*/NUMBUTTONS,
  /*NUM HAT PAD=*/ 0,
  /*XYZ RxRyRz*/ false, false, true, false, false, false,
  /*rudder, throttle, accelerator, brake, steering*/true, true, false, false, false);

void setup() {
//  Serial.begin(9600);
  Serial.begin(BAUD_RATE);
  // put your setup code here, to run once:
  Joystick.begin();
  // Keyboard.begin();
  rotary_init();
}

void loop() { 
  CheckAllEncoders();
  CheckAllButtonsJoystick();
  CheckAllAxes();
}


// void CheckAllButtons(void) {
//   char key = buttbx.getKey();
//   if (key != NO_KEY)  {
//     // Keyboard.press(KEY_LEFT_CTRL);
//     // Keyboard.press(KEY_LEFT_ALT);
//     Keyboard.press(key);
//     delay(150);
//     Keyboard.releaseAll();
//   }//Keyboard.write(key);
// }

void CheckAllButtonsJoystick(void) {
  if (buttbx.getKeys()) {
    for (int i=0; i<LIST_MAX; i++) {
      if ( buttbx.key[i].stateChanged ) {
        switch (buttbx.key[i].kstate) {  
          case PRESSED:
          case HOLD:
            Joystick.setButton(buttbx.key[i].kchar, 1);
            break;
          case RELEASED:
          case IDLE:
            Joystick.setButton(buttbx.key[i].kchar, 0);
            break;
        }
      }   
    }
  }
}


void rotary_init() {
  for (int i=0;i<NUMROTARIES;i++) {
    pinMode(rotaries[i].pin1, INPUT);
    pinMode(rotaries[i].pin2, INPUT);
    #ifdef ENABLE_PULLUPS
      digitalWrite(rotaries[i].pin1, HIGH);
      digitalWrite(rotaries[i].pin2, HIGH);
    #endif
  }
}


unsigned char rotary_process(int _i) {
  unsigned char pinstate = (digitalRead(rotaries[_i].pin2) << 1) | digitalRead(rotaries[_i].pin1);
  rotaries[_i].state = ttable[rotaries[_i].state & 0xf][pinstate];
  return (rotaries[_i].state & 0x30);
}

void CheckAllEncoders(void) {
  for (int i=0;i<NUMROTARIES;i++) {
    unsigned char result = rotary_process(i);
    if (result == DIR_CCW) {
      if (rotaries[i].hatswitch < 0){
        Joystick.setButton(rotaries[i].ccwchar, 1); delay(50); Joystick.setButton(rotaries[i].ccwchar, 0);
      } else {
        SetHatSwitch(rotaries[i].ccwdeg);
      }
    };
    if (result == DIR_CW) {
      if (rotaries[i].hatswitch < 0){
        Joystick.setButton(rotaries[i].cwchar, 1); delay(50); Joystick.setButton(rotaries[i].cwchar, 0);
      } else {
        SetHatSwitch(rotaries[i].cwdeg);
      }
    };
  }
}

void SetHatSwitch(int deg) {
#ifdef SERIAL_SIGNAL
  if (deg == 0) {
    Serial.write(1);
  } else {
    Serial.write(2);
  }
#else
  Joystick.setHatSwitch(rotaries[i].hatswitch, deg); delay(50); Joystick.setHatSwitch(rotaries[i].hatswitch, JOYSTICK_HATSWITCH_RELEASE); 
#endif
}

// Channel is the analog channel ID(0,1,2,3)
//analog read with moving average filter  ===================================================================================================
int filter(int channel) 
{
  unsigned int sum=0;
  unsigned int i=0;

  for (i=0; i<5; i++)
  {
    sum = sum + analogRead(channel);
  }
  
  sum = sum / 5;
  return(sum);
}

void CheckAllAxes(void) {
  Joystick.setThrottle(analogRead(1));
  Joystick.setRudder(analogRead(2));
  Joystick.setZAxis(analogRead(3));
}

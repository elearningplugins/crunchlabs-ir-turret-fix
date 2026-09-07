//////////////////////////////////////////////////
              //  LICENSE  //
//////////////////////////////////////////////////
#pragma region LICENSE
/*
  ************************************************************************************
  * MIT License
  *
  * Copyright (c) 2025 Crunchlabs LLC (IRTurret Control Code)
  * Copyright (c) 2020-2022 Armin Joachimsmeyer (IRremote Library)

  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is furnished
  * to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
  * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  *
  ************************************************************************************
*/
#pragma endregion LICENSE

//////////////////////////////////////////////////
              //  LIBRARIES  //
//////////////////////////////////////////////////
#pragma region LIBRARIES

#include <Arduino.h>
#include <Servo.h>
//DECODE_NEC must be defined BEFORE IRremote.hpp is included, or the library compiles its whole
//default protocol set instead of just NEC - that wastes flash and lets other protocols' command
//bytes reach our handler. Check the boot banner: it should list NEC only.
#define DECODE_NEC
#include <IRremote.hpp>

#pragma endregion LIBRARIES

//////////////////////////////////////////////////
               //  IR CODES  //
//////////////////////////////////////////////////
#pragma region IR CODES
/*
** if you want to add other remotes (as long as they're on the same protocol above):
** press the desired button and look for a hex code similar to those below (ex: 0x11)
** then add a new line to #define newCmdName 0x11,
** and add a case to the switch statement like case newCmdName: 
** this will let you add new functions to buttons on other remotes!
** the best remotes to try are cheap LED remotes, some TV remotes, and some garage door openers
*/

//defines the specific command code for each button on the remote
#define left 0x8
#define right 0x5A
#define up 0x18
#define down 0x52
#define ok 0x1C
#define cmd1 0x45
#define cmd2 0x46
#define cmd3 0x47
#define cmd4 0x44
#define cmd5 0x40
#define cmd6 0x43
#define cmd7 0x7
#define cmd8 0x15
#define cmd9 0x9
#define cmd0 0x19
#define star 0x16
#define hashtag 0xD

#pragma endregion IR CODES

//////////////////////////////////////////////////
          //  PINS AND PARAMETERS  //
//////////////////////////////////////////////////
#pragma region PINS AND PARAMS
//this is where we store global variables!
Servo yawServo; //names the servo responsible for YAW rotation, 360 spin around the base
Servo pitchServo; //names the servo responsible for PITCH rotation, up and down tilt
Servo rollServo; //names the servo responsible for ROLL rotation, spins the barrel to fire darts

int yawServoVal = 90; //initialize variables to store the current value of each servo
int pitchServoVal = 100;
int rollServoVal = 90;

int pitchMoveSpeed = 6; //this variable is the angle added to the pitch servo to control how quickly the PITCH servo moves - raise for faster tilt, lower for finer aim
int yawMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the YAW servo motor. It is added or subtracted from the yawStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Try values between 10 and 90;
int yawStopSpeed = 90; //value to stop the yaw motor - keep this at 90
int rollMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the ROLL servo motor. It is added or subtracted from the rollStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Keep this at 90 for best performance / highest torque from the roll motor when firing.
int rollStopSpeed = 90; //value to stop the roll motor - keep this at 90

//safe limits for the live tuning keys - a negative or absurd run time would be handed to delay(),
//which takes an unsigned long, so a negative value wraps to roughly 49 days of full speed rotation
#define ROLL_TIME_MIN 60    // ms, shorter than this cannot move a chamber
#define ROLL_TIME_MAX 600   // ms, longer than this overshoots wildly
#define ROLL_STEP_MIN -40   // ms per shot
#define ROLL_STEP_MAX 40

int rollStep = 10; //ms added to rollPrecision for each dart already fired - positive rolls LONGER as the magazine empties
int dartsFired = 0; //how many shots since the last reload, so the ramp knows where it is in the magazine

int yawPrecision = 70; // this variable represents the time in milliseconds that the YAW motor will remain at it's set movement speed. Try values between 50 and 500 to start (500 milliseconds = 1/2 second)
int rollPrecision = 240; // this variable represents the time in milliseconds that the ROLL motor with remain at it's set movement speed. If this ROLL motor is spinning more or less than 1/6th of a rotation when firing a single dart (one call of the fire(); command) you can try adjusting this value down or up slightly, but it should remain around the stock value (160ish) for best results.

int pitchMax = 150; // this sets the maximum angle of the pitch servo to prevent it from crashing, it should remain below 180, and be greater than the pitchMin
int pitchMin = 33; // this sets the minimum angle of the pitch servo to prevent it from crashing, it should remain above 0, and be less than the pitchMax

void shakeHeadYes(int moves = 3); //function prototypes for shakeHeadYes and No for proper compiling
void shakeHeadNo(int moves = 3);
void handleCommand(int command, bool isRepeat); //function prototype for the command handler used by loop()
void flushIR(); //function prototype for the IR buffer flush used after long blocking moves
int constrainRollTime(int ms); //function prototype for the roll time clamp
void handleSerial(); //DEBUG BUILD: lets a computer drive the turret over USB
void printStatus(); //DEBUG BUILD: dumps the turret's current state
void timedFire(); //DEBUG BUILD: fires one dart and reports the commanded run time (not actual rotation)
void spinRoll(int ms); //DEBUG BUILD: runs the roll servo for a measured time so its real speed can be calibrated
#pragma endregion PINS AND PARAMS

//////////////////////////////////////////////////
              //  S E T U P  //
//////////////////////////////////////////////////
#pragma region SETUP
void setup() { //this is our setup function - it runs once on start up, and is basically where we get everything "set up"
    Serial.begin(115200); // initializes the Serial communication between the computer and the microcontroller

    yawServo.attach(10); //attach YAW servo to pin 10
    pitchServo.attach(11); //attach PITCH servo to pin 11
    rollServo.attach(12); //attach ROLL servo to pin 12

    // Just to know which program is running on my microcontroller
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(9, ENABLE_LED_FEEDBACK);

    Serial.print(F("Ready to receive IR signals of protocols: "));
    printActiveIRProtocols(&Serial);
    Serial.println(F("at pin 9"));

    homeServos(); //set servo motors to home position

    Serial.println(F("READY - serial tuning active, send ? for keys"));
}
#pragma endregion SETUP

//////////////////////////////////////////////////
               //  L O O P  //
//////////////////////////////////////////////////
#pragma region LOOP

void loop() {

    handleSerial(); //DEBUG BUILD: check for commands typed over USB before looking at IR

    /*
    * Check if received data is available and if yes, try to decode it.
    */
    if (IrReceiver.decode()) {

        /*
        * Print a short summary of received data
        */
        //IrReceiver.printIRResultShort(&Serial);
        //IrReceiver.printIRSendUsage(&Serial);
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) { //command garbled or not recognized
            Serial.println(F("Received noise or an unknown (or not yet enabled) protocol - if you wish to add this command, define it at the top of the file with the hex code printed below (ex: 0x8)"));
            // We have an unknown protocol here, print more info
            IrReceiver.printIRResultRawFormatted(&Serial, true);
        }
        //Serial.println();

        int command = IrReceiver.decodedIRData.command; //store the button that was pressed
        bool isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT); //read the repeat flag before resume() can overwrite it

        /*
        * !!!Important!!! Enable receiving of the next value,
        * since receiving has stopped after the end of the current received data packet.
        */
        IrReceiver.resume(); // Enable receiving of the next value

        /*
        * Finally, check the received data and perform actions according to the received command
        */
        handleCommand(command, isRepeat); //all of the button handling now lives in handleCommand
    }
    delay(5);
}

#pragma endregion LOOP

//////////////////////////////////////////////////
              //  HELPERS  //
//////////////////////////////////////////////////
#pragma region HELPERS

int constrainRollTime(int ms) { //keeps any run time we hand to delay() inside a sane, non negative range
    if (ms < ROLL_TIME_MIN) {
        return ROLL_TIME_MIN;
    }
    if (ms > ROLL_TIME_MAX) {
        return ROLL_TIME_MAX;
    }
    return ms;
}

void flushIR() { //throws away any button that arrived while a long move was blocking
    if (IrReceiver.decode()) {
        IrReceiver.resume();
    }
}

void handleCommand(int command, bool isRepeat) {
    switch (command) {

        case up: //pitch up
          upMove(isRepeat ? 2 : 1); //a held button covers twice the ground
          break;

        case down: //pitch down
          downMove(isRepeat ? 2 : 1);
          break;

        case left: //fast counterclockwise rotation
          leftMove(isRepeat ? 2 : 1);
          break;

        case right: //fast clockwise rotation
          rightMove(isRepeat ? 2 : 1);
          break;

        case ok: //fire a single dart
          fire();
          break;

        case star: //fire all six
          fireAll();
          break;

        case hashtag: //tell the turret you reloaded, so the firing ramp starts over
          dartsFired = 0;
          Serial.println(F("RELOADED"));
          break;

        case cmd1:
          shakeHeadYes(3);
          break;

        case cmd2:
          shakeHeadNo(3);
          break;

    }
}

#pragma endregion HELPERS


//////////////////////////////////////////////////
          //  DEBUG / SERIAL CONTROL  //
//////////////////////////////////////////////////
#pragma region DEBUG
/*
** This region exists only for debugging over USB. It does not touch the IR path -
** the remote keeps working exactly as before. Delete this region to get the clean sketch back.
*/

void printStatus() {
    Serial.print(F("STATUS pitch="));
    Serial.print(pitchServoVal);
    Serial.print(F(" pitchMin="));
    Serial.print(pitchMin);
    Serial.print(F(" pitchMax="));
    Serial.print(pitchMax);
    Serial.print(F(" pitchStep="));
    Serial.print(pitchMoveSpeed);
    Serial.print(F(" yawPrec="));
    Serial.print(yawPrecision);
    Serial.print(F(" rollPrec="));
    Serial.print(rollPrecision);
    Serial.print(F(" rollStep="));
    Serial.print(rollStep);
    Serial.print(F(" dartsFired="));
    Serial.print(dartsFired);
    Serial.print(F(" uptime="));
    Serial.println(millis());
}

void timedFire() { //fires one dart and reports the commanded run time for this shot
    //NOTE: this measures how long the code ran the servo, NOT how far the barrel actually turned.
    //A continuous rotation servo reports no position, so a stalled barrel produces the same numbers
    //as a healthy one. Only your eyes can tell whether the chamber advanced.
    int commanded = constrainRollTime(rollPrecision + (rollStep * dartsFired));
    unsigned long t0 = millis();
    fire();
    unsigned long t1 = millis();
    Serial.print(F("FIRED elapsed="));
    Serial.print(t1 - t0);
    Serial.print(F(" commanded="));
    Serial.print(commanded);
    Serial.println(F(" (commanded time only - watch the barrel to see if it advanced)"));
}

void spinRoll(int ms) { //runs the barrel at full speed for exactly ms, then stops - used to measure real rotation under load
    if (ms < 1 || ms > ROLL_TIME_MAX * 6) { //never hand delay() a negative or runaway duration
        Serial.println(F("SPIN refused - duration out of range"));
        return;
    }
    Serial.print(F("SPIN start ms="));
    Serial.println(ms);
    rollServo.write(rollStopSpeed + rollMoveSpeed);
    delay(ms);
    rollServo.write(rollStopSpeed);
    Serial.println(F("SPIN done - how far did the barrel actually turn?"));
    flushIR();
}

void handleSerial() {
    if (!Serial.available()) {
        return;
    }
    char c = Serial.read();
    int cmd = 0;

    switch (c) {
        case 'u': cmd = up;      break;
        case 'd': cmd = down;    break;
        case 'l': cmd = left;    break;
        case 'r': cmd = right;   break;
        case 'f': cmd = ok;      break; //fire one dart
        case 'a': cmd = star;    break; //fire all six
        case 'x': cmd = hashtag; break; //reloaded - reset the firing ramp

        case 's': printStatus(); return; //report state without moving anything
        case 't': timedFire();   return; //timed single shot

        //live tuning of rollPrecision, so the barrel can be calibrated without re-uploading
        case '[': rollPrecision = constrainRollTime(rollPrecision - 10); Serial.print(F("rollPrecision=")); Serial.println(rollPrecision); return;
        case ']': rollPrecision = constrainRollTime(rollPrecision + 10); Serial.print(F("rollPrecision=")); Serial.println(rollPrecision); return;
        case '<': rollPrecision = constrainRollTime(rollPrecision - 2); Serial.print(F("rollPrecision=")); Serial.println(rollPrecision); return;
        case '>': rollPrecision = constrainRollTime(rollPrecision + 2); Serial.print(F("rollPrecision=")); Serial.println(rollPrecision); return;

        //ramp tuning: k/K change how much each successive shot is lengthened, z resets the magazine counter
        case 'k': rollStep = max(ROLL_STEP_MIN, rollStep - 2); Serial.print(F("rollStep=")); Serial.println(rollStep); return;
        case 'K': rollStep = min(ROLL_STEP_MAX, rollStep + 2); Serial.print(F("rollStep=")); Serial.println(rollStep); return;
        case 'z': dartsFired = 0; Serial.println(F("magazine counter reset")); return;

        //calibration spins: measure how far the barrel really goes under load
        case 'R': spinRoll(1000);              return; //one second of rotation
        case 'F': spinRoll(constrainRollTime(rollPrecision) * 6); return; //what the code thinks is one full turn
        case 'H': spinRoll(constrainRollTime(rollPrecision)); return; //what the code thinks is one chamber

        case 'T': { //walk pitch to the top and report where it actually stopped
            for (int i = 0; i < 60; i++) { upMove(1); }
            Serial.print(F("PITCH TOP pitch="));
            Serial.print(pitchServoVal);
            Serial.print(F(" pitchMax="));
            Serial.println(pitchMax);
            return;
        }
        case 'B': { //walk pitch to the bottom and report where it actually stopped
            for (int i = 0; i < 60; i++) { downMove(1); }
            Serial.print(F("PITCH BOTTOM pitch="));
            Serial.print(pitchServoVal);
            Serial.print(F(" pitchMin="));
            Serial.println(pitchMin);
            return;
        }

        case '?': //help
            Serial.println(F("KEYS u/d/l/r=aim f=fire a=fireAll x=reloaded s=status t=fire T=top B=bottom [/]=rollPrecision+-10 </>=rollPrecision+-2 k/K=rollStep+-2 z=reset mag R=spin1s H=one chamber F=full turn"));
            return;

        default: return; //ignore newlines and anything unmapped
    }

    handleCommand(cmd, false);
    Serial.print(F("SERIAL cmd="));
    Serial.print(c);
    Serial.print(F(" pitch="));
    Serial.println(pitchServoVal);
}
#pragma endregion DEBUG

//////////////////////////////////////////////////
               // FUNCTIONS  //
//////////////////////////////////////////////////
#pragma region FUNCTIONS

void leftMove(int moves){ // function to move left
    for (int i = 0; i < moves; i++){
        yawServo.write(yawStopSpeed + yawMoveSpeed); // adding the servo speed = 180 (full counterclockwise rotation speed)
        delay(yawPrecision); // stay rotating for a certain number of milliseconds
        yawServo.write(yawStopSpeed); // stop rotating
        delay(5); //delay for smoothness
        //Serial.println("LEFT");
  }

}

void rightMove(int moves){ // function to move right
  for (int i = 0; i < moves; i++){
      yawServo.write(yawStopSpeed - yawMoveSpeed); //subtracting the servo speed = 0 (full clockwise rotation speed)
      delay(yawPrecision);
      yawServo.write(yawStopSpeed);
      delay(5);
      //Serial.println("RIGHT");
  }
}

void upMove(int moves){ // function to tilt up
  for (int i = 0; i < moves; i++){
        //clamp to pitchMax instead of discarding the step, so a big pitchMoveSpeed still reaches the top
        pitchServoVal = min(pitchServoVal + pitchMoveSpeed, pitchMax);
        pitchServo.write(pitchServoVal);
        //Serial.println("UP");
  }
}

void downMove (int moves){ // function to tilt down
  for (int i = 0; i < moves; i++){
      //clamp to pitchMin instead of discarding the step, so a big pitchMoveSpeed still reaches the bottom
      pitchServoVal = max(pitchServoVal - pitchMoveSpeed, pitchMin);
      pitchServo.write(pitchServoVal);
      //Serial.println("DOWN");
  }
}

void fire() { //function for firing a single dart
    int thisShot = rollPrecision + (rollStep * dartsFired); //ramp the time as the magazine empties
    thisShot = constrainRollTime(thisShot); //never hand delay() a negative or runaway value
    rollServo.write(rollStopSpeed + rollMoveSpeed);//start rotating the servo
    delay(thisShot);//time for approximately 60 degrees of rotation
    rollServo.write(rollStopSpeed);//stop rotating the servo
    delay(5); //delay for smoothness
    if (dartsFired < 6) { //stop climbing once the magazine is spent
      dartsFired++;
    }
    Serial.print(F("SHOT n="));
    Serial.print(dartsFired);
    Serial.print(F(" ms="));
    Serial.println(thisShot);
    flushIR(); //drop anything pressed while the barrel was turning
}

void fireAll() { //function to fire all 6 darts at once
    rollServo.write(rollStopSpeed + rollMoveSpeed);//start rotating the servo
    delay(constrainRollTime(rollPrecision) * 6); //time for 360 degrees of rotation
    rollServo.write(rollStopSpeed);//stop rotating the servo
    delay(5); // delay for smoothness
    //Serial.println("FIRING ALL");
    dartsFired = 0;
    flushIR(); //drop anything pressed during the ~1 second it takes to empty the barrel
}

void homeServos(){ // sends servos to home positions
    yawServo.write(yawStopSpeed); //setup YAW servo to be STOPPED (90)
    delay(20);
    rollServo.write(rollStopSpeed); //setup ROLL servo to be STOPPED (90)
    delay(100);
    pitchServo.write(100); //set PITCH servo to 100 degree position
    delay(100);
    pitchServoVal = 100; // store the pitch servo value
    Serial.println("HOMING");
}

void shakeHeadYes(int moves) { //sets the default number of nods to 3, but you can pass in whatever number of nods you want
    //Serial.println("YES");

    int originalPitchVal = pitchServoVal; //remember the aim so the nod can hand it back afterwards

    if ((pitchMax - pitchServoVal) < 15){
      pitchServoVal = pitchServoVal - 15;
    }else if ((pitchServoVal - pitchMin) < 15){
      pitchServoVal = pitchServoVal + 15;
    }
    pitchServo.write(pitchServoVal);

    int startAngle = pitchServoVal; // Current position of the pitch servo
    int nodAngle = startAngle + 15; // Angle for nodding motion

    for (int i = 0; i < moves; i++) { // Repeat nodding motion three times
        // Nod up
        for (int angle = startAngle; angle <= nodAngle; angle++) {
            pitchServo.write(angle);
            delay(7); // Adjust delay for smoother motion
        }
        delay(50); // Pause at nodding position
        // Nod down
        for (int angle = nodAngle; angle >= startAngle; angle--) {
            pitchServo.write(angle);
            delay(7); // Adjust delay for smoother motion
        }
        delay(50); // Pause at starting position
    }

    pitchServoVal = originalPitchVal; //put the aim back where the nod found it
    pitchServo.write(pitchServoVal);

    flushIR(); //drop anything pressed during the nod
}

void shakeHeadNo(int moves) {
    //Serial.println("NO");

    for (int i = 0; i < moves; i++) { // Repeat nodding motion three times
        // rotate right, stop, then rotate left, stop
        yawServo.write(140);
        delay(190); // Adjust delay for smoother motion
        yawServo.write(yawStopSpeed);
        delay(50);
        yawServo.write(40);
        delay(190); // Adjust delay for smoother motion
        yawServo.write(yawStopSpeed);
        delay(50); // Pause at starting position
    }
    flushIR(); //drop anything pressed during the head shake
}
#pragma endregion FUNCTIONS

//////////////////////////////////////////////////
               //  END CODE  //
//////////////////////////////////////////////////

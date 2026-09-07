// Tests for IRTurret_FixedFiring.ino, the passcode free sketch most people use.
// Build:  python3 generate.py && g++ -std=c++17 -I. community_tests.cpp stubs.cpp -o ctests && ./ctests
#include "stubs.h"
#include "community_sketch.cpp"

static int failures = 0, checks = 0;
#define CHECK(cond, msg) do{ checks++; if(!(cond)){ failures++; printf("  FAIL: %s\n", msg);} }while(0)

static void press(int cmd, bool repeat=false){
    IrReceiver.decodedIRData.command = cmd;
    IrReceiver.decodedIRData.flags = repeat ? IRDATA_FLAGS_IS_REPEAT : 0;
    handleCommand(cmd, repeat);
}
static void reset_state(){
    pitchServoVal = 100; dartsFired = 0;
    rollPrecision = 240; rollStep = 10;
    pitchServo.writes.clear(); yawServo.writes.clear(); rollServo.writes.clear();
    pitchServo.attached = yawServo.attached = rollServo.attached = true;
    IrReceiver.pending = 0; Serial.inbuf.clear();
}
static void serial_key(char c){ Serial.inbuf.clear(); Serial.inbuf += c; handleSerial(); }

int main(){
    printf("=== 1. remote buttons ===\n");
    reset_state(); press(ok);
    CHECK(!rollServo.writes.empty(), "OK must fire");
    reset_state(); press(up);
    CHECK(pitchServoVal > 100, "up must raise pitch");
    reset_state(); press(down);
    CHECK(pitchServoVal < 100, "down must lower pitch");
    reset_state(); press(star);
    CHECK(dartsFired == 0, "star (fireAll) must reset the magazine counter");
    reset_state(); dartsFired = 4; press(hashtag);
    CHECK(dartsFired == 0, "hashtag must reset the magazine counter after reloading");

    printf("=== 2. serial keys map to the right actions ===\n");
    // regression: 'a' once mapped to hashtag, so fireAll was unreachable over serial
    reset_state(); dartsFired = 3;
    unsigned long t0 = g_virtual_ms; serial_key('a');
    unsigned long fireAllMs = g_virtual_ms - t0;
    CHECK(fireAllMs >= (unsigned long)rollPrecision * 6, "'a' must run fireAll, not a counter reset");
    CHECK(dartsFired == 0, "'a' (fireAll) must reset the magazine counter");

    reset_state(); dartsFired = 5; serial_key('x');
    CHECK(dartsFired == 0, "'x' must reset the magazine counter");

    reset_state();
    t0 = g_virtual_ms; serial_key('f');
    CHECK(g_virtual_ms - t0 >= (unsigned long)rollPrecision, "'f' must fire one dart");
    CHECK(dartsFired == 1, "'f' must advance the magazine counter");

    printf("=== 3. roll time is always safe for delay() ===\n");
    {
        int bad = 0;
        for(int p = -2000; p <= 2000; p += 7)
        for(int st = -200; st <= 200; st += 13)
        for(int n = 0; n <= 12; n++){
            int v = constrainRollTime(p + st * n);
            if(v < ROLL_TIME_MIN || v > ROLL_TIME_MAX) bad++;
        }
        CHECK(bad == 0, "constrainRollTime must never return a negative or runaway value");
    }
    {   // drive the tuning keys past their limits and make sure firing stays sane
        reset_state();
        for(int i=0;i<200;i++) serial_key('[');
        CHECK(rollPrecision >= ROLL_TIME_MIN, "'[' spam must not drive rollPrecision below the floor");
        for(int i=0;i<200;i++) serial_key('k');
        CHECK(rollStep >= ROLL_STEP_MIN, "'k' spam must not drive rollStep below the floor");
        dartsFired = 0;
        unsigned long worst = 0;
        for(int n=0;n<12;n++){
            unsigned long a = g_virtual_ms; fire();
            unsigned long d = g_virtual_ms - a;
            if(d > worst) worst = d;
        }
        CHECK(worst <= (unsigned long)ROLL_TIME_MAX + 50, "no shot may exceed the clamp even after key spam");
        printf("  worst shot after spamming the keys: %lu ms\n", worst);

        reset_state();
        for(int i=0;i<200;i++) serial_key(']');
        CHECK(rollPrecision <= ROLL_TIME_MAX, "']' spam must not drive rollPrecision above the ceiling");
        for(int i=0;i<200;i++) serial_key('K');
        CHECK(rollStep <= ROLL_STEP_MAX, "'K' spam must not drive rollStep above the ceiling");
    }

    printf("=== 4. pitch limits ===\n");
    for(int spd=1; spd<=30; spd++){
        pitchMoveSpeed = spd; reset_state();
        for(int i=0;i<200;i++) press(up);
        if(pitchServoVal != pitchMax){ printf("  FAIL: speed=%d topped at %d\n", spd, pitchServoVal); failures++; } else checks++;
        reset_state();
        for(int i=0;i<200;i++) press(down);
        if(pitchServoVal != pitchMin){ printf("  FAIL: speed=%d bottomed at %d\n", spd, pitchServoVal); failures++; } else checks++;
    }
    pitchMoveSpeed = 6;

    printf("=== 5. firing ramp ===\n");
    reset_state();
    { int times[6];
      for(int n=0;n<6;n++){ unsigned long a=g_virtual_ms; fire(); times[n]=(int)(g_virtual_ms-a); }
      printf("  shot times:"); for(int n=0;n<6;n++) printf(" %d", times[n]); printf("\n");
      int bad=0; for(int n=1;n<6;n++) if(times[n]-times[n-1] != rollStep) bad++;
      CHECK(bad==0, "each shot must be rollStep longer than the last");
      for(int i=0;i<10;i++) fire();
      CHECK(dartsFired==6, "magazine counter must cap at 6"); }

    printf("=== 6. nod must not move the aim ===\n");
    { int worst=0;
      for(int start=pitchMin; start<=pitchMax; start++){
          reset_state(); pitchServoVal=start; shakeHeadYes(2);
          int d=pitchServoVal-start; if(abs(d)>abs(worst)) worst=d; }
      CHECK(worst==0, "shakeHeadYes must restore the aim"); }

    printf("=== 7. tuned defaults ===\n");
    reset_state();
    CHECK(rollPrecision==240, "rollPrecision default must be 240");
    CHECK(rollStep==10, "rollStep default must be 10");
    CHECK(pitchMin==33 && pitchMax==150, "pitch limits must be 33 and 150");

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

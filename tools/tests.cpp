#include "stubs.h"
// pull in the sketch under test
#include "sketch.cpp"

static int failures = 0, checks = 0;
#define CHECK(cond, msg) do{ checks++; if(!(cond)){ failures++; \
    printf("  FAIL: %s\n", msg);} }while(0)

static void press(int cmd, bool repeat=false){
    IrReceiver.decodedIRData.command = cmd;
    IrReceiver.decodedIRData.flags = repeat ? IRDATA_FLAGS_IS_REPEAT : 0;
    handleCommand(cmd, repeat);
}
static void reset_state(){
    passcodeEntered = false; passcode[0]='\0';
    pitchServoVal = 100; pitchServo.writes.clear();
    yawServo.writes.clear(); rollServo.writes.clear();
    pitchServo.attached = yawServo.attached = rollServo.attached = true;
    IrReceiver.pending = 0;
}
static void unlock(){ press(cmd2); press(cmd4); press(cmd6); press(cmd8); }

// every pitch write must stay inside the mechanical limits
static void assert_pitch_in_range(const char* ctx){
    for(int v : pitchServo.writes){
        if(v < pitchMin || v > pitchMax){
            printf("  FAIL: %s wrote pitch=%d, outside [%d,%d]\n", ctx, v, pitchMin, pitchMax);
            failures++; return;
        }
    }
    checks++;
}

int main(){
    printf("=== 1. passcode state machine ===\n");
    reset_state();
    unlock();
    CHECK(passcodeEntered, "correct passcode 2468 should unlock");

    reset_state();
    press(cmd1); press(cmd1); press(cmd1); press(cmd1);
    CHECK(!passcodeEntered, "wrong passcode 1111 must stay locked");
    CHECK(strlen(passcode)==0, "buffer must reset after a wrong attempt");

    reset_state();
    press(cmd2); press(cmd4); press(cmd6); press(cmd9);
    CHECK(!passcodeEntered, "near-miss 2469 must stay locked");
    reset_state();
    press(cmd2); press(cmd4); press(cmd6); press(cmd9);
    unlock();
    CHECK(passcodeEntered, "correct entry after a failed attempt should unlock");

    printf("=== 2. buffer overflow safety ===\n");
    reset_state();
    for(int i=0;i<50;i++) press(cmd1);
    CHECK(strlen(passcode) <= PASSCODE_LENGTH, "buffer must never exceed PASSCODE_LENGTH");
    CHECK(!passcodeEntered, "50 wrong digits must not unlock");

    printf("=== 3. locked turret ignores movement ===\n");
    reset_state();
    press(up); press(down); press(left); press(right); press(ok);
    CHECK(pitchServo.writes.empty(), "pitch must not move while locked");
    CHECK(yawServo.writes.empty(),   "yaw must not move while locked");
    CHECK(rollServo.writes.empty(),  "roll must not fire while locked");

    printf("=== 4. repeat debounce while locked ===\n");
    reset_state();
    press(cmd2); press(cmd2, true); press(cmd2, true);
    CHECK(strlen(passcode)==1, "held digit must register only once");

    printf("=== 5. star re-locks and clears ===\n");
    reset_state(); unlock();
    press(cmd2);
    press(star);
    CHECK(!passcodeEntered, "star should re-lock");
    CHECK(strlen(passcode)==0, "locking should clear the buffer");

    printf("=== 6. pitch clamp reaches exact limits, every step size ===\n");
    for(int spd=1; spd<=40; spd++){
        pitchMoveSpeed = spd;
        reset_state(); unlock(); pitchServo.writes.clear();
        for(int i=0;i<200;i++) press(up);
        if(pitchServoVal != pitchMax){
            printf("  FAIL: speed=%d topped out at %d, expected %d\n", spd, pitchServoVal, pitchMax);
            failures++;
        } else checks++;
        assert_pitch_in_range("upMove");

        reset_state(); unlock(); pitchServo.writes.clear();
        for(int i=0;i<200;i++) press(down);
        if(pitchServoVal != pitchMin){
            printf("  FAIL: speed=%d bottomed out at %d, expected %d\n", spd, pitchServoVal, pitchMin);
            failures++;
        } else checks++;
        assert_pitch_in_range("downMove");
    }
    pitchMoveSpeed = 6;

    printf("=== 7. shakeHeadYes/No respect pitch limits from any start ===\n");
    for(int start=pitchMin; start<=pitchMax; start++){
        reset_state();
        pitchServoVal = start; pitchServo.writes.clear();
        shakeHeadYes(2);
        assert_pitch_in_range("shakeHeadYes");
    }

    printf("=== 8. does a nod leave the aim where it found it? ===\n");
    reset_state();
    pitchServoVal = 100;
    int before = pitchServoVal;
    shakeHeadYes(3);
    if(pitchServoVal != before)
        printf("  NOTE: aim drifted %d -> %d after a nod (%+d deg)\n",
               before, pitchServoVal, pitchServoVal-before);

    printf("=== 9. firing does not disturb the aim ===\n");
    reset_state(); unlock();
    pitchServoVal = 120; pitchServo.writes.clear();
    fire(); fireAll();
    CHECK(pitchServoVal==120, "fire/fireAll must not change pitch");


    printf("=== 10. does a nod shift the stored aim, at any start angle? ===\n");
    {
        int worst = 0, worst_at = -1;
        for(int start=pitchMin; start<=pitchMax; start++){
            reset_state(); pitchServoVal = start;
            shakeHeadYes(2);
            int drift = pitchServoVal - start;
            if(abs(drift) > abs(worst)){ worst = drift; worst_at = start; }
        }
        if(worst != 0)
            printf("  NOTE: worst aim drift %+d deg (starting at %d)\n", worst, worst_at);
        else printf("  no drift at any start angle\n");
    }

    printf("=== 11. randomized property test (200k random button sequences) ===\n");
    {
        unsigned seed = 12345;
        auto rnd = [&](int n){ seed = seed*1103515245u + 12345u; return (int)((seed>>16)%n); };
        int cmds[] = {up,down,left,right,ok,star,hashtag,
                      cmd0,cmd1,cmd2,cmd3,cmd4,cmd5,cmd6,cmd7,cmd8,cmd9,0xFF};
        int nc = sizeof(cmds)/sizeof(cmds[0]);
        reset_state();
        int viol_range=0, viol_buf=0, viol_locked=0;
        for(long i=0;i<200000;i++){
            if(rnd(500)==0) reset_state();
            pitchMoveSpeed = 1 + rnd(30);          // fuzz the tunables too
            bool wasLocked = !passcodeEntered;
            size_t lenBefore = strlen(passcode);
            pitchServo.writes.clear(); yawServo.writes.clear(); rollServo.writes.clear();
            press(cmds[rnd(nc)], rnd(4)==0);
            // a press that completes the code triggers a nod/shake, which is legitimate feedback
            bool attemptFinished = (lenBefore == (size_t)PASSCODE_LENGTH-1);
            // INVARIANT A: pitch never commanded outside its mechanical limits
            for(int v: pitchServo.writes) if(v<pitchMin||v>pitchMax) viol_range++;
            // INVARIANT B: passcode buffer never overflows
            if(strlen(passcode) > (size_t)PASSCODE_LENGTH) viol_buf++;
            // INVARIANT C: a locked turret never drives yaw or fires
            if(wasLocked && !rollServo.writes.empty()) viol_locked++;          // never fire while locked
            if(wasLocked && !passcodeEntered && !attemptFinished
               && !yawServo.writes.empty()) viol_locked++;                     // no aiming while locked
        }
        CHECK(viol_range==0,  "pitch commanded outside [pitchMin,pitchMax]");
        CHECK(viol_buf==0,    "passcode buffer overflowed");
        CHECK(viol_locked==0, "locked turret aimed or fired outside of feedback gestures");
        printf("  range=%d buffer=%d locked=%d violations\n", viol_range, viol_buf, viol_locked);
        pitchMoveSpeed = 6;
    }

    printf("=== 12. feedback gestures: correct=nod, wrong=shake ===\n");
    reset_state();
    yawServo.writes.clear(); pitchServo.writes.clear();
    unlock();
    CHECK(passcodeEntered, "2468 still unlocks after the swap");
    CHECK(!pitchServo.writes.empty(), "correct passcode should NOD (pitch moves)");
    CHECK(yawServo.writes.empty(), "correct passcode should not shake");
    reset_state();
    yawServo.writes.clear(); pitchServo.writes.clear();
    press(cmd1);press(cmd1);press(cmd1);press(cmd1);
    CHECK(!yawServo.writes.empty(), "wrong passcode should SHAKE (yaw moves)");
    CHECK(pitchServo.writes.empty(), "wrong passcode should not nod");


    printf("=== 13. nod drift is gone, from every start angle ===\n");
    {
        int worst=0, at=-1;
        for(int start=pitchMin; start<=pitchMax; start++){
            reset_state(); pitchServoVal=start;
            shakeHeadYes(2);
            int d = pitchServoVal-start;
            if(abs(d)>abs(worst)){ worst=d; at=start; }
        }
        CHECK(worst==0, "shakeHeadYes must restore the aim it started with");
        if(worst) printf("  worst drift %+d at start=%d\n", worst, at);
    }

    printf("=== 14. exhaustive: all 10000 four-digit codes ===\n");
    {
        int digits[10]={cmd0,cmd1,cmd2,cmd3,cmd4,cmd5,cmd6,cmd7,cmd8,cmd9};
        int unlocked=0, which=-1;
        for(int n=0;n<10000;n++){
            reset_state();
            int a=n/1000,b=(n/100)%10,c=(n/10)%10,d=n%10;
            press(digits[a]);press(digits[b]);press(digits[c]);press(digits[d]);
            if(passcodeEntered){ unlocked++; which=n; }
        }
        CHECK(unlocked==1, "exactly one 4-digit code may unlock");
        CHECK(which==2468, "the only unlocking code must be 2468");
        printf("  %d of 10000 codes unlocked (code %04d)\n", unlocked, which);
    }

    printf("=== 15. monotonicity: up never lowers, down never raises ===\n");
    {
        int bad=0;
        for(int spd=1; spd<=30; spd++){
            pitchMoveSpeed=spd;
            reset_state(); unlock();
            for(int i=0;i<80;i++){ int b=pitchServoVal; press(up);   if(pitchServoVal<b) bad++; }
            for(int i=0;i<80;i++){ int b=pitchServoVal; press(down); if(pitchServoVal>b) bad++; }
        }
        pitchMoveSpeed=6;
        CHECK(bad==0, "pitch moved the wrong direction");
    }

    printf("=== 16. reachability from any angle, any step size ===\n");
    {
        int bad=0;
        for(int spd=1; spd<=30; spd+=3)
        for(int start=pitchMin; start<=pitchMax; start+=7){
            pitchMoveSpeed=spd;
            reset_state(); unlock(); pitchServoVal=start;
            for(int i=0;i<200;i++) press(up);
            if(pitchServoVal!=pitchMax) bad++;
            for(int i=0;i<400;i++) press(down);
            if(pitchServoVal!=pitchMin) bad++;
        }
        pitchMoveSpeed=6;
        CHECK(bad==0, "both limits reachable from any start at any step size");
    }

    printf("=== 17. lock/unlock idempotence ===\n");
    reset_state(); unlock();
    for(int i=0;i<10;i++) press(star);
    CHECK(!passcodeEntered, "repeated star stays locked");
    unlock();
    CHECK(passcodeEntered, "still unlockable after repeated locking");
    for(int i=0;i<10;i++) unlock();
    CHECK(passcodeEntered, "re-entering the code while unlocked stays unlocked");

    printf("=== 18. digits while unlocked must not re-lock or buffer ===\n");
    reset_state(); unlock();
    press(cmd3);press(cmd4);press(cmd5);press(cmd7);
    CHECK(passcodeEntered, "typing digits while unlocked must not lock the turret");
    CHECK(strlen(passcode)==0, "digits while unlocked must not fill the buffer");


    printf("=== 19. pin the calibration constants ===\n");
    CHECK(pitchMax == 150, "pitchMax must be 150 (frame clearance)");
    CHECK(pitchMin == 33,  "pitchMin must be 33 (frame clearance)");
    CHECK(PASSCODE_LENGTH == 4, "PASSCODE_LENGTH must be 4");
    CHECK(strcmp(CORRECT_PASSCODE, "2468") == 0, "passcode must be 2468");
    CHECK(rollPrecision == 240, "rollPrecision must be the tuned 240");
    CHECK(rollStep == 10, "rollStep must be the tuned +10");
    CHECK(rollMoveSpeed == 90 && rollStopSpeed == 90, "roll must stay at full speed");

    printf("=== 20. firing timing (virtual clock) ===\n");
    reset_state(); unlock(); dartsFired=0;
    { unsigned long t0=g_virtual_ms; fire();
      unsigned long d=g_virtual_ms-t0;
      CHECK(d >= (unsigned long)rollPrecision, "fire() must run the barrel for rollPrecision ms");
      CHECK(d < (unsigned long)rollPrecision+50, "fire() must not dawdle beyond rollPrecision");
      printf("  fire() = %lu ms\n", d); }
    { unsigned long t0=g_virtual_ms; fireAll();
      unsigned long d=g_virtual_ms-t0;
      CHECK(d >= (unsigned long)rollPrecision*6, "fireAll() must run 6x rollPrecision");
      printf("  fireAll() = %lu ms\n", d); }

    printf("=== 21. yaw timing per press ===\n");
    reset_state(); unlock();
    { unsigned long t0=g_virtual_ms; leftMove(1);
      unsigned long d=g_virtual_ms-t0;
      CHECK(d >= (unsigned long)yawPrecision && d < (unsigned long)yawPrecision+50,
            "leftMove(1) must run the yaw servo for yawPrecision ms");
      printf("  leftMove(1) = %lu ms (yawPrecision=%d)\n", d, yawPrecision); }

    printf("=== 22. held button covers exactly twice a tap ===\n");
    reset_state(); unlock();
    pitchServoVal = 100;
    press(up, false);
    int tap = pitchServoVal - 100;
    pitchServoVal = 100;
    press(up, true);
    int held = pitchServoVal - 100;
    CHECK(tap == pitchMoveSpeed, "a tap moves exactly pitchMoveSpeed");
    CHECK(held == 2*tap, "a held button moves exactly twice a tap");
    printf("  tap=%d held=%d\n", tap, held);

    printf("=== 23. buffer guard is actually reachable ===\n");
    {
        reset_state();
        // drive addPasscodeDigit directly past the limit, bypassing checkPasscode
        for(int i=0;i<10;i++) addPasscodeDigit('1');
        CHECK(strlen(passcode) == (size_t)PASSCODE_LENGTH,
              "addPasscodeDigit must cap the buffer on its own");
        printf("  buffer after 10 direct digits: \"%s\"\n", passcode);
    }


    printf("=== 24. the per-shot ramp ===\n");
    {
        reset_state(); unlock(); dartsFired = 0;
        int times[6];
        for(int n=0;n<6;n++){
            unsigned long t0=g_virtual_ms;
            fire();
            times[n] = (int)(g_virtual_ms - t0);
        }
        printf("  shot times:");
        for(int n=0;n<6;n++) printf(" %d", times[n]);
        printf("\n");
        int bad=0;
        for(int n=1;n<6;n++) if(times[n]-times[n-1] != rollStep) bad++;
        CHECK(bad==0, "each shot must be exactly rollStep longer than the last");
        CHECK(dartsFired==6, "magazine counter must reach 6");
        // and must not keep climbing past the magazine
        for(int i=0;i<10;i++) fire();
        CHECK(dartsFired==6, "counter must cap at 6 rather than climbing forever");
        // fireAll resets it
        fireAll();
        CHECK(dartsFired==0, "fireAll must reset the magazine counter");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

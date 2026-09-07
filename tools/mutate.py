import subprocess, shutil, os
base = open("sketch.cpp").read()

# (label, find, replace) - each flips one real decision in the logic
MUTANTS = [
 ("clamp up: min->max",            "min(pitchServoVal + pitchMoveSpeed, pitchMax)", "max(pitchServoVal + pitchMoveSpeed, pitchMax)"),
 ("clamp down: max->min",          "max(pitchServoVal - pitchMoveSpeed, pitchMin)", "min(pitchServoVal - pitchMoveSpeed, pitchMin)"),
 ("up: + -> -",                    "min(pitchServoVal + pitchMoveSpeed, pitchMax)", "min(pitchServoVal - pitchMoveSpeed, pitchMax)"),
 ("down: - -> +",                  "max(pitchServoVal - pitchMoveSpeed, pitchMin)", "max(pitchServoVal + pitchMoveSpeed, pitchMin)"),
 ("pitchMax 150 -> 151",           "int pitchMax = 150;", "int pitchMax = 151;"),
 ("pitchMin 33 -> 32",             "int pitchMin = 33;",  "int pitchMin = 32;"),
 ("passcode 2468 -> 2469",         '#define CORRECT_PASSCODE "2468"', '#define CORRECT_PASSCODE "2469"'),
 ("PASSCODE_LENGTH 4 -> 3",        "#define PASSCODE_LENGTH 4", "#define PASSCODE_LENGTH 3"),
 ("strcmp ==0 -> !=0",             "strcmp(passcode, CORRECT_PASSCODE) == 0", "strcmp(passcode, CORRECT_PASSCODE) != 0"),
 ("unlock sets false",            "passcodeEntered = true;\n        shakeHeadYes();", "passcodeEntered = false;\n        shakeHeadYes();"),
 ("debounce && -> ||",             "if (isRepeat && !passcodeEntered)", "if (isRepeat || !passcodeEntered)"),
 ("debounce inverted",             "if (isRepeat && !passcodeEntered)", "if (!isRepeat && !passcodeEntered)"),
 ("drop debounce return",          "if (isRepeat && !passcodeEntered) {\n        return;\n    }", ""),
 ("buffer guard < -> <=",          "if (len < PASSCODE_LENGTH)", "if (len <= PASSCODE_LENGTH)"),
 ("check == -> >=",                "if (strlen(passcode) == PASSCODE_LENGTH)", "if (strlen(passcode) >= PASSCODE_LENGTH)"),
 ("star does not clear buffer",    "passcodeEntered = false;\n            passcode[0] = '\\0';", "passcodeEntered = false;"),
 ("no buffer reset after check",   "passcode[0] = '\\0'; // reset the buffer either way", ""),
 ("nod restore removed",           "pitchServoVal = originalPitchVal;", ""),
 ("locked: up not gated",          "case up: //pitch up\n          if (passcodeEntered) { upMove(isRepeat ? 2 : 1); }", "case up: //pitch up\n          { upMove(isRepeat ? 2 : 1); }"),
 ("locked: fire not gated",        "if (passcodeEntered) { fire(); }", "{ fire(); }"),
 ("repeat multiplier 2 -> 1",      "upMove(isRepeat ? 2 : 1)", "upMove(1)"),
 ("rollPrecision 240 -> 180", "int rollPrecision = 240;", "int rollPrecision = 180;"),
 ("rollStep 10 -> 0",            "int rollStep = 10;", "int rollStep = 0;"),
 ("ramp: + -> -",                 "int thisShot = rollPrecision + (rollStep * dartsFired);", "int thisShot = rollPrecision - (rollStep * dartsFired);"),
 ("counter cap removed",         "if (dartsFired < 6) { //stop climbing once the magazine is spent\n      dartsFired++;\n    }", "dartsFired++;"),
 ("fireAll no counter reset",     "dartsFired = 0;\n    flushIR();", "flushIR();"),
 ("yawPrecision 70 -> 200",        "int yawPrecision = 70;", "int yawPrecision = 200;"),
 ("pitchMoveSpeed 6 -> 20",        "int pitchMoveSpeed = 6;", "int pitchMoveSpeed = 20;"),
]

killed=survived=invalid=0
stale=[]
surv_list=[]
for label, find, repl in MUTANTS:
    if base.count(find) < 1:
        stale.append(label)
        print(f"  STALE PATTERN (does not match the sketch): {label}"); continue
    open("sketch.cpp","w").write(base.replace(find, repl, 1))
    c = subprocess.run(["g++","-std=c++17","-I.","tests.cpp","stubs.cpp","-o","mut"],
                       capture_output=True)
    if c.returncode != 0:
        invalid+=1; print(f"  invalid  {label}"); continue
    r = subprocess.run(["./mut"], capture_output=True, timeout=180)
    if r.returncode != 0:
        killed+=1; print(f"  KILLED   {label}")
    else:
        survived+=1; surv_list.append(label); print(f"  SURVIVED {label}")

open("sketch.cpp","w").write(base)
total = killed+survived
print(f"\nmutation score: {killed}/{total} killed ({100*killed//max(total,1)}%), {invalid} invalid")
if stale:
    print("\nSTALE PATTERNS, the reported score is not trustworthy:")
    for s in stale: print("  -", s)
    raise SystemExit(1)
if surv_list:
    print("survivors:")
    for s in surv_list: print("  -", s)

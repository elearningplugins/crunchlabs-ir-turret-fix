import random, pickle
CHAMBER = 60.0
best_p = pickle.load(open("fit.pkl","rb"))
v0, c, h, tol, sigma = best_p

def sim(base, ramp, loaded, trials, rng):
    tot = 0
    for _ in range(trials):
        pos = 0.0; fired = 0; remaining = loaded
        for n in range(loaded):
            t = base + ramp*n
            v = v0 - c*remaining - h*n
            if v < 0.01: v = 0.01
            pos += v*t + rng.gauss(0, sigma)
            if abs(pos - CHAMBER*(n+1)) <= tol:
                fired += 1; remaining -= 1
        tot += fired
    return tot/trials

rng = random.Random(11)
print("Fitted params: v0=%.4f c=%.4f h=%.4f tol=%.1f sigma=%.1f\n" % best_p)

# ---- grid search over base and ramp ----
results = []
for base in range(190, 331, 2):
    for ramp in range(-12, 25, 2):
        m = sim(base, ramp, 6, 600, rng)
        results.append((m, base, ramp))
results.sort(reverse=True)

print("TOP 15 CONFIGURATIONS (expected darts fired out of 6)")
print(f"  {'base':>5} {'ramp':>5} {'expected':>9}   shot times")
seen = set()
shown = 0
for m, base, ramp in results:
    key = (base//6, ramp)
    if key in seen: continue
    seen.add(key)
    times = ", ".join(str(base+ramp*n) for n in range(6))
    print(f"  {base:>5} {ramp:>+5} {m:>9.2f}   {times}")
    shown += 1
    if shown >= 15: break

# ---- refine the winner with more trials ----
print("\nREFINED (5000 trials each) around the best region")
cands = sorted({(b,r) for _,b,r in results[:40]})
scored = sorted(((sim(b,r,6,5000,rng), b, r) for b,r in cands), reverse=True)
for m,b,r in scored[:8]:
    print(f"  base={b:>3} ramp={r:>+3}  expected={m:.2f}/6")

bm, bb, br = scored[0]
print(f"\nBEST: base={bb} ramp={br:+d}  expected {bm:.2f}/6")
print("  shot times:", ", ".join(str(bb+br*n) for n in range(6)))

# ---- how good is a flat setting at best? ----
flat = sorted(((sim(b,0,6,5000,rng), b) for b in range(190,331,2)), reverse=True)
print(f"\nBest FLAT setting: base={flat[0][1]} expected {flat[0][0]:.2f}/6")
print(f"Ramp advantage: {bm - flat[0][0]:+.2f} darts")

# ---- free-form: optimise all six times independently (upper bound on any schedule) ----
print("\nUPPER BOUND: optimising all six shot times independently")
times = [bb + br*n for n in range(6)]
def sim_times(ts, trials, rng):
    tot = 0
    for _ in range(trials):
        pos = 0.0; fired = 0; remaining = 6
        for n in range(6):
            v = v0 - c*remaining - h*n
            if v < 0.01: v = 0.01
            pos += v*ts[n] + rng.gauss(0, sigma)
            if abs(pos - CHAMBER*(n+1)) <= tol:
                fired += 1; remaining -= 1
        tot += fired
    return tot/trials
cur = sim_times(times, 3000, rng)
for it in range(600):
    i = rng.randrange(6)
    cand = list(times); cand[i] += rng.choice([-6,-4,-2,2,4,6])
    v = sim_times(cand, 3000, rng)
    if v > cur: cur, times = v, cand
print("  times:", ", ".join(str(t) for t in times))
print(f"  expected {cur:.2f}/6")

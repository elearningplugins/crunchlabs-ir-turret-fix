import random, math

# ---- observed data: (base_ms, ramp_ms_per_shot, darts_loaded, darts_fired) ----
DATA = [
    (158,  0, 5, 2),
    (238,  0, 6, 5),
    (248,  0, 6, 5),
    (248,  0, 6, 4),
    (258,  0, 6, 5),
    (264,  0, 6, 5),
    (268,  0, 6, 3),
    (240,  8, 6, 5),
    (240, 12, 6, 4),
]

CHAMBER = 60.0  # degrees between chambers

def simulate(base, ramp, loaded, p, trials, rng):
    """p = (v0, c, h, tol, sigma). Returns mean darts fired."""
    v0, c, h, tol, sigma = p
    total = 0
    for _ in range(trials):
        pos = 0.0          # actual cumulative barrel rotation
        fired = 0
        remaining = loaded
        for n in range(loaded):
            t = base + ramp * n
            # speed falls with load on the barrel and with servo heating over the burst
            v = v0 - c * remaining - h * n
            if v < 0.01:
                v = 0.01
            advance = v * t + rng.gauss(0, sigma)
            pos += advance
            target = CHAMBER * (n + 1)
            if abs(pos - target) <= tol:
                fired += 1
                remaining -= 1
        total += fired
    return total / trials

def loss(p, rng, trials=300):
    s = 0.0
    for base, ramp, loaded, obs in DATA:
        pred = simulate(base, ramp, loaded, p, trials, rng)
        s += (pred - obs) ** 2
    return s

# ---- fit by coarse-to-fine random search ----
rng = random.Random(7)
best, best_loss = None, float("inf")
# v0 deg/ms at no load, c deg/ms per dart, h deg/ms per shot (heating), tol deg, sigma deg
lo = [0.20, 0.000, 0.000,  4.0, 0.5]
hi = [0.80, 0.060, 0.080, 40.0, 16.0]

for it in range(4000):
    p = tuple(rng.uniform(lo[i], hi[i]) for i in range(5))
    L = loss(p, rng, trials=120)
    if L < best_loss:
        best_loss, best = L, p

for shrink in (0.5, 0.25, 0.12, 0.06):
    span = [(hi[i] - lo[i]) * shrink for i in range(5)]
    for it in range(3000):
        p = tuple(min(hi[i], max(lo[i], best[i] + rng.uniform(-span[i], span[i]))) for i in range(5))
        L = loss(p, rng, trials=200)
        if L < best_loss:
            best_loss, best = L, p

v0, c, h, tol, sigma = best
print("FITTED MODEL")
print(f"  speed at no load      v0    = {v0:.4f} deg/ms")
print(f"  slowdown per dart     c     = {c:.4f} deg/ms/dart")
print(f"  slowdown per shot     h     = {h:.4f} deg/ms/shot (heating)")
print(f"  peg tolerance         tol   = {tol:.1f} deg")
print(f"  per-shot noise        sigma = {sigma:.1f} deg")
print(f"  fit loss (sum sq err) = {best_loss:.2f}\n")

print("MODEL vs OBSERVED")
print(f"  {'base':>5} {'ramp':>5} {'obs':>4} {'pred':>6}")
for base, ramp, loaded, obs in DATA:
    pred = simulate(base, ramp, loaded, best, 4000, rng)
    print(f"  {base:>5} {ramp:>5} {obs:>4} {pred:>6.2f}")

import pickle
pickle.dump(best, open("fit.pkl","wb"))

import random, pickle
CHAMBER=60.0
v0,c,h,tol,sigma = pickle.load(open("fit.pkl","rb"))
def sim(base,ramp,trials,rng,loaded=6):
    tot=0
    for _ in range(trials):
        pos=0.0; fired=0; rem=loaded
        for n in range(loaded):
            t=base+ramp*n
            v=v0-c*rem-h*n
            if v<0.01: v=0.01
            pos+=v*t+rng.gauss(0,sigma)
            if abs(pos-CHAMBER*(n+1))<=tol:
                fired+=1; rem-=1
        tot+=fired
    return tot/trials
rng=random.Random(23)
print("Best BASE for each ramp value (20,000 trials each)\n")
print(f"  {'ramp':>5} {'base':>5} {'expected':>9}   shot times")
rows=[]
for ramp in range(-12,13,2):
    best=(-1,None)
    for base in range(200,301,2):
        m=sim(base,ramp,1500,rng)
        if m>best[0]: best=(m,base)
    m=sim(best[1],ramp,20000,rng)
    rows.append((m,ramp,best[1]))
    times=", ".join(str(best[1]+ramp*n) for n in range(6))
    print(f"  {ramp:>+5} {best[1]:>5} {m:>9.3f}   {times}")
print()
rows.sort(reverse=True)
print("Ranked:")
for m,ramp,base in rows:
    print(f"  ramp={ramp:>+3} base={base:>3}  {m:.3f}/6")
pos=[r for r in rows if r[1]>0]
neg=[r for r in rows if r[1]<0]
flat=[r for r in rows if r[1]==0]
print(f"\nBest POSITIVE ramp: ramp={pos[0][1]:+d} base={pos[0][2]}  {pos[0][0]:.3f}/6")
print(f"Best NEGATIVE ramp: ramp={neg[0][1]:+d} base={neg[0][2]}  {neg[0][0]:.3f}/6")
print(f"Flat:               base={flat[0][2]}  {flat[0][0]:.3f}/6")
print(f"\nSpread across all ramps: {rows[0][0]-rows[-1][0]:.3f} darts")

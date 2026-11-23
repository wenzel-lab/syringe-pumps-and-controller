# Syringe-pump gravimetric calibration

Calibrate a syringe pump's delivered flow using **only an analytical balance** — no flow
sensor needed. Real syringes have inner-diameter tolerance, so the delivered flow can be off
by several percent; this measures the true diameter and corrects it.

Water weighs ~1 g/mL (**1 mg ≈ 1 µL**), so the balance gives real dispensed volume:

```
Q_real = mass / (ρ_water · time)
D_real = D₀ · √(Q_real / Q_set)
```

## Quick start

```bash
# interactive — just type the numbers you measured
python3 calibrate.py

# or one line
python3 calibrate.py --d0 8.94 --qset 1000 --mass 170.5 --minutes 10 --temp 20
#   flow real (balance): 1024.8 uL/h  (+2.5 %)
#   CORRECTED DIAMETER : 9.05 mm   <- enter this on the pump
```

Enter the corrected diameter on the pump; it now delivers true flow. Every run is logged to
`calibrations/`.

## Full bench procedure

See **[PROTOCOL.md](PROTOCOL.md)** — balance calibration & warm-up, syringe prep, weighed
collection (with parafilm against evaporation), computing and verifying the diameter.

## Requirements

Python 3 standard library only — no dependencies.

## Higher precision: flow-sensor cross-check

Optional. With an in-line flow sensor the same runs also calibrate the sensor against the
balance; pass `--sensor-ulh <reading>` to `calibrate.py`. For the flow-sensor hardware and
acquisition see **[wenzel-lab/rio-controller](https://github.com/wenzel-lab/rio-controller)**.

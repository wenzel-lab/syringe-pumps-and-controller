#!/usr/bin/env python3
"""Gravimetric flow calibration — corrects syringe inner diameter AND the flow sensor.

A single set of gravimetric runs yields two corrections, both anchored to the balance
(ground truth, 1 mg water ~ 1 uL):

  1. Syringe diameter (pump).  A Harvard-type syringe pump sets plunger speed from the
     diameter D0 you program and the target flow Q_set. If the true diameter differs,
     delivered flow scales with AREA (proportional to D^2), not speed:
         Q_real = mass / (rho_water * time)
         D_real = D0 * sqrt(Q_real / Q_set)
     Balance reads MORE than programmed -> syringe is WIDER -> increase D.

  2. Flow sensor.  Independent of the pump: compare balance vs what the sensor read
     over the same window:
         sensor_factor = Q_real / sensor_reading        (corrected = reading * factor)
     factor ~ 1.00 means the sensor is good as-is.

No hand calculation: enter the few numbers you measured and the tool does the rest.

INTERACTIVE:
    python3 calibrate.py

ONE-LINER:
    python3 calibrate.py --syringe BD-3ml --qset 1000 --mass 168.2 --minutes 10 --temp 20
    python3 calibrate.py --d0 8.66 --qset 1000 --qreal 1011 --sensor-ulh 1091

Each run is logged to ./calibrations/<timestamp>_<syringe>.json
"""
import argparse, json, os, math, time

OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "calibrations")

# Nominal inner diameter (mm) of common syringes — the starting point D0.
# Correcting these to D_real is the whole point of the procedure.
SYRINGES = {
    "BD-1ml": 4.78, "BD-3ml": 8.66, "BD-5ml": 11.99, "BD-10ml": 14.50,
    "BD-20ml": 19.13, "BD-50ml": 26.59,
    "Hamilton-1ml": 4.61, "HSW-NormJect-3ml": 8.90,
}

# Water density (g/mL) vs temperature (C), ambient pressure.
_RHO = {15: 0.99910, 18: 0.99860, 20: 0.99821, 22: 0.99777,
        24: 0.99730, 25: 0.99705, 28: 0.99626, 30: 0.99565}


def rho_water(t):
    ks = sorted(_RHO)
    if t <= ks[0]: return _RHO[ks[0]]
    if t >= ks[-1]: return _RHO[ks[-1]]
    for a, b in zip(ks, ks[1:]):
        if a <= t <= b:
            return _RHO[a] + (t - a) / (b - a) * (_RHO[b] - _RHO[a])


def ask(prompt, cast=float, default=None):
    while True:
        s = input(f"{prompt}{f' [{default}]' if default is not None else ''}: ").strip()
        if not s and default is not None:
            return default
        try:
            return cast(s)
        except ValueError:
            print("  invalid value")


def main():
    p = argparse.ArgumentParser(description="Gravimetric syringe-diameter and flow-sensor calibration")
    p.add_argument("--syringe", choices=list(SYRINGES))
    p.add_argument("--d0", type=float, help="nominal inner diameter (mm) if not in the list")
    p.add_argument("--qset", type=float, help="flow programmed on the pump (uL/h)")
    p.add_argument("--mass", type=float, help="dispensed mass = collected water, full − empty (mg)")
    p.add_argument("--minutes", type=float, help="dispense duration (min)")
    p.add_argument("--qreal", type=float, help="measured real flow (uL/h) — alternative to mass+minutes")
    p.add_argument("--temp", type=float, default=20.0, help="water temperature (C)")
    p.add_argument("--sensor-ulh", type=float, help="median the flow sensor read over the window (uL/h)")
    p.add_argument("--label", default="")
    a = p.parse_args()

    interactive = a.qset is None
    print("=== Gravimetric flow calibration (Harvard reference pump + analytical balance) ===\n")

    if a.d0 is not None:
        d0, syr = a.d0, (a.syringe or f"custom-{a.d0}mm")
    elif a.syringe:
        d0, syr = SYRINGES[a.syringe], a.syringe
    else:
        print("Known syringes:", ", ".join(SYRINGES))
        syr = input("Syringe (name from list, or ENTER to type a diameter): ").strip()
        if syr in SYRINGES:
            d0 = SYRINGES[syr]
        else:
            d0 = ask("Nominal inner diameter D0 (mm)")
            syr = syr or f"custom-{d0}mm"

    qset = a.qset if a.qset is not None else ask("Flow programmed on the pump Q_set (uL/h)")
    temp = a.temp if not interactive else ask("Water temperature (C)", default=20.0)

    if a.qreal is not None:
        qreal, mass, minutes = a.qreal, None, None
    else:
        mass = a.mass if a.mass is not None else ask("Dispensed mass on the balance (collected water, full − empty) (mg)")
        minutes = a.minutes if a.minutes is not None else ask("Dispense duration (min)")
        qreal = (mass / rho_water(temp)) / (minutes / 60.0)   # uL/h

    ratio = qreal / qset
    d_real = d0 * math.sqrt(ratio)
    err = 100.0 * (ratio - 1.0)

    print("\n--- RESULT ---")
    print(f"syringe            : {syr}")
    print(f"nominal diameter   : {d0:.3f} mm")
    print(f"flow programmed    : {qset:.1f} uL/h")
    print(f"flow real (balance): {qreal:.1f} uL/h   ({err:+.1f} % vs programmed)")
    print(f"CORRECTED DIAMETER : {d_real:.3f} mm   <- enter this on the pump")
    if abs(err) > 15:
        print("  ! correction > 15 %: check units, leaks or bubbles before trusting it.")

    rec = {"date": time.strftime("%Y-%m-%d %H:%M:%S"), "syringe": syr, "d0_mm": d0,
           "qset_ul_h": qset, "temp_c": temp, "q_real_ul_h": round(qreal, 2),
           "error_pct": round(err, 2), "d_real_mm": round(d_real, 3), "label": a.label}
    if mass is not None:
        rec.update(mass_mg=mass, minutes=minutes, rho_g_ml=round(rho_water(temp), 5))

    if a.sensor_ulh is not None:
        fac = qreal / a.sensor_ulh
        serr = 100.0 * (a.sensor_ulh / qreal - 1.0)
        print(f"\nsensor read        : {a.sensor_ulh:.1f} uL/h   ({serr:+.1f} % vs balance)")
        print(f"SENSOR FACTOR      : x{fac:.4f}   (corrected = reading * {fac:.4f})")
        rec.update(sensor_ul_h=a.sensor_ulh, sensor_factor=round(fac, 4),
                   sensor_error_pct=round(serr, 2))

    os.makedirs(OUTDIR, exist_ok=True)
    fn = os.path.join(OUTDIR, f"{time.strftime('%Y%m%d_%H%M%S')}_{syr}.json")
    with open(fn, "w") as f:
        json.dump(rec, f, indent=2)
    print(f"\nlog: {fn}")


if __name__ == "__main__":
    main()

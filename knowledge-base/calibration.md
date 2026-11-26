# Calibrating a syringe pump by gravimetry

A syringe pump, 3D printed or commercial, computes plunger speed from the **syringe inner diameter** you program.

Real syringes have diameter tolerance, so the **delivered flow can be off by several percent** even though the pump is mechanically accurate. In our tests a "3 mL" syringe with a nominal 8.94 mm bore actually delivered **+2.5 %**; the corrected diameter was 9.05 mm.

## Method 1: Gravimetric calibration

This gravimetric calibration needs **only an analytical balance**, no flow sensor.

Water weighs ~1 g/mL, so **1 mg ≈ 1 µL**: the balance gives real dispensed volume directly.

```math
Q_{\text{real}} = \frac{\text{mass}}{\rho_{\text{water}} \cdot \text{time}}
D_{\text{real}} = D_0 \cdot \sqrt{\frac{Q_{\text{real}}}{Q_{\text{set}}}}
```

If the balance reads **more** flow than programmed → the syringe is **wider** than you told the pump → increase the diameter. Enter `D_real` and the pump delivers true flow.

The calculator `calibrate.py` does the math, you just type the numbers you measured.

>i **No install needed:** open the [web calculator](calibration/calculator.html) in any browser (PC, phone), type the weighed values, and it returns the corrected diameter. The `calibrate.py` script below does the same from the command line.

### Equipment

- **Analytical balance, 1 mg** (0.001 g); 0.1 mg is even better.
- Syringe under test, filled with **bubble-free** distilled water.
- **Parafilm** + a small receiving tube (e.g. Eppendorf).
- Tweezers/tongs for handling calibration weights (skin oil weighs enough to matter).
- The syringe pump (this project).

## Calibrate the balance (once) {pagestep}

Place it on a stable, **level** table (centre the bubble with the feet); no drafts, sun or vibration. **Warm it up for ≥ 1 hour** powered on.

Empty pan, reading `0.0`. Enter external calibration (on CGOLDENWALL or JoanLab HZ-type balances: `Set` → menu → **`e-cal`**; it requests a standard weight, e.g. `cal-200` = 200 g).

Place the exact standard weight **with tongs**, wait, remove when prompted → returns to `0.0`.

>i Place a known weight (e.g. 100 g) and check it reads correctly.

## Prepare the run {pagestep}

Fill the syringe, **purge all air** (a single bubble ruins a run). Mount it in the pump.

Program the **nominal diameter D₀** (caliper the inner bore, or use the syringe spec) and the target flow **Q_set**.

Let the pump drip into **any waste tube** until the flow is **steady** — **we use 5 min**; the first minutes clear residual bubbles and let the mechanics settle.

## Collect the output, weighed (timed collection) {pagestep}

**Prepare the collection tube first:** close the empty Eppendorf, seal the top with **parafilm**, pierce a **small hole** with the **sharp tip of scissors** (just big enough for the capillary — keeps evaporation low), and stand it on a **small, very light base** (we cut one from the foam used to pack boxes — negligible mass, keeps the round-bottom tube upright on the pan).

**Weigh it in this final state** (tube + parafilm + hole + foam base) → `m_empty`, or tare to zero.

**Then collect, without stopping the flow:** with the flow already steady (Step 2), move the capillary from the waste tube into the collection tube through the parafilm hole, and **start the stopwatch at that instant**.

Collect for an **exactly timed interval `t`** (`t` goes straight into the result). Move the capillary out, press the parafilm closed over the hole, and weigh immediately → `m_full`.

```math
Δm = m_{\text{full}} − m_{\text{empty}}
```

Aim for **Δm ≥ 100 mg** (e.g. 10 min at 1000 µL/h ≈ 167 mg → <1 % uncertainty at 1 mg).

<table>
<tr>
<td align="center" width="25%"><img width="200" alt="step 1" src="https://github.com/user-attachments/assets/76a64963-dd73-4e40-815d-11dcb9445f08" /><br><sub>1 — Caliper the bore to get the nominal diameter D₀</sub></td>
<td align="center" width="25%"><img width="200" alt="step 2" src="https://github.com/user-attachments/assets/36ad7822-de1d-44a0-88d6-b6ef6476418c" /><br><sub>2 — Seal the tube top with parafilm</sub></td>
<td align="center" width="25%"><img width="200" alt="step 3" src="https://github.com/user-attachments/assets/1dcda40d-8cfe-4803-887a-38633ac9baae" /><br><sub>3 — Pierce a small hole with a scissor tip</sub></td>
<td align="center" width="25%"><img width="200" alt="step 4" src="https://github.com/user-attachments/assets/a4ea4f2d-0760-4617-b317-4fd9e5fd26e1" /><br><sub>4 — Insert the capillary through the hole</sub></td>
</tr>
</table>



## Compute and correct {pagestep}

Download the tool: [`calibrate.py`](./calibration/calibrate.py) (Python 3, no dependencies).

```python
python3 calibrate.py --d0 8.94 --qset 1000 --mass 170.5 --minutes 10 --temp 20
#   flow real (balance): 1024.8 uL/h  (+2.5 %)
#   CORRECTED DIAMETER : 9.05 mm   <- enter this on the pump
```

Enter `D_real` on the pump. Repeat **one point** to confirm Q_real ≈ Q_set.

## Verify linearity {pagestep}

Repeat at **2–3 flows** (e.g. 500, 1000, 2000 µL/h) with the corrected diameter. If they all give the same `D_real`, the calibration is solid and flow-independent.

>i **Practical tips (what ruins a run):**
>i 
>i - **Evaporation**: at 1000 µL/h an open tube loses ~1 mg/min, always parafilm, and weigh immediately after capping.
>i - **Weights with tongs**, never fingers.
>i - **Exclude the first ~5 min** of each run (stabilization + bubble clearing).
>i - **Record water temperature**; the tool adjusts density (20 °C → 0.99821 g/mL). ±2 °C is negligible (<0.1 %).
>i - **Bubbles**: purge before, and if the output suddenly spikes/dips erratically, there is air in the line, stop and purge.

## Method 2: cross-check with a flow sensor (higher precision)

The balance fixes the diameter. If you also have an **in-line flow sensor**, the same run lets you check and calibrate the sensor against the balance (ground truth), and then use the sensor as a **continuous** flow reference without weighing every time. 

`calibrate.py` reports the sensor factor if you pass `--sensor-ul-h <reading>`.

For the flow-sensor hardware, firmware and high-rate acquisition, see the Rio controller repository: **[wenzel-lab/rio-controller](https://github.com/wenzel-lab/rio-controller)** (pressure/flow module).

---
*Tool: `calibrate.py` · rev 2026-11-19*

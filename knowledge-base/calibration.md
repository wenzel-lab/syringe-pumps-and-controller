# Calibrating a syringe pump by gravimetry

A syringe pump's delivered flow depends on two things: **how far the drive head travels** (the pump mechanics — the carriage that pushes the plunger) and the **syringe inner diameter** you program.

Weighing the water measures only their *product*, so calibrate them **in order** — the pump **mechanics first** (Method 1), then the **syringe diameter** (Method 2). Do it the other way round and a mechanics error hides inside a wrong "effective diameter". In our tests a "3 mL" syringe with a nominal 8.94 mm bore actually delivered **+2.5 %**; the corrected diameter was 9.05 mm.

## Method 1: Calibrate the pump mechanics (plunger travel)

First check the mechanics on their own: *does the drive head move the distance it should?* This is the **displacement method** of EURAMET CG-27 §4.3.3. The metrology-lab version uses a laser interferometer; the workshop version uses a **caliper or dial indicator** — the same measurement a 3D-printer axis's steps/mm calibration uses.

>i **⚙️ First, match the software to this pump.** `gearbox`, `rod` (lead screw) and `microstep` are stored **per pump** and must match the hardware you built — set them on the keypad menu or with `set_gearbox` / `set_rod` / `set_microstep`. Re-check them whenever you swap pumps, or move the controller between a geared and an ungeared pump. On this project's pumps: ungeared → `gearbox 1:1`; geared → `gearbox 100:1`; both use the T8 **1-star (2 mm)** rod and `microstep 1/8`.

The pump has **no "move N mm" command** — you set a diameter and a flow and let it run, so the travel comes from `flow × time`:

1. Mount a **caliper or dial indicator** against the drive head / carriage and **zero it**. Keep a syringe mounted, dispensing to waste, as in a normal run.
2. Set the **diameter D** and a **flow Q** and run for a few minutes (**longer = smaller % error**). The commanded travel is:

```math
L_{\text{set}} = \frac{Q \cdot t}{\pi (D/2)^2}
```

e.g. D = 9.05 mm, Q = 3000 µL/h, t = 15 min → **L_set ≈ 11.7 mm** (keep units consistent; the gearbox does **not** change this travel).

3. Read the **actual travel** `L_real` (= |end − start| caliper readings). Repeat 2–3× and average to shrink the reading scatter.
4. If `L_real ≈ L_set`, the mechanics are good. Otherwise correct the lead-screw pitch (`THREADROD`): `THREADROD_corr = THREADROD × (L_real / L_set)`. The firmware derives steps/mm from `microstep`, `gearbox` and `threadrod`, so `threadrod` is the knob to trim.

>i **No install needed:** open the [mechanics web calculator](calibration/calculator-mechanics.html) in any browser — enter flow, time, diameter and the start/end caliper readings, and it returns the expected travel, the mechanical error, and the corrected THREADROD.

>i **No caliper?** Even a **ruler** works — just run longer so the move is ~50–100 mm and the coarser reading stays a small fraction of the distance.

**If `L_real` comes out very different from `L_set`**, a *clean factor* points to a wrong setting; a small % is just real tolerance or reading noise:

| `L_real` vs `L_set` | Likely cause | Fix |
|---|---|---|
| ~25× or ~100× off | **Gearbox** set wrong | set it to the pump's real ratio (1:1 / 25:1 / 100:1) |
| ~4× off | **Rod** wrong (1-star = 2 mm vs 4-star = 8 mm) | set rod to match the physical lead screw |
| ~2× / 4× / 8× off (gearbox & rod OK) | **Microstep** mismatch (driver ≠ firmware) | check microstep — over UART it's normally self-consistent, so this is rare |
| small (< ~2 %) | lead-screw pitch tolerance or reading noise | tiny `THREADROD` tweak, or repeat 2–3× and average |
| erratic / not repeatable | loose coupling grub screw, binding, or missed steps | tighten the coupling, check for binding, lower speed / raise motor current |

Once the mechanics check out, the diameter you get in Method 2 is the **real bore**, not a number that silently absorbs a steps/mm error.

## Method 2: Gravimetric calibration (the syringe diameter)

This gravimetric calibration needs **only an analytical balance**, no flow sensor.

Water weighs ~1 g/mL, so **1 mg ≈ 1 µL**: the balance gives real dispensed volume directly.

```math
Q_{\text{real}} = \frac{\text{mass}}{\rho_{\text{water}} \cdot \text{time}}
D_{\text{real}} = D_0 \cdot \sqrt{\frac{Q_{\text{real}}}{Q_{\text{set}}}}
```

If the balance reads **more** flow than programmed → the syringe is **wider** than you told the pump → increase the diameter. Enter `D_real` and the pump delivers true flow.

The calculator `calibrate.py` does the math, you just type the numbers you measured.

>i **No install needed:** open the [diameter web calculator](calibration/calculator.html) in any browser (PC, phone), type the weighed values, and it returns the corrected diameter. The `calibrate.py` script below does the same from the command line.

### Equipment

- **Analytical balance, 1 mg** (0.001 g); 0.1 mg is even better.
- **Caliper or dial indicator** (0.01 mm) — to measure drive-head travel (Method 1).
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

**Prepare the collection tube first:** close the empty Eppendorf, seal the top with **parafilm**, pierce a **small hole** with the **sharp tip of scissors** (just big enough for the capillary — the thin outlet tube from the syringe — keeps evaporation low), and stand it on a **small, very light base** (we cut one from the foam used to pack boxes — negligible mass, keeps the round-bottom tube upright on the pan).

**Weigh it in this final state** (tube + parafilm + hole + foam base) → `m_empty`, or tare to zero.

**Then collect, without stopping the flow:** with the flow already steady (previous step), move the capillary from the waste tube into the collection tube through the parafilm hole, and **start the stopwatch at that instant**.

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

Repeat at **at least 3 flow points — the ones you'll actually use — with 3 measurements each** (e.g. 500 / 1000 / 2000 µL/h ×3), following EURAMET CG-27. The 3 repeats give the repeatability; if all points give the same `D_real`, the calibration is solid and flow-independent.

>i **Practical tips (what ruins a run):**
>i 
>i - **Evaporation**: at 1000 µL/h an open tube loses ~1 mg/min, always parafilm, and weigh immediately after capping.
>i - **Weights with tongs**, never fingers.
>i - **Exclude the first ~5 min** of each run (stabilization + bubble clearing).
>i - **Record water temperature**; the tool adjusts density (20 °C → 0.99821 g/mL). ±2 °C is negligible (<0.1 %).
>i - **Bubbles**: purge before, and if the output suddenly spikes/dips erratically, there is air in the line, stop and purge.

## Method 3: cross-check with a flow sensor (higher precision)

The balance fixes the diameter. If you also have an **in-line flow sensor**, the same run lets you check and calibrate the sensor against the balance (ground truth), and then use the sensor as a **continuous** flow reference without weighing every time. 

`calibrate.py` reports the sensor factor if you pass `--sensor-ul-h <reading>`.

For the flow-sensor hardware, firmware and high-rate acquisition, see the Rio controller repository: **[wenzel-lab/rio-controller](https://github.com/wenzel-lab/rio-controller)** (pressure/flow module).

## References

- **EURAMET**, *Calibration Guide No. 27 — Guidelines for the Calibration of Drug Delivery Devices and Infusion Device Analysers*, v2.0 (2026). Gravimetric (§4.3.1) and **displacement (§4.3.3)** methods, mechanism cycle time. [PDF](https://www.euramet.org/Media/docs/Publications/calguides/I-CAL-GUI-027_Calibration_Guide_No._27_web.pdf)
- **IEC 60601-2-24** — infusion-pump flow accuracy (trumpet curve, start-up).
- **ISO 7886-2** — syringe cross-sectional-area tolerances (≈1 % large, 3 % small).
- **E. Batista et al.** (2020), *Development of an experimental setup for microflow measurement using interferometry*, Flow Measurement and Instrumentation **75**, 101789 — the displacement method. [link](https://www.sciencedirect.com/science/article/pii/S0955598620301345)
- **B. Wijnen, E. J. Hunt, G. C. Anzalone, J. M. Pearce** (2014), *Open-Source Syringe Pump Library*, PLoS ONE **9(9)**:e107216. [link](https://doi.org/10.1371/journal.pone.0107216)

---
*Tools: web calculators (mechanics + gravimetric) and `calibrate.py` · rev 2026-11-26*

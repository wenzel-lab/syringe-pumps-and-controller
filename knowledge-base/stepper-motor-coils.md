
# Stepper Motor Coil Identification Methods

Most NEMA 17 bipolar stepper motors have four wires corresponding to two independent electromagnetic coils.

The objective is to identify:

- Coil A (A+ and A-)

- Coil B (B+ and B-)

If you have access to the motor's datasheet, you can find the coil specifications there.

In our case, we are using a Usongshine Stepping Motor (Model: 17HS4401). Because the datasheet from the manufacturer is not available, we will use another one as a reference from the same motor model to identify the coils. The datasheet can be found [here](./17HS4401_Datasheet.pdf).

According to the datasheet, the coil resistance is **1.5 Ω** and the maximum current is **1.7 A**. The wire color coding is as follows:

| Wiring | Coil terminal |
|---|---|
| Black | Coil A+ | 
| Green | Coil A- |
| Red | Coil B+ |
| Blue | Coil B- |

To verify the coil connections, the following three methods shown in the video can be used to identify the motor coils. Instructions for each method are provided below:

![Video demonstration](https://www.youtube.com/watch?v=bx5XrROhr-E)

---

## Method 1: Multimeter Resistance Test (Recommended)

### Required equipment
- Digital multimeter.

### Procedure

1. Set the multimeter to resistance (Ω) or continuity mode.
2. Select any two motor wires.
3. Measure the resistance between them.
4. Repeat the measurement with different wire combinations.
5. Identify the two pairs that show a measurable resistance.

### Expected results

| Wire combination | Measurement | Result |
|---|---|---|
| Black – Green | 1.5 Ω | Coil A |
| Red – Blue | 1.5 Ω | Coil B |
| Red – Green | 0 Ω | Different coils |
| Red – Black | 0 Ω | Different coils |

### Interpretation

- Two wires with measurable resistance belong to the same coil.
- Two wires from different coils should show an open circuit (0 Ω).
- Typical winding resistance depends on the motor model.

>i **Note:** The wire color coding may vary depending on the manufacturer and the supplied cable. Always verify the wiring before connecting to the driver.

---

## Method 2: LED Generation Test

### Required equipment
- One LED.
- Connecting wires or alligator clips.

### Procedure

1. Connect the LED directly across two motor wires.
2. Rotate the motor shaft manually.
3. Observe whether the LED flashes.
4. Repeat with different wire combinations.

### Expected results

When the selected wires belong to the same coil, manually rotating the shaft generates a voltage in the winding.

This voltage can illuminate or flash the LED.

**Advantages:**

- Does not require a multimeter.
- Demonstrates electromagnetic induction.
- Useful for identifying winding pairs.

**Limitations:**

- Less reliable than the multimeter method.
- LED illumination depends on rotation speed and generated voltage.
- Some LEDs may not light up due to polarity or low voltage.

---

## Method 3: Shaft Resistance (Electromagnetic Braking) Test

### Required equipment
- No special equipment required.

### Procedure

1. Hold the motor shaft and rotate it gently.
2. Observe the force required to rotate the shaft.
3. Short-circuit two motor wires together.
4. Rotate the shaft again.
5. Compare the resistance to rotation.
6. Repeat with other wire combinations.

### Expected results

When two wires belonging to the same coil are shorted together, the motor generates current as the shaft rotates.

This produces electromagnetic braking, making the shaft noticeably harder to turn.

When wires from different coils are shorted together, this effect should not occur in the same way.

**Advantages:**

- No additional equipment required.
- Quick and easy to perform.
- Useful as an additional confirmation.

**Limitations:**

- The difference in rotational resistance can be subtle.
- Results depend on motor construction and rotational speed, as well as user perception.

---

## Coil Polarity Identification and Driver Connection

Once both coil pairs have been identified, they can be connected to a stepper motor driver.

### Example: A4988, DRV8825 or TMC2209

| Driver terminal | Motor connection |
|---|---|
| 1A | Coil A wire 1 |
| 1B | Coil A wire 2 |
| 2A | Coil B wire 1 |
| 2B | Coil B wire 2 |

### Determining polarity

The two wires belonging to each coil can initially be connected in either order.

For example:

- Coil A: Red → 1A, Blue → 1B
- Coil B: Green → 2A, Black → 2B

>i **Important:** The relative polarity of the windings determines the motor's rotation direction.

If the motor rotates in the opposite direction than expected, reverse the order of one coil or invert the direction signal in software.

>! **Safety Precautions**
>!
>! - Never connect or disconnect motor wires while the stepper driver is powered.
>! - Never measure winding resistance while the motor is connected to an energized driver.
>! - Avoid shorting motor wires while the driver is powered.
>! - Disconnect the motor from the driver before performing identification tests.
>! - Do not exceed the motor or driver current ratings.

## Final Recommendation

The **multimeter resistance test** (Method 1) is the recommended approach for identifying NEMA 17 motor coils.

The other two methods can be used as complementary tests when a multimeter is unavailable or when additional confirmation is desired.




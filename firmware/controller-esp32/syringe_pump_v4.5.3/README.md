# Syringe Pump Controller v4.5.3

A syringe pump controller with USB serial interface and physical keypad control. USB Serial Only version with enhanced safety and usability features.

## Features

- **Multi-motor Control**: Supports up to 4 independent syringe pumps (A-D).
- **Dual Control Modes**:
  - Physical keypad interface with 16x2 LCD display.
  - USB Serial communication for computer control (Jupyter Notebook).
- **Precise Flow Control**:
  - Adjustable flow rates (µL/hr, µL/min, mL/hr, mL/min) with safety validation (1–6000 µL/h).
  - Configurable syringe diameters (1–50 mm) with validation.
  - Infuse/Withdraw functionality.
- **Flexible Configuration**:
  - Multiple gearbox ratios (1:1, 25:1, 100:1).
  - Adjustable microstepping (1/8, 1/16, 1/32, 1/64).
  - Customizable lead screw options (1-start, 4-start).

- **Persistent Settings**: Motor configurations are saved to non-volatile storage and automatically loaded on startup.

- **Enhanced Safety**:
  - Input validation with real-time error feedback on LCD.
  - Out-of-range values are rejected with helpful error messages.
  - Users can immediately retry with corrected values without exiting input mode.

- **Instant Operation**: Pump runs immediately with last-saved settings on power-up.

## Hardware Requirements

- ESP32 microcontroller
- Stepper motor drivers (e.g., TMC2226 or TMC2209)
- 16x2 I2C LCD Display
- 4x4 Matrix Keypad
- Power supply (12V/6A recommended)
- Custom PCB (v3.0)

### Stepper Connection

|Motor Cable| Controller Connection|
|---|---|
|Red    | A+|
|Green  | A-|
|Black  | B+|
|Blue   | B-|

## Installation

1. Install the required libraries (Arduino IDE 2.3.5):
	- `PCA9555`				    / Author: Nico Verduin / Source: https://github.com/nicoverduin/PCA9555 
	- `PCF8574` library       @ 2.3.7 / Author: Renzo Mischianti   # GPIO extensor
	- `I2CKeyPad`             @ 0.5.0 / Author: Rob Tillaart       # Keypad
	- `LiquidCrystal I2C` 	@ 1.1.2 / Author: Frank de Brabander #LCD I2C display
	- `ContinuousStepper` 	@ 3.1.0 / Author: Benoit Blanchon	 # Control of stepper motors by rpm
	- `Preferences` (built-in with ESP32 core)

2. Open `syringe_pump_v4.5.3.ino` in Arduino IDE.

3. Select your ESP32 board and port.

4. Upload the sketch.

## Key Functions

|Key| Description|
|---|---|
|**A**| Enter flow input screen.
|**B**| Toggle infuse/withdraw mode.
|**C**| Cycle through active pumps (A → B → C → D).
|**D**| Insert decimal point (in diameter screen).
|*| Start/stop pump (main screen) or clear input.
|**#**| Enter settings menu or confirm selection.
|**2/8**| Navigate menu up/down

## Serial Commands

The pump is controlled via a text-based serial protocol over USB (115200 baud). Commands are case-insensitive and terminated by a newline character (\n).

### SET Command

Modifies a parameter for a specific pump.

Syntax: `SET PUMP=<pump> [PARAMETER=VALUE] ...`

`<pump>`: The target pump (`A`, `B`, `C`, or `D`).

### GET Command

Retrieves the current status and configuration of a pump.

Syntax: `GET PUMP=<pump> STATUS`

`<pump>`: The target pump (`A`, `B`, `C`, or `D`).

Example Response: `PUMP=A FLOW=150.50 DIAMETER=10.25 DIRECTION=INFUSE STATE=RUN UNIT=ML/MIN GEARBOX=25:1 MICROSTEP=1/32 ROD=4-START ENABLE=ON`

### Python Client & Jupyter Notebook

A Python client and a Jupyter Notebook with detailed examples are available to control the pump.

- Python Client: `client/python/pump_comm.py`
- Jupyter Notebook: `examples/pump-serial-comm.ipynb`

### Requirements

Install the pyserial library:

```python
pip install pyserial
```

### Example 1: Basic Configuration

Configure a pump and check its status.

```python
from pump_comm import SyringePumpController

# Connect to the pump on your system's serial port
# (e.g., 'COM7' on Windows or '/dev/ttyUSB0' on Linux)
try:
    ctrl = SyringePumpController('COM7')
    pump = 'B'

    # Configure pump parameters
    ctrl.set_flow(pump, 1000)
    ctrl.set_unit(pump, 'UL/HR')
    ctrl.set_diameter(pump, 8.17)
    ctrl.set_direction(pump, 'INFUSE')

    # Get the updated status
    status = ctrl.get_pump_status(pump)
    print(f"New status: {status}")

except Exception as e:
    print(f"An error occurred: {e}")
finally:
    if 'ctrl' in locals():
        ctrl.close()

```

### Example 2: Simple Pump Cycle

Run two pumps simultaneously for a fixed period.

```python
import time
from pump_comm import SyringePumpController

try:
    ctrl = SyringePumpController('COM7')
    pumps_to_run = ['A', 'B']
    run_period = 10  # seconds

    # Start pumps
    for p in pumps_to_run:
        ctrl.set_state(p, 'RUN')
        print(f"Pump {p} started: {ctrl.get_pump_status(p)}")

    # Wait for the run period
    print(f"Running pumps for {run_period} seconds...")
    time.sleep(run_period)

    # Stop pumps
    for p in pumps_to_run:
        ctrl.set_state(p, 'STOP')
        print(f"Pump {p} stopped: {ctrl.get_pump_status(p)}")

except Exception as e:
    print(f"An error occurred: {e}")
finally:
    if 'ctrl' in locals():
        ctrl.close()
```

### Example 3: Advanced Pump Sequence

Define a multi-step sequence with different flow rates and pauses. This example is simplified from the Jupyter Notebook.

```python
import time
from pump_comm import SyringePumpController

# Define the sequence of operations
SEQUENCE = [
    {
        'description': "Initial high flow",
        'duration': 10,  # seconds
        'pumps': {
            'A': {'flow_rate': 2000, 'diameter': 8.17, 'unit': 'UL/HR'},
            'B': {'flow_rate': 1000, 'diameter': 4.78, 'unit': 'UL/HR'}
        }
    },
    {
        'description': "Pause all pumps",
        'duration': 5,
        'pumps': {}  # Empty dict means all pumps are off
    },
    {
        'description': "Medium flow rate",
        'duration': 10,
        'pumps': {
            'A': {'flow_rate': 1000},
            'B': {'flow_rate': 500}
        }
    }
]

try:
    ctrl = SyringePumpController('COM7')
    
    # For the full implementation of run_pump_sequence,
    # see examples/pump-serial-comm.ipynb
    print("Running advanced sequence...")
    # run_pump_sequence(ctrl, SEQUENCE) 

    print("\nFor more advanced examples, please see the Jupyter Notebook:")
    print("examples/pump-serial-comm.ipynb")

except Exception as e:
    print(f"An error occurred: {e}")
finally:
    if 'ctrl' in locals():
        ctrl.close()
```
## Version History

### v4.5.3 (Current)
- **USB Serial Only**: Keypad interface operates in serial-only fallback mode if unavailable.
- **Enhanced Input Validation**: Flow rate (1–6000 µL/h) and diameter (1–50 mm) inputs are validated in real-time.
- **Improved Error Handling**: Invalid inputs display error messages on LCD; users can retry immediately.
- **Instant Startup**: Motor RPM calculations are performed during initialization so pumps run immediately with saved settings.
- **Fixed UI Display**: Asterisk indicator now correctly displays selected options across all menu categories (Unit, Gearbox, Microstep, Thread Rod).

### v4.5.x (Previous)
- Initial release with keypad and USB serial support.

## Release Notes

**v4.5.3 Improvements:**
- Safety limits enforced on all numeric inputs to prevent invalid configurations.
- Persistent settings now fully utilized—no need to re-enter parameters after power-up.
- Improved LCD feedback for user interactions.
- More robust state machine handling during menu navigation.
MIT License

### Credits
- Original code by Wladimir @ LibreHub
- Contributions by Pierre Padilla @ LibreHub
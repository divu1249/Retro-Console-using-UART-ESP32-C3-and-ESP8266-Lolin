# Hardware and Firmware Testing

## Test Environment

Record the board variants, display model, power source, and firmware revision used for testing.

## Functional Tests

| Test                       | Expected Result                      | Status   |
| -------------------------- | ------------------------------------ | -------- |
| ESP32-C3 startup           | Console initializes                  | Working  |
| TFT display                | Interface renders correctly          | Working  |
| Home screen                | Main screen displays correctly       | Working  |
| Menu navigation            | Inputs navigate the interface        | Working  |
| Snake                      | Game runs and accepts inputs         | Working  |
| 2048                       | Game runs and accepts inputs         | Working  |
| Button test                | Button inputs are detected           | Working  |
| Serial communication       | Boards exchange supported messages   | Working  |
| Buzzer                     | Supported sound commands work        | Working  |
| Battery percentage display | Not advertised as a reliable feature | Excluded |

## Known Limitation

At lower battery voltages, the ESP32-C3 may not receive sufficient initial current for consistent startup. Additional capacitors are installed, but this behavior has not been fully resolved.

## Resource Constraints

Microcontroller memory is a limiting factor for expanding the game's complexity and adding more features.

## Test Notes

Record the date, firmware revision, test conditions, and any changes whenever a test is repeated.

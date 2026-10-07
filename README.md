# Medicine Reminder System (PIC16F877A)

A medicine reminder simulation written in **PIC assembly** for the PIC16F877A. The system keeps a software clock, stores one daily reminder time for each of three medicines, and uses an LCD, three LEDs, and a buzzer to notify the user. It was developed as an educational embedded systems team project and simulated in Proteus.

## Features

- Set the current time and three medicine times with a matrix keypad (`HHMM`, then `#` to confirm; `*` to clear).
- Reject incomplete entries and invalid times such as `27:80`.
- Show the current time and next reminder on a 16x2 LCD.
- Blink the corresponding medicine LED and sound a buzzer when a reminder is due.
- Use a separate ACK button to mark the dose taken; otherwise mark it missed after 20 simulated seconds.
- Queue reminders scheduled for the same time and handle the lower medicine number first.
- Repeat the three reminders after the software clock rolls over to a new day.

## Hardware and pin connections

| Component | PIC connection | Purpose |
| --- | --- | --- |
| 4 MHz crystal | OSC1/OSC2 (pins 13/14) | PIC clock source |
| 3x4 phone keypad rows A-D | RC0-RC3 | One row is driven LOW at a time |
| Keypad columns 1-3 | RC4-RC6 | Digital inputs with external 10 kΩ pull-ups to 5 V |
| Medicine 1/2/3 LEDs | RB0/RB1/RB2 | One indicator per medicine; each has a current-limiting resistor |
| Active buzzer | RB3 through a 1 kΩ base resistor and 2N2222 driver | Turns the buzzer on during an active alarm |
| ACK button | RB4 | Active LOW, with a 10 kΩ pull-up |
| LCD RS / E | RD1 / RD2 | LCD control signals |
| LCD D4-D7 | RD4-RD7 | LCD data in 4-bit mode |

The LCD R/W pin is connected to ground. The schematic also includes a reset circuit, LCD contrast potentiometer, 5 V supply, oscillator capacitors, and decoupling capacitors. All VSS and GND connections share the same 0 V reference.

## Repository layout

```text
.
├── main.asm
├── common.inc
├── board.inc
├── LCD.inc
├── keypad.inc
├── clock.inc
├── alarm_controller.inc
├── Medicine_Reminder.X/
│   ├── Makefile
│   └── nbproject/                 # shared MPLAB X project files
├── schematic/
│   ├── MedRemainderSys.pdsprj    # final tested Proteus project
│   └── MedReminderSys.pdf        # exported circuit diagram
├── firmware/
│   └── Medicine_Reminder.X.production.hex  # copy of final tested build
├── .gitattributes
├── .gitignore
└── README.md
```

| File | Role |
| --- | --- |
| `main.asm` | Only independently assembled source; defines configuration, reset/interrupt vectors, the main loop, and module includes. |
| `common.inc` | Shared RAM addresses, pin names, states, masks, and timing constants. |
| `board.inc` | PIC port directions, digital I/O setup, unused peripheral shutdown, and RAM initialization. |
| `LCD.inc` | LCD initialization, 4-bit writes, text formatting, and incremental screen updates. |
| `keypad.inc` | Matrix scan, software debounce, and HH:MM input validation. |
| `clock.inc` | Timer2 interrupt, software clock, daily reminder queue, and next-reminder selection. |
| `alarm_controller.inc` | Welcome/setup/run states, ACK debounce, LEDs, buzzer, timeout, and result display state. |
| `Medicine_Reminder.X/` | MPLAB X project configuration. Its `build/`, `debug/`, `dist/`, and `nbproject/private/` directories are local/generated. |
| `schematic/MedRemainderSys.pdsprj` | Proteus schematic for the final tested hardware configuration. |
| `schematic/MedReminderSys.pdf` | PDF export of the complete schematic for viewing without Proteus. |
| `firmware/Medicine_Reminder.X.production.hex` | A convenient copy of the final production HEX for simulation without rebuilding. |

The `.inc` files contain PIC assembly and are included by `main.asm`; they are not separate build sources.

## Build and simulate

1. Open `Medicine_Reminder.X` in MPLAB X with a **classic MPASM** toolchain and PIC16F877A support. The project is configured for a 4 MHz oscillator.
2. Build the **production** configuration. The generated HEX is normally at `Medicine_Reminder.X/dist/default/production/Medicine_Reminder.X.production.hex`.
3. Open `schematic/MedRemainderSys.pdsprj` in Proteus. In the PIC component properties, set **Program File** to the freshly built HEX (or the copy in `firmware/`) and **Processor Clock Frequency** to `4MHz`.
4. Run the simulation. If you modify the assembly, rebuild and select the updated HEX. Update the `firmware/` copy before committing a new release.

The Proteus project was tested with a compatible DIGITAL model selection for the keypad-side pull-up components. If keypad columns remain LOW after a click, check those component model settings and the wiring before changing the keypad assembly.

## Operating the simulation

1. After reset, the LCD shows `Welcome to` / `Med Reminder` for two seconds.
2. Enter the current time as four digits. For example, enter `1230#` for 12:30. Use `*` to clear an entry.
3. Enter the reminder time for Medicine 1, then 2, then 3, confirming each with `#`.
4. The LCD shows the current time and the next medicine. At a due time, its LED blinks and the buzzer sounds.
5. Press and release the ACK button during the alarm to show `Dose Taken`. If there is no ACK within 20 simulated seconds, the result is `Dose Missed`.

When multiple medicines are due together, the next alarm starts immediately after the preceding one ends. The result message can briefly remain on the LCD while that next alarm is active; release ACK before pressing it for the next medicine.

## Timing and design

At 4 MHz, Timer2 uses `PR2=249`, prescaler 4, and postscaler 10 to generate a **10 ms interrupt**. The interrupt updates a 16-bit tick counter and advances the software clock when setup is complete. The main loop polls inputs, processes due reminders, handles the alarm, and services the LCD without a blocking countdown.

`common.inc` defaults to `FAST_DEMO=0`, where 100 ticks make one clock second and the ACK window is 2,000 ticks (20 seconds). An optional `FAST_DEMO=1` build makes the software clock advance about 60 times faster for demonstration; its ACK window is correspondingly short. Rebuild after changing this constant.

## Scope

This is an **educational simulation**, not a medical device. Its time and reminder settings are entered manually after reset; it has no battery-backed real-time clock or persistent schedule storage.

## Contributors

- [Maryam Maslamani](https://github.com/maryammaslamani)
- [Dina Otor](https://github.com/DinaOtor)
- [Doha Albitar](https://github.com/Doha178)
- [Hala Shojaia](https://github.com/halakshujaia)
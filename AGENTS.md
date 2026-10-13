# Pomodoro Timer

The `firmware/` directory contains the C-based embedded firmware for the Pomodoro Timer.
See `README.md` for the main project overview and functional logic.

We use [ch32fun](https://github.com/cnlohr/ch32fun) as a SDK, included as a Git submodule at `firmware/ch32fun/`.
To build and flash the firmware, run `make` in the firmware directory.

## Hardware Information

The device has an array of LEDs and a buzzer.

### LED configuration

- **Remaining Time**: 20 LEDs
- **Set Counter**: 4 LEDs
- **Break Indicator**: 1 LED

The remaining time and set counter LEDs are controlled by three daisy-chained 74HC595 shift registers.

The output mapping from Shift Register 1, Pin A (1A) to Shift Register 3, Pin H (3H) is as follows:

- **1A**: Set 4 indicator
- **1B**: Set 3 indicator
  ...
- **1E**: Remaining time bar (Rightmost)
  ...
- **3H**: Remaining time bar (Leftmost)

The break indicator is directory connected to a GPIO.

### Wirings

- **PC7**: Break indicator LED
- **PC6**: Shift register data (MOSI)
- **PC5**: Shift register clock (SCK)
- **PC4**: Shift register latch
- **PD4**: Buzzer

### External crystal for the clock

The external crystal is **12 MHz**, not the standard 24 MHz.

# FlightPanel

MSFS2020 client to read data from SimConnect and send data to Arduino via serial ports. Also receives serial input from Arduino and send data to game via SimConnect API.

Inspired by the [instrument project](https://github.com/scott-vincent/instrument-data-link), mostly reused the SimConnect data defs and communications code.

I mainly added Serial communication code to talk to the Arduinos, as well as Arduino input handling. (for customized trim up/down).

Arduino files are in the `Arduino` folder. I used two boards:
* Arduino Pro micro as a joystick, for the button/axis/encoder input.
  * A button matrix was used for push button/toggle switches
  * 3 Encoders and 3 analog in channels
  * Identified as a joystick by the system
  * I used a custom hardware definition when writing the board, following [this guide](http://liveelectronics.musinou.net/MIDIdeviceName.php)
  * I created a [breakout PCB](https://easyeda.com/hueyhy/arduino-breakout) for the "button" input for easier wiring.
* Arduino Uno, for driving the two servos and LEDs.


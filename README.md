# BMS-Example

Example code to read "monitoring 2" message from BMS from Dong Guan Shi Bestway Technology Co. Ltd., based on BMS Serial Communication Protocol version A5 dated 2018-12-10.

## bms-arduino-read

Requires an arduino with Serial3, such as the Arduino MEGA or Arduino DUE. The Serial3 interface is assumed to be connected to a RS232 to RS485 converter, with DE and RE connected to pin 45 on the arduino, and DI connected to pin 16 and R0 to pin 17. The serial print of voltage and current was added post haste.

## bms-python-read-display-once

Meant to be run on a computer connected with a USB to RS485 converter on `/dev/ttyUSB0`. Requires [Python3](https://www.python.org/) and [pySerial](https://pyserial.readthedocs.io/en/latest/pyserial.html) to be installed. Reads the BMS "monitoring 2" message once before exiting.


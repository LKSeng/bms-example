#!/usr/bin/env python3

import serial           # import the module
ComPort = serial.Serial('/dev/ttyUSB0', timeout=1.0) # open /dev/ttyUSB0, read timeout of 1.0s
ComPort.baudrate = 9600 # set Baud rate to 9600
ComPort.bytesize = 8    # Number of data bits = 8
ComPort.parity   = 'N'  # No parity
ComPort.stopbits = 1    # Number of Stop bits = 1

data = bytearray(b'\x7f\x10\x02\x06\x11\x58') # monitor 2 message
print(data.hex())                             # print the transmitted data

ComPort.send_break(0.1)          # Set to IDLE
No = ComPort.write(data)         # Write data, this is blocking btw

dataIn = ComPort.read(60)        # blocks till reading 60 bytes OR timeout set previously NB: mon2 message return is 27 bytes long

print(dataIn.hex())              # print the received data

# Example, print current
if len(dataIn) > 10:
  current = (dataIn[10] << 8) | dataIn[9]
  print("Battery Current[0.1A]: {}".format(current,))
# Example, print voltage
if len(dataIn) > 16:
  current = (dataIn[16] << 8) | dataIn[15]
  print("Battery Voltage[0.01mV]: {}".format(current,))

ComPort.close()                  # Close the Com port

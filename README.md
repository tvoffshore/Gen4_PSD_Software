###⚠️ COM Port Configuration Notice
When connecting to the device, select a COM port linked to the RS232/RS485 interface operating at 9600 baud. Do not use a USB COM port by default.

Note on USB COM Ports:
While USB can be used at a higher baud rate (115200), it requires disabling interface logging first. Otherwise, log messages will interfere with communication between the Device 
Assistant application and the board.

To disable logging on the USB interface, send the following command before attempting communication:

!123:LOGL=0

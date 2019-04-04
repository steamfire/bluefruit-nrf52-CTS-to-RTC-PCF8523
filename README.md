# bluefruit nrf52 CTS to RTC PCF8523

This queries the time from an iOS device over Bluetooth Low Energy (BLE) using the CTS protocol, and sets the Realtime Clock (RTC) with the obtained time.  

RTC library is from the Adafruit arduino 1.2.0 fork of the Jeelabs RTCLib.

This is a modification of the Adafruit example code for the Feather nrf52.  I have tested this with the Adalogger featherwing PCF8523 Realtime Clock, running on the nrf52832 BLE Feather.

NOTE: you MUST have a CR1220 battery in the battery holder to do anything with the RTC!  Without the battery the RTC will likely just hang the arduino on boot.  The battery can even be dead.  That's just a hardware limitation of this chip/board.

-Dan


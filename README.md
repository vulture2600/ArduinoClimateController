Arduino Climate Controller
Arduino powered climate controller for use with plant or reptile husbandry.

I developed this project to control the climate of an indoor greenhouse. This controller uses 4 relays to control up to 4 AC devices. In my greenhouse, I used a
small grow light, a small ceramic space heater with a fan, a reptile terrarium humidifier, and some 5 inch PC case fans powered by a 12vdc supply. This could easily be used to control a reptile enclosure.
Most parameters are changeable via the LCD shield buttons.


Hardware used:
Arudino Mega2560.
Dallas OneWire DS18B20 waterproof digital temp sensor.
AM2315 I2C digital temperature/humidity sensor.
Adafruit PCF8523 I2C real time clock chip.
Adafruit RGB LCD shield. 
Arduino Uno proto shield.
Velleman VMA400 4 channel relay board.
Various connectors and cables for easy disassembly.

Each of the four channels of the relay are connected to one outlet in the two gang box. Make sure to break the tabs connecting the two outlets so that each outlet
can be powered individually. Keep the neutral tabs intact. You will be switching AC loads with this setup. Use at your own risk. Make sure all connections are correct,
tight, and properly grounded. 

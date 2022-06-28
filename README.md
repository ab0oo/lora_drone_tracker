# lora_drone_tracker
Highly optimized LORA tracking system for localized tracking of fast-moving drones

This project is designed to use specific hardware for tracking R/C drones in a geographically constrained area.

On the drone side, we are using a Heltec HTCC-AB02S LoRa/GPS/OLED board (https://heltec.org/product/htcc-ab02s), configured to transmit an APRS-correct
compressed location packet every 1000ms (+/- 200ms or so).

On the ground-station side, an Adafruit LoRa Bonnet (https://www.adafruit.com/product/4074) is used to receive the location data, which is parsed and
uploaded via MQTT to a central collection server.  The data is then stored in a spatially indexed database and displayed in a near-realtime map.

This project is presented as a PlatformIO build.  Get PlatformIO here:  https://platformio.org/platformio-ide.  Edits will probably be needed in the platformio.ini file to set up the upload and monitor ports for your particular build.

Additional docs for the Python code can be found in the Python directory.



This is the python code that runs on the Adafruit LoRa Bonnet-equipped Raspberry Pi, intended to be used at the ground-station(s) in the drone-tracking project

The following python libraries are needed:
Adafruit-Blinka                  8.0.2
adafruit-circuitpython-busdevice 5.1.10
adafruit-circuitpython-framebuf  1.4.11
adafruit-circuitpython-rfm9x     2.2.7
adafruit-circuitpython-ssd1306   2.12.6
adafruit-circuitpython-typing    1.7.1
Adafruit-PlatformDetect          3.24.1
Adafruit-PureIO                  1.1.9
aprslib                          0.7.1
paho-mqtt                        1.5.1
RPi.GPIO                         0.7.0

pip is your friend for installing libraries, until I write a requirements.txt file.

The systemd .service file can be copied to the systemd config directory, so the lora GW will run at system startup:
sudo cp python/lora_mqtt_bridge.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable lora_mqtt_bridge.service
sudo systemctl start lora_mqtt_bridge.service

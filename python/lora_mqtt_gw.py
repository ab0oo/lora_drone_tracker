# SPDX-FileCopyrightText: 2018 Brent Rubell for Adafruit Industries
#
# SPDX-License-Identifier: MIT


# Import Python System Libraries
import time, sys, getopt, traceback
# Import Blinka Libraries
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
# Import the SSD1306 module.
import adafruit_ssd1306
# Import RFM9x
import adafruit_rfm9x
import aprslib
import logging
import json
import paho.mqtt.client as mqtt
# logging.basicConfig(level=logging.DEBUG) # enable this to see APRS parser debugging

freq = 913.5 ## This needs to be the same as the transmit freq on the LORA tracker
broker = ""
mqttPort = 1883
ka_interval = 45
topic = "aircraft/drones"

rxName = "HOUSE"
mqttUser = ""
mqttPass = ""

# Initialize I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# Initialize 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)
display = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, reset=reset_pin)
# Clear the display.
display.fill(0)
display.show()
width = display.width
height = display.height

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, freq)


def main(argv):
    if len(argv) < 3:
        print("Usage:")
        print('test.py -h <mqtt host> -u <mqtt user> -p <mqtt password>')
        sys.exit(1)
    try:
        opts, args = getopt.getopt(argv,"?u:p:h:",["user=","pass=","host=","help"])
    except getopt.GetoptError:
        print('test.py -h <mqtt host> -u <mqtt user> -p <mqtt password>')
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-?', "--help"):
            print('test.py -h <mqtt host> -u <mqtt user> -p <mqtt password>')
            sys.exit()
        elif opt in ("-u", "--user"):
            mqttUser = arg
        elif opt in ("-p", "--pass"):
            mqttPass = arg
        elif opt in ("-h", "--host"):
            broker = arg

    client=mqtt.Client(rxName)
    client.username_pw_set(mqttUser,mqttPass)
    client.connect(broker, port=mqttPort, keepalive=ka_interval)
    client.loop_start()

    prev_packet = None
    lastAlt=0
    lastCog=0
    lastVel=0
    print("Starting now.")
    while True:
        packet = None
        # draw a box to clear the image
        display.fill(0)
        display.text('RasPi LoRa', 35, 0, 1)
    
        uplink = {}
        uplink["receiver"] = rxName
        # check for packet rx
        packet = rfm9x.receive(keep_listening=True,with_header=True,with_ack=False,timeout=None)
        if packet is None:
            display.show()
            display.text('- Waiting for PKT -', 15, 20, 1)
        else:
            uplink["toi"] = time.time()
            uplink['rssi'] = rfm9x.last_rssi
            display.fill(0)
            prev_packet = packet
            try:
                packet_text = str(prev_packet, "utf-8")
                aprs_packet = aprslib.parse(packet_text)
                uplink["lat"] = aprs_packet["latitude"]
                uplink["lon"] = aprs_packet["longitude"]
                uplink['source'] = aprs_packet['from']
                if 'altitude' in aprs_packet:
                    uplink['alt'] = aprs_packet['altitude']
                    lastAlt = aprs_packet['altitude']
                else:
                    uplink['alt'] = lastAlt
                if 'speed' in aprs_packet:
                    uplink["vel"] = aprs_packet["speed"]
                    uplink["cog"] = aprs_packet["course"]
                    lastCog = aprs_packet["course"]
                    lastVel = aprs_packet["speed"]
                else:
                    uplink["vel"] = lastVel
                    uplink["cog"] = lastCog
                display.text('RX: ', 0, 0, 1)
                display.text(uplink['source'], 25, 0, 1)
                display.text('RSSI: ', 0, 10, 1)
                display.text(str(uplink['rssi']), 25, 10, 1)
                display.show()
                print(json.dumps(uplink))
                client.publish(topic, payload=json.dumps(uplink), qos=0, retain=False)
            except Exception as ex:
                print("Unknown packet format:  ", ex)
                print(traceback.format_exc())
    
        time.sleep(0.1)

if __name__ == "__main__":
   main(sys.argv[1:])

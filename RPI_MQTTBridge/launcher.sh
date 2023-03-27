#!/bin/sh
# launcher.sh

sleep 60

cd /
cd home/pi/HomeSensors/RPI_MQTTBridge
sudo python3 MQTTInfluxDBBridge.py
cd /

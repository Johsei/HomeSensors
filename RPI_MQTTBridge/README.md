The python script MQTTInfluxDBBridge.py is run by starting the script launcher.sh every minute with the crontab entry:
	@reboot sh /home/pi/HomeSensors/RPI_MQTTBridge/launcher.sh >/home/pi/logs/cronlog 2>&1

The code is mainly inspired by https://diyi0t.com/microcontroller-to-raspberry-pi-wifi-mqtt-communication/

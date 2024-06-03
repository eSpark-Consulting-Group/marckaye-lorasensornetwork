# LoRaWAN Sensor and Bridge Network
#### Client Name: Marc Kaye
# Softwares
1. https://www.espressif.com/en/support/download/other-tools
2. https://www.arduino.cc/en/software
# Hardware Connections
1. Connect Tx of the bridge board to the Rx(GPIO16) of the Esp32.
2. Connect GPIO27 with a push button such that one terminal connects with GPIO27 & the other connects with 3V3 of Esp32 for Start Fumigation function. On press, a telemetry will be sent to the cloud. 
3. Make sure to make all boards use common GND. Connect GND from the bridge board to the GND of the Esp32.
# Flashing Esp32
Open Flash Tool & follow the below mentioned steps in the snap to flash your esp32.
![image](https://github.com/eSpark-Consultants/marckaye-lorasensornetwork/assets/144008938/c8baa016-09e5-41c0-ab7c-bdd737cbb41f)

**NOTE: Make sure to upload the updated code present in this repo in your bridge board as well.**

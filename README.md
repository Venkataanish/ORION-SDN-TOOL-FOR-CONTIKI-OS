


              				                             ORION-MQTT
  
This version of MQTT is used to publish and subscribe to a topic and receive informtion about CC2538 open motes such as:

1). Device ID

2). On-Chip Tempterature

3). Voltage level 

4). ON/OF LED's


The steps to make the subscribing and publishing possible is given below:

1).  Install mosquitto and its clients

2).  Install cutecom

3).  Go to orion/contiki/example/orion-mqtt-sn

4).  Do make clean

5).  Connect the CC2538 open motes to the USB ports in the computer.

6).  Connect the sleep pin of the mote to the GROUND pin using a female-to-female wire connector.

7).  Reset the motes.

8).  Now remove the wire.

9).  Ensure that the FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN is changed to the value 6 in contiki/platforms/cc2538dk/contiki-conf.h

10). Do make orion-mqtt-sn.upload

11). After the compilation is over run cutecom and connect to the two respective ports to which the openmotes are connected.

12). Reset the motes

13). Now run mosquitto_sub -d -t iot-2/cmd/dev/fmt/json on one cutecom User Interface

             mosquitto_pub -h fd00::1 -m "dev" -t iot-2/evt/status/fmt/json on the second interface

14). A text file with the Device ID, On-Chip temperature and VDD level will be generated.

15). In order to exted this functionality to outside the localhost, border router functionality is required and will be seen as a future scope. 

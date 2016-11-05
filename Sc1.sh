#!/bin/bash
cd /home/user/
mosquitto &
cd org.eclipse.paho.mqtt-sn.apps/apps/MQTTSN-Gateway/src/org/eclipse/paho/mqttsn/gateway/
javac  $(find ./* | grep .java)
jar cfm Gateway.jar manifest.txt $(find ./* | grep .class)
cd ../../../../../../
java -cp src org.eclipse.paho.mqttsn.gateway.Gateway

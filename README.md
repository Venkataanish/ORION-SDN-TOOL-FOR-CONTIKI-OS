################### STEPS FOR CONSTRUCTION ###################

1. Open a new terminal and do the following:
	
         git clone https://github.com/ANRGUSC/orion.git

2. Open another terminal and do the following:
	
         ./Sc-installation.sh
	 

3. Go to contiki-2.6 folder and do the following:
	
         a. sudo -s
	 b. When prompted enter the password as "user"
	 c. Then do ./Sc1.sh
	 This will start the mosquitto broker and gateway
	
4. Open another terminal and do the following:
	
         Go to contiki-2.6 and do
	 cd examples/mqtt-sn-tools-contiki-master/
	 make

	 This will compile the entire mqtt-sn folder.
	

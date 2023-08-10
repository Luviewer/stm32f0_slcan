STM32F072C8T6 SLCAN

```sh
sudo modprobe can
sudo modprobe can-raw
sudo modprobe slcan
sudo slcand -s8 -S115200 /dev/ttyACM0 can0 # CAN Speed 8 ->1000 kBaud, 115200 Baud serial
sudo ifconfig can0 up
sudo candump can0      
or 
sudo cansend can0 123#ABCDEF   		# Send a frame to 0x123 with payload 0xABCDEF
```

```sh
-s0 = 10k
-s1 = 20k
-s2 = 50k
-s3 = 100k
-s4 = 125k
-s5 = 250k
-s6 = 500k
-s7 = 750k
-s8 = 1M
```
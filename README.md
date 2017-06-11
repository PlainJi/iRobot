# iRobot
A Robot Based On Raspberry Pi.  
It can talk with you in chinese.  
It can record with a CSI or USB camera and video format is mp4(720P 25fps)  
Real-time video can be previewed by vlc via rtsp protocol, use cmd: rtsp://192.168.1.222:554  
Record file is splited by 250MB and is named by local time.  
****
### Author:Plain Ji
### E-mail:plain_ji@163.com
****
#### prepare hardware
1. a raspberry pi
2. connect a speaker to pi via the 3.5mm jack
3. plug in the usb mic phone
#### check hardware
1. enter home dir:
	cd /home/
2. record short pcm data(ctrl+c to stop):
	arecord -D hw:1,0 -f S16_LE test.pcm
3. replay it
	aplay -f S16_LE test.pcm
4. have you here what you recorded?
#### install essential packages
./requirements.sh
#### make iRobot
make
#### execute program
./iRobot
#### other open source code url
1. https://sourceforge.net/projects/cjson/
2. https://curl.haxx.se/download/curl-7.54.0.tar.gz

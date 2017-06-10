# iRobot
A Robot Based On Raspberry Pi.

# prepare hardware
1.a raspberry pi
2.connect a speaker to pi via the 3.5mm jack
3.plug in the usb mic phone

# check hardware
1.enter home dir:
	cd /home/
2.record short pcm data(ctrl+c to stop):
	arecord -D hw:1,0 -f S16_LE test.pcm
3.replay it
	aplay -f S16_LE test.pcm
4.have you here what you recorded?

# install essential packages
./requirements.sh

# make iRobot
make

# execute program
./iRobot

# other open source code url
https://sourceforge.net/projects/cjson/
https://curl.haxx.se/download/curl-7.54.0.tar.gz

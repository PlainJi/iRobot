#ifndef __TTS_H
#define __TTS_H

//语音合成接口
#define TTS_SPD			(5)		//speed: 0~9
#define TTS_PIT			(5)		//pitch: 0~9
#define TTS_VOL			(9)		//volum: 0~9
#define TTS_PER			(0)		//0:woman, 1:man
#define TTS_API			"http://tsn.baidu.com/text2audio"
#define TTS_BODY		"tex=%s&lan=zh&ctp=1&spd=%d&pit=%d&vol=%d&per=%d&cuid=%s&tok=%s"
#define TTS_FILE		"out.mp3"

typedef struct {
	unsigned char *addr;
	unsigned int len;
}MEMORY_STRUCT;

int bd_tts(char *str);

#endif
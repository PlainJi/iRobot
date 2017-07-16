#ifndef __VOICE_H
#define __VOICE_H

#include "comdef.h"

/***********different for everyone*******************/
#define APP_CUID		"86306203908453"
#define API_KEY			"0cngDrZhiiej0Q2RNVM02Z1d"
#define SECRET_KEY		"MYGbcipDLuDAc22kQCik14z6PKpiNLFS"
#define TOKEN			"24.149e342a85babaf9fcce284eae2b361f.2592000.1495642585.282335-5492944"
/****************************************************/

//TOKEN接口
#define TOKEN_URL		"http://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s"
#define HTTP_BODY_SIZE	(2*1024)
#define VOICE_STR_LEN	(1*1024)
#define PCM_DATA_BUF	(512*1024)

//语音识别接口
#define REC_API	"http://vop.baidu.com/server_api?cuid=%s&token=%s"
int bd_voice_recognition(const char *pcmData, s32 pcmLen, char *outStr, int outLen);

//语音合成接口
#define TTS_SPD			(5)		//speed: 0~9
#define TTS_PIT			(5)		//pitch: 0~9
#define TTS_VOL			(9)		//volum: 0~9
#define TTS_PER			(1)		//0:woman, 1:man
#define TTS_API			"http://tsn.baidu.com/text2audio"
#define TTS_BODY		"tex=%s&lan=zh&ctp=1&spd=%d&pit=%d&vol=%d&per=%d&cuid=%s&tok=%s"
#define TTS_API2		"http://tts.baidu.com/text2audio"
#define TTS_BODY2		"lan=zh&pid=101&ie=UTF-8&spd=5&text=%s"
#define TTS_FILE		"out.mp3"
int bd_tts(char *inStr, int isUTF8, char *outFileName);

struct MemoryStruct {
	char *memory;
	size_t size;
};

int bd_init(void);
int bd_deinit(void);

#endif

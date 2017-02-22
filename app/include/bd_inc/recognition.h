#ifndef __RECOGNITION_H
#define __RECOGNITION_H

//语音识别接口
#define REC_API			"http://vop.baidu.com/server_api?cuid=%s&token=%s"

int bd_voice_recognition(unsigned char *pcmData, unsigned int pcmLen);

#endif
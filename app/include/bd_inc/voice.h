#ifndef __VOICE_H
#define __VOICE_H

#include "comdef.h"
#include "tts.h"
#include "recognition.h"

/***********different for everyone*******************/
#define APP_CUID		"11111111111111111111111111"
#define API_KEY			"22222222222222222222222222222222"
#define SECRET_KEY		"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
/****************************************************/

//TOKEN½Ó¿Ú
#define TOKEN_URL		"http://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s"

int bd_init(void);
int bd_deinit(void);

#endif

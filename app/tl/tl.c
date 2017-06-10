#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "curl.h"
#include "easy.h"
#include "cJSON.h"
#include "tl.h"
#include "bd.h"

extern int utf2gb(char *inbuf, size_t inlen, char *outbuf, size_t outlen);
extern int gb2utf(char *inbuf, size_t inlen, char *outbuf, size_t outlen);

//inStr:  gb2312 or gbk or utf8 string
//isUTF8: if utf8,1; if gb*,0.
//outStr: utf8 string
int tl_ask(char *inStr, int isUTF8, char *outStr, int outLen)
{
	FILE *fpp = NULL;
	char *cmd = NULL;
	char *host = NULL;
	char *result = NULL;
	cJSON *pJson = NULL;
	cJSON *pcode = NULL;
	cJSON *ptext = NULL;
	char *utfStr = NULL;
	unsigned int ret = -1;

	if(NULL == (cmd=(char*)malloc(HTTP_BODY_SIZE))) {
		SYS_ERR();
		goto exit;
	}
	if(NULL == (host=(char*)malloc(HTTP_BODY_SIZE))) {
		SYS_ERR();
		goto exit;
	}
	if(NULL == (result=(char*)malloc(VOICE_STR_LEN))) {
		SYS_ERR();
		goto exit;
	}
	if(NULL == (utfStr=(char*)malloc(VOICE_STR_LEN))) {
		SYS_ERR();
		goto exit;
	}
	
	memset(outStr, 0, outLen);
	if (isUTF8) {
		strncpy(utfStr, inStr, VOICE_STR_LEN);
	}else {
		gb2utf(inStr, strlen(inStr), utfStr, VOICE_STR_LEN);
	}
	snprintf(host, HTTP_BODY_SIZE, TL_API, TL_KEY, utfStr);
	snprintf(cmd, HTTP_BODY_SIZE, "curl -s \"%s\"", host);
	fpp = popen(cmd, "r");
	fgets(result, VOICE_STR_LEN, fpp);
	pclose(fpp);
	
	//parse token
	if(NULL == (pJson=cJSON_Parse(result))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (pcode = cJSON_GetObjectItem(pJson, "code"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (ptext = cJSON_GetObjectItem(pJson, "text"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(100000 != pcode->valueint) {
		printf("TL_ERR: ret=%d, msg=%s\n", pcode->valueint, ptext->valuestring);
		goto exit;
	}
	strncpy(outStr, ptext->valuestring, outLen);
	printf("ask: %s\nanswer:%s\n", utfStr, outStr);
	ret = 0;
	
exit:
	if(pJson) cJSON_Delete(pJson);
	if(cmd) free(cmd);
	if(host) free(host);
	if(result) free(result);
	if(utfStr) free(utfStr);
	return ret;
}



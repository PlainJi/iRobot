#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "interaction.h"

char answer[256] = {0};

extern int utf2gb(char *inbuf, size_t inlen, char *outbuf, size_t outlen);
extern int gb2utf(char *inbuf, size_t inlen, char *outbuf, size_t outlen);

int tl_ask(char *str)
{
	FILE *fpp = NULL;
	char *cmd = NULL;
	char *host = NULL;
	char *result = NULL;
	cJSON *pJson = NULL;
	cJSON *pcode = NULL;
	cJSON *ptext = NULL;
	unsigned int ret = -1;
	char utfStr[256] = {0};

	if(NULL == (cmd=(char*)malloc(256))) {
		SYS_ERR();
		goto exit;
	}
	if(NULL == (host=(char*)malloc(256))) {
		SYS_ERR();
		goto exit;
	}
	if(NULL == (result=(char*)malloc(1024))) {
		SYS_ERR();
		goto exit;
	}
	
	gb2utf(str, strlen(str), utfStr, sizeof(utfStr));
	snprintf(host, 256, TL_API, TL_KEY, utfStr);
	snprintf(cmd, 256, "curl -s \"%s\"", host);
	fpp = popen(cmd, "r");
	fgets(result, 1024, fpp);
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
	utf2gb(ptext->valuestring, strlen(ptext->valuestring), answer, sizeof(answer));
	printf("ask: %s, answer:%s\n", str, answer);
	ret = 0;
	
exit:
	if(pJson)
		cJSON_Delete(pJson);
	if(cmd)
		free(cmd);
	if(host)
		free(host);
	if(result)
		free(result);
	return ret;
}



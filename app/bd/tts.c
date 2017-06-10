#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include "curl.h"
#include "easy.h"
#include "cJSON.h"
#include "bd.h"

extern char *my_token;

//文字编码转换:从一种编码转为另一种编码
int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	memset(outbuf, 0, outlen);

	cd = iconv_open(to_charset, from_charset);
	if (cd==0) {
		return -1;
	}
	if (iconv(cd, pin, (size_t*)&inlen, pout, (size_t*)&outlen)==-1) {
		return -1;
	}
	iconv_close(cd);

	return 0;
}

//gb2utf码转为GB2312码
int utf2gb(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}

//GB2312码转为gb2utf码
int gb2utf(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct*)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) { 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void saveMP3(unsigned char *buf, unsigned int len, char *outFileName)
{
	unsigned int ret = 0;
	FILE *fp = fopen(outFileName, "wb");
	if(NULL == fp) {
		SYS_ERR();
		return;
	}
	
	ret = fwrite(buf, 1, len, fp);
	if(ret != len) {
		SYS_ERR();
		return;
	}
	
	fclose(fp);
}

//inStr: gbk or gb2312
int bd_tts(char *inStr, int isUTF8, char *outFileName)
{
	CURLcode res;
	CURL *curl = NULL;
	char *utfStr = NULL;
	char *body = NULL;
	cJSON *pJson = NULL;
	cJSON *err_msg = NULL;
	cJSON *err_no = NULL;
	int ret = -1;
	struct MemoryStruct chunk;
	memset(&chunk, 0, sizeof(chunk));
	
	if(NULL == my_token || NULL == inStr) {
		CUS_ERR("bd_tts\n");
		return ret;
	}
	if(NULL == (body = (char*)malloc(HTTP_BODY_SIZE))) {
		CUS_ERR("bd_tts malloc error!\n");
		goto exit;
	}
	if(NULL == (utfStr = (char*)malloc(VOICE_STR_LEN))) {
		CUS_ERR("bd_tts malloc error!\n");
		goto exit;
	}
	
	if(isUTF8) {
		strncpy(utfStr, inStr, VOICE_STR_LEN);
	} else {
		gb2utf(inStr, strlen(inStr), utfStr, VOICE_STR_LEN);
	}
#if 0
	snprintf(body, HTTP_BODY_SIZE, 
		TTS_BODY, utfStr , TTS_SPD, TTS_PIT, TTS_VOL, TTS_PER, APP_CUID, my_token);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, TTS_API);
#else
	snprintf(body, HTTP_BODY_SIZE, TTS_BODY2, utfStr);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, TTS_API2);
#endif
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("perform curl error:%d.\n", res);
		goto exit;
	}
	//printf("mp3 len=%u\n", chunk.size);
	
	//parse result
	if(NULL != (pJson=cJSON_Parse((const char*)chunk.memory))) {
		if(NULL == (err_no = cJSON_GetObjectItem(pJson, "err_no"))) {
			CUS_ERR(cJSON_GetErrorPtr());
			goto exit;
		}
		if(NULL == (err_msg = cJSON_GetObjectItem(pJson, "err_msg"))) {
			CUS_ERR(cJSON_GetErrorPtr());
			goto exit;
		}
		if(0 != err_no->valueint) {
			printf("tts errno: %d, %s\n", err_no->valueint, err_msg->valuestring);
			goto exit;
		}
	}
	saveMP3((unsigned char*)chunk.memory, chunk.size, outFileName);
	ret = 0;
	
exit:
	if(curl) curl_easy_cleanup(curl);
	if(pJson) cJSON_Delete(pJson);
	if(chunk.memory) free(chunk.memory);
	if(body) free(body);
	if(utfStr) free(utfStr);	
	return ret;
}













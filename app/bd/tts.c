#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "voice.h"

extern char *my_token;

size_t writefunc2(void *ptr, size_t size, size_t nmemb, void *userp)
{
	long realsize = size * nmemb;
	MEMORY_STRUCT *mem = (MEMORY_STRUCT *)userp;

	mem->addr = realloc(mem->addr, mem->len + realsize + 1);
	if(mem->addr == NULL) {
		SYS_ERR();
		return 0;
	}

	memcpy(mem->addr + mem->len, (unsigned char*)ptr, realsize);
	mem->len += realsize;
	mem->addr[mem->len] = 0;

	return realsize;
}

void saveMP3(unsigned char *buf, unsigned int len)
{
	unsigned int ret = 0;
	FILE *fp = fopen(TTS_FILE, "wb");
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

//代码转换:从一种编码转为另一种编码
int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	int rc;
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

int bd_tts(char *str)
{
	char body[256] = {0};
	char utfStr[256] = {0};
	CURL *curl;
	CURLcode res;
	cJSON *pJson = NULL;
	cJSON *err_msg = NULL;
	cJSON *err_no = NULL;
	int ret = -1;
	MEMORY_STRUCT memStruct;
	
	memset(&memStruct, 0, sizeof(MEMORY_STRUCT));
	
	if(NULL == my_token || NULL == str) {
		CUS_ERR("bd_tts\n");
		return ret;
	}
	gb2utf(str, strlen(str), utfStr, 256);
	snprintf(body, sizeof(body), 
		TTS_BODY, utfStr , TTS_SPD, TTS_PIT, TTS_VOL, TTS_PER, APP_CUID, my_token);
	
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, TTS_API);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc2);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&memStruct);
	
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("perform curl error:%d.\n", res);
		goto exit;
	}
	//printf("len=%u\n", memStruct.len);

	//parse result
	if(NULL != (pJson=cJSON_Parse((const char*)memStruct.addr))) {
		CUS_ERR("bd_tts failed!\n");
		
		if(NULL == (err_no = cJSON_GetObjectItem(pJson, "err_no"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
		}
		if(NULL == (err_msg = cJSON_GetObjectItem(pJson, "err_msg"))) {
			CUS_ERR(cJSON_GetErrorPtr());
			goto exit;
		}
		if(0 != err_no->valueint) {
			printf("ERRNO=%d, ERR_MSG=%s\n", err_no->valueint, err_msg->valuestring);
			goto exit;
		}
	}
	saveMP3(memStruct.addr, memStruct.len);
	ret = 0;
	
exit:
	if(pJson)
		cJSON_Delete(pJson);
	curl_easy_cleanup(curl);
	if(memStruct.addr)
		free(memStruct.addr);
	
	return ret;
}

















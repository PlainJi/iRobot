#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "voice.h"

extern char *my_token;

size_t writefunc1(void *ptr, size_t size, size_t nmemb, char **result)
{
	long result_len = size * nmemb;
	*result = (char *)realloc(*result, result_len + 1);
	if (*result == NULL)
	{
		CUS_ERR("realloc failure!\n");
		return -1;
	}

	memcpy(*result, ptr, result_len);
	(*result)[result_len] = '\0';

	return result_len;
}

//8K, 16bit, 1 channel
int bd_voice_recognition(unsigned char *pcmData, unsigned int pcmLen)
{
	char host[256] = {0};
	char tmp[256] = {0};
	CURL *curl;
	CURLcode res;
	char *resultBuf = NULL;
	struct curl_slist *headerlist = NULL;
	cJSON *pJson = NULL;
	cJSON *result = NULL;
	cJSON *result1 = NULL;
	cJSON *err_msg = NULL;
	cJSON *err_no = NULL;
	int arraySize = 0;
	int ret = -1;
	
	if(NULL == pcmData || NULL == my_token) {
		CUS_ERR("bd_voice_recognition\n");
		return ret;
	}
	//host
	snprintf(host, sizeof(host), REC_API, APP_CUID, my_token);
	//header
	snprintf(tmp, sizeof(tmp), "%s","Content-Type: audio/pcm; rate=8000");
	headerlist = curl_slist_append(headerlist, tmp);
	snprintf(tmp, sizeof(tmp), "Content-Length: %d", pcmLen);
	headerlist = curl_slist_append(headerlist, tmp);
	
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, host);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void*)pcmData);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, pcmLen);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBuf);
	
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("perform curl error:%d.\n", res);
		goto exit;
	}
	
	//parse result
	if(NULL == (pJson=cJSON_Parse(resultBuf))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (err_no = cJSON_GetObjectItem(pJson, "err_no"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (err_msg = cJSON_GetObjectItem(pJson, "err_msg"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (result = cJSON_GetObjectItem(pJson, "result"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(0 != err_no->valueint) {
		printf("ERRNO=%d, ERR_MSG=%s\n", err_no->valueint, err_msg->valuestring);
		goto exit;
	}
	
	arraySize = cJSON_GetArraySize(result);
	if(NULL == (result1 = cJSON_GetArrayItem(result, 0))) {
		printf("get array[0] error!\n");
		goto exit;
	}
	printf("result: %s\n", result1->valuestring);
	ret = 0;
	
exit:
	if(pJson)
		cJSON_Delete(pJson);
	curl_slist_free_all(headerlist);
	curl_easy_cleanup(curl);
	if(resultBuf)
		free(resultBuf);
	
	return ret;
}


















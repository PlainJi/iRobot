#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "curl.h"
#include "easy.h"
#include "cJSON.h"
#include "bd.h"

extern char *my_token;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

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

//8K, 16bit, 1 channel
//input:  pcm data
//output: utf8 recog result
int bd_voice_recognition(const char *pcmData, int pcmLen, char *outStr, int outLen)
{
	CURLcode res;
	CURL *curl = NULL;
	struct curl_slist *headerlist = NULL;
	char *host = NULL;
	char *tmp = NULL;
	cJSON *pJson = NULL;
	cJSON *result = NULL;
	cJSON *result1 = NULL;
	cJSON *err_msg = NULL;
	cJSON *err_no = NULL;
	int arraySize = 0;
	int ret = -1;
	struct MemoryStruct chunk;
	
	if(NULL == pcmData || NULL == my_token) {
		CUS_ERR("bd_voice_recognition\n");
		return ret;
	}

	memset(outStr, 0, outLen);
	memset(&chunk, 0, sizeof(chunk));
	if(NULL == (host = (char*)malloc(HTTP_BODY_SIZE))) {
		CUS_ERR("bd_rec malloc error!\n");
		goto exit;
	}
	if(NULL == (tmp = (char*)malloc(HTTP_BODY_SIZE))) {
		CUS_ERR("bd_rec malloc error!\n");
		goto exit;
	}
	
	//host
	snprintf(host, HTTP_BODY_SIZE, REC_API, APP_CUID, my_token);
	//header
	snprintf(tmp, HTTP_BODY_SIZE, "%s","Content-Type: audio/pcm; rate=8000");
	headerlist = curl_slist_append(headerlist, tmp);
	snprintf(tmp, HTTP_BODY_SIZE, "Content-Length: %d", pcmLen);
	headerlist = curl_slist_append(headerlist, tmp);

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, host);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); 
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pcmData);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, pcmLen);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);		//debug mode
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("perform curl error:%d.\n", res);
		goto exit;
	}
	
	//parse result
	if(NULL == (pJson=cJSON_Parse((const char*)chunk.memory))) {
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
	if(0 != err_no->valueint) {
		printf("recognize error: %d %s\n", err_no->valueint, err_msg->valuestring);
		goto exit;
	}
	if(NULL == (result = cJSON_GetObjectItem(pJson, "result"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	
	arraySize = cJSON_GetArraySize(result);
	if(NULL == (result1 = cJSON_GetArrayItem(result, 0))) {
		printf("get recognize result array[0] error!\n");
		goto exit;
	}
	//printf("result: %s\n", result1->valuestring);
	strncpy(outStr, result1->valuestring, outLen);
	ret = 0;
	
exit:
	if(pJson) cJSON_Delete(pJson);
	if(chunk.memory) free(chunk.memory);
	if(host) free(host);
	if(tmp) free(tmp);
	if(headerlist) curl_slist_free_all(headerlist);
	if(curl) curl_easy_cleanup(curl);
	curl_global_cleanup();
	
	return ret;
}















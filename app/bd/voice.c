#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "voice.h"

char *my_token = NULL;

unsigned int getToken(void)
{
	FILE *fpp = NULL;
	char *cmd = NULL;
	char *host = NULL;
	char *result = NULL;
	cJSON *pJson = NULL;
	cJSON *psub = NULL;
	unsigned int ret = -1;
	
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
	
	//get token
	snprintf(host, 256, TOKEN_URL, API_KEY, SECRET_KEY);
	snprintf(cmd, 256, "curl -s \"%s\"", host);
	fpp = popen(cmd, "r");
	fgets(result, 1024, fpp);
	pclose(fpp);
	
	//parse token
	if(NULL == (pJson=cJSON_Parse(result))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	if(NULL == (psub = cJSON_GetObjectItem(pJson, "access_token"))) {
		CUS_ERR(cJSON_GetErrorPtr());
		goto exit;
	}
	
	//printf("token: %s\n", psub->valuestring);
	my_token = malloc(strlen(psub->valuestring)+1);
	stpcpy(my_token, psub->valuestring);
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

int bd_init(void)
{
	if(getToken()) {
		printf("--- bd init Failed!\n");
		return -1;
	}
	
	printf("+++ bd  OK\n");
	return 0;
}

int bd_deinit(void)
{
	if(my_token)
		free(my_token);
	
	printf("+++ bd closed\n");
	return 0;
}













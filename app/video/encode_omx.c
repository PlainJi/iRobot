#include "encode_omx.h"

static ENC_HANDLE *handle;
static ENC_SPSPPS encSPSPPS;

int saveSPSorPPS(unsigned char *p, int len)
{
	int i = 0;
	unsigned char type = 0;
	unsigned char *pSPS = NULL;
	unsigned char *pPPS = NULL;
	unsigned char temp[4] = {0, 0, 0, 1};
	
	if(!encSPSPPS.init){
		type = (*(p+4))&0x1f;
		if( 7==type ){
			pSPS = p;
			while((++i)<(len-4)){
				if(!memcmp(temp, p+i, 4))
					break;
			}
			if(i<(len-4))
				pPPS = p+i;

			if(pSPS && pPPS){
				encSPSPPS.spsLen = (int)(pPPS-pSPS);
				encSPSPPS.ppsLen = (int)(pSPS+len-pPPS);
				memcpy(&encSPSPPS.privateSPS, pSPS, encSPSPPS.spsLen);
				memcpy(&encSPSPPS.privatePPS, pPPS, encSPSPPS.ppsLen);
				encSPSPPS.init = 1;
			}
		}
	}

	return 0;
}

int get_SPS(u8 **p, u32 *len)
{
	if(!encSPSPPS.init) {
		*len = 0;
		*p = NULL;
		return -1;
	}

	*p = encSPSPPS.privateSPS;
	*len = encSPSPPS.spsLen;
	
	return 0;
}

int get_PPS(u8 **p, u32 *len)
{
	if(!encSPSPPS.init) {
		*len = 0;
		*p = NULL;
		return -1;
	}

	*p = encSPSPPS.privatePPS;
	*len = encSPSPPS.ppsLen;
	
	return 0;
}

int get_spspps_state()
{
	return encSPSPPS.init;
}

static void print_def(OMX_PARAM_PORTDEFINITIONTYPE def)
{
	printf("+++ Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u %u %u\n",
			def.nPortIndex, def.eDir == OMX_DirInput ? "in" : "out",
			def.nBufferCountActual, def.nBufferCountMin, def.nBufferSize,
			def.nBufferAlignment, def.bEnabled ? "enabled" : "disabled",
			def.bPopulated ? "populated" : "not pop.",
			def.bBuffersContiguous ? "contig." : "not cont.",
			def.format.video.nFrameWidth, def.format.video.nFrameHeight,
			def.format.video.nStride, def.format.video.nSliceHeight,
			def.format.video.xFramerate>>16, def.format.video.eColorFormat);
}

ENC_HANDLE *encode_open(ENC_PARAM param)
{
	handle = malloc(sizeof(ENC_HANDLE));
	if (!handle){
		printf("--- malloc enc handle failed\n");
		return NULL;
	}
	CLEAR(*handle);

	handle->EncodeBuf.buf = (char *)malloc(ENCODE_BUF_SIZE);
	if(!handle->EncodeBuf.buf){
		printf("--- malloc EncodeBuf failed\n");
		if(handle->EncodeBuf.buf)
			free(handle->EncodeBuf.buf);
	}
	memset(handle->EncodeBuf.buf, 0, ENCODE_BUF_SIZE);
	
	handle->video_encode = NULL;
	memset(handle->list, 0, sizeof(handle->list));
	handle->client = NULL;
	handle->out = NULL;
	handle->frame_counter = 0;
	handle->params.src_picwidth = param.src_picwidth;
	handle->params.src_picheight = param.src_picheight;
	handle->params.enc_picwidth = param.enc_picwidth;
	handle->params.enc_picheight = param.enc_picheight;
	handle->params.fps = param.fps;
	handle->params.bitrate = param.bitrate;
	handle->params.gop = param.gop;
	handle->params.chroma_interleave = param.chroma_interleave;

	bcm_host_init();

	if ((handle->client = ilclient_init()) == NULL){
		printf("--- ilclient_init failed\n");
		goto err0;
	}
	if (OMX_Init() != OMX_ErrorNone){
		printf("--- OMX_Init failed\n");
		goto err1;
	}
	
	// create video encoder
	OMX_ERRORTYPE ret;
	ret = ilclient_create_component(handle->client, &handle->video_encode, "video_encode", 
			ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS| ILCLIENT_ENABLE_OUTPUT_BUFFERS);
	if (ret != 0){
		printf("--- ilclient_create_component failed\n");
		goto err2;
	}
	handle->list[0] = handle->video_encode;
	
	//get current settings of video_encode component from input port
	OMX_PARAM_PORTDEFINITIONTYPE def;
	memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	def.nVersion.nVersion = OMX_VERSION;
	def.nPortIndex = OMX_VIDENC_INPUT_PORT;
	if (OMX_GetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamPortDefinition, &def) != OMX_ErrorNone){
		printf("--- OMX_GetParameter for video encode input port failed\n");
		goto err3;
	}
	print_def(def);
	def.format.video.nFrameWidth = handle->params.src_picwidth;
	def.format.video.nFrameHeight = handle->params.src_picheight;
	def.format.video.xFramerate = handle->params.fps << 16;
	def.format.video.nSliceHeight = def.format.video.nFrameHeight;
	def.format.video.nStride = def.format.video.nFrameWidth;
	def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	print_def(def);
	ret = OMX_SetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamPortDefinition, &def);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_SetParameter for video encode input port failed with %x\n", ret);
		goto err3;
	}
	
	// set output format
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;
	ret = OMX_SetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoPortFormat, &format);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_SetParameter for video encode output port failed with: %x\n", ret);
		goto err3;
	}
	
	// set output bitrate
	OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
	memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
	bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
	bitrateType.nVersion.nVersion = OMX_VERSION;
	bitrateType.eControlRate = OMX_Video_ControlRateVariable;
	bitrateType.nTargetBitrate = handle->params.bitrate * 1000;    // to kbps
	bitrateType.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	ret = OMX_SetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoBitrate, &bitrateType);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_SetParameter for bitrate for video encode output port failed with %x\n", ret);
		goto err3;
	}
	
	// get current bitrate
	memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
	bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
	bitrateType.nVersion.nVersion = OMX_VERSION;
	bitrateType.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	if (OMX_GetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoBitrate, &bitrateType) != OMX_ErrorNone){
		printf("%s:%d: OMX_GetParameter() for video_encode for bitrate output port failed!\n", __FUNCTION__, __LINE__);
		goto err3;
	}
	printf("+++ Bitrate %u\n", bitrateType.nTargetBitrate);
	
	// set SPS/PPS on each I-frame
	OMX_CONFIG_PORTBOOLEANTYPE inlineHeader;
	memset(&inlineHeader, 0, sizeof(OMX_CONFIG_PORTBOOLEANTYPE));
	inlineHeader.nSize = sizeof(OMX_CONFIG_PORTBOOLEANTYPE);
	inlineHeader.nVersion.nVersion = OMX_VERSION;
#ifdef ENABLE_INLINE_HEADER
	inlineHeader.bEnabled = OMX_TRUE;	 // Enable SPS/PPS insertion in the encoder bitstream
#else
	inlineHeader.bEnabled = OMX_FALSE;	  // Enable SPS/PPS insertion in the encoder bitstream
	memset(&encSPSPPS, 0, sizeof(encSPSPPS));
#endif
	inlineHeader.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	ret = OMX_SetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlineHeader);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_SetParameter for inline heander failed\n");
		goto err3;
	}
	ret = OMX_GetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlineHeader);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_GetParameter for inline heander failed\n");
		goto err3;
	}
	printf("+++ SPS PPS bEnabled = %d\n", inlineHeader.bEnabled);
	
	// get current settings of avcConfig
	OMX_VIDEO_PARAM_AVCTYPE avcConfig;
	memset(&avcConfig, 0, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
	avcConfig.nSize = sizeof(OMX_VIDEO_PARAM_AVCTYPE);
	avcConfig.nVersion.nVersion = OMX_VERSION;
	avcConfig.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	if (OMX_GetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoAvc, &avcConfig) != OMX_ErrorNone){
		printf("--- %s:%d OMX_GetParameter for avcConfig failed\n", __FUNCTION__, __LINE__);
		goto err3;
	}
	
	// set new avcConfig
	avcConfig.nPFrames = handle->params.gop - 1;
	ret = OMX_SetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoAvc, &avcConfig);
	if (ret != OMX_ErrorNone){
		printf("--- OMX_SetParameter for avcConfig failed\n");
		goto err3;
	}
	
	// get and check avcConfig again
	memset(&avcConfig, 0, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
	avcConfig.nSize = sizeof(OMX_VIDEO_PARAM_AVCTYPE);
	avcConfig.nVersion.nVersion = OMX_VERSION;
	avcConfig.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	if (OMX_GetParameter(ILC_GET_HANDLE(handle->video_encode), OMX_IndexParamVideoAvc, &avcConfig) != OMX_ErrorNone){
		printf("--- %s:%d OMX_GetParameter for avcConfig failed\n", __FUNCTION__, __LINE__);
		goto err3;
	}
	printf("+++ nPFrames set to: %u\n", avcConfig.nPFrames);

	// change il state
	if (ilclient_change_component_state(handle->video_encode, OMX_StateIdle) == -1){
		printf("--- %s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed", __FUNCTION__, __LINE__);
		goto err3;
	}
	if (ilclient_enable_port_buffers(handle->video_encode, OMX_VIDENC_INPUT_PORT, NULL, NULL, NULL) != 0){
		printf("--- enabling port buffers for input failed!\n");
		goto err4;
	}
	if (ilclient_enable_port_buffers(handle->video_encode, OMX_VIDENC_OUTPUT_PORT, NULL, NULL, NULL) != 0){
		printf("--- enabling port buffers for output failed!\n");
		goto err5;
	}
	if (ilclient_change_component_state(handle->video_encode, OMX_StateExecuting) == -1){
		printf("--- %s:%d: ilclient_change_component_state(video_encode, OMX_StateExecuting) failed", __FUNCTION__, __LINE__);
		goto err6;
	}
	printf("+++ Encode Opened\n");
	return handle;

	err6: ilclient_disable_port_buffers(handle->video_encode,
	OMX_VIDENC_OUTPUT_PORT, NULL, NULL, NULL);
	err5: ilclient_disable_port_buffers(handle->video_encode,
	OMX_VIDENC_INPUT_PORT, NULL, NULL, NULL);
	err4: ilclient_state_transition(handle->list, OMX_StateIdle);
	ilclient_state_transition(handle->list, OMX_StateLoaded);
	err3: ilclient_cleanup_components(handle->list);
	err2: OMX_Deinit();
	err1: ilclient_destroy(handle->client);
	err0: free(handle);
	return NULL;
}

void encode_close(ENC_HANDLE *handle)
{
	if(NULL == handle) {
		return;
	}
	ilclient_disable_port_buffers(handle->video_encode, OMX_VIDENC_INPUT_PORT, NULL, NULL, NULL);
	ilclient_disable_port_buffers(handle->video_encode, OMX_VIDENC_OUTPUT_PORT, NULL, NULL, NULL);
	ilclient_state_transition(handle->list, OMX_StateIdle);
	ilclient_state_transition(handle->list, OMX_StateLoaded);
	ilclient_cleanup_components(handle->list);
	OMX_Deinit();
	ilclient_destroy(handle->client);

	if(handle->EncodeBuf.buf)
		free(handle->EncodeBuf.buf);
	free(handle);
	printf("+++ Encode Closed\n");
}

int encode_one_frame(ENC_HANDLE *handle, void *ibuf, int ilen, void **pobuf, int *polen, PIC_TYPE *type)
{
	int len = 0;
	int flag = 0;
	OMX_ERRORTYPE ret;
	OMX_BUFFERHEADERTYPE *buf;

	buf = ilclient_get_input_buffer(handle->video_encode, OMX_VIDENC_INPUT_PORT, 0);
	if (buf == NULL){
		buf = ilclient_get_input_buffer_sec(handle->video_encode, OMX_VIDENC_INPUT_PORT, 0);
		if(buf == NULL){
			printf("!!! no buffers for me!\n");
			goto Exit;
		}
	}

	memcpy(buf->pBuffer, ibuf, ilen);
	buf->nFilledLen = ilen;

	if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(handle->video_encode), buf) != OMX_ErrorNone){
		printf("!!! Error emptying buffer\n");
		goto Exit;
	}

	handle->out = ilclient_get_output_buffer(handle->video_encode, OMX_VIDENC_OUTPUT_PORT, 1);
	ret = OMX_FillThisBuffer(ILC_GET_HANDLE(handle->video_encode), handle->out);
	if (ret != OMX_ErrorNone){
		printf("!!! Error filling buffer: %x\n", ret);
		goto Exit;
	}

	if (handle->out == NULL){
		printf("!!! not getting out\n");
		goto Exit;
	}
	if (handle->out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
		flag = 1;
	}

	memcpy(handle->EncodeBuf.buf, handle->out->pBuffer, handle->out->nFilledLen);
	len += handle->out->nFilledLen;
	while(!(handle->out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)) {		//not end of frame
		ret = OMX_FillThisBuffer(ILC_GET_HANDLE(handle->video_encode), handle->out);
		if (ret != OMX_ErrorNone) {
			printf("!!! Error filling buffer: %x\n", ret);
			goto Exit;
		}

		if (handle->out == NULL) {
			printf("!!! not getting out\n");
			goto Exit;
		}
		memcpy(handle->EncodeBuf.buf+len, handle->out->pBuffer, handle->out->nFilledLen);
		len += handle->out->nFilledLen;
	}

	if(flag) {
		saveSPSorPPS((unsigned char*)handle->EncodeBuf.buf, len);
	}
	*type = handle->EncodeBuf.buf[4] & 0x1f;
	*pobuf = handle->EncodeBuf.buf;
	*polen = len;
	handle->frame_counter++;
	handle->out->nFilledLen = 0;    // set to 0 at end
	return 0;
	
Exit:
	*pobuf = NULL;
	*type = NONE;
	*polen = 0;
	return -1;
}

void encode_force_I_Frame(void)
{
	OMX_CONFIG_BOOLEANTYPE requestIFrame;
	memset(&requestIFrame, 0, sizeof(OMX_CONFIG_BOOLEANTYPE));
	requestIFrame.nSize = sizeof(OMX_CONFIG_BOOLEANTYPE);
	requestIFrame.nVersion.nVersion = OMX_VERSION;
	requestIFrame.bEnabled = OMX_TRUE;		// this automatically resets itself.

	OMX_ERRORTYPE ret = OMX_SetConfig(ILC_GET_HANDLE(handle->video_encode), 
		OMX_IndexConfigBrcmVideoRequestIFrame, &requestIFrame);
	if (ret != OMX_ErrorNone) {
		printf("--- failed to force IPic\n");
	}
	printf("Force An I-Frame.\n");
	
	return;
}

/*
int encode_get_headers(ENC_HANDLE *handle, void **pbuf, int *plen, PIC_TYPE *type)
{
	UNUSED(handle);

	*pbuf = NULL;
	*plen = 0;
	*type = NONE;

	return 0;
}

int encode_set_qp(ENC_HANDLE *handle, int val)
{
	UNUSED(handle);
	UNUSED(val);
	printf("*** %s.%s: This function is not implemented\n", __FILE__, __FUNCTION__);
	return -1;
}

int encode_set_gop(ENC_HANDLE *handle, int val)
{
	UNUSED(handle);
	UNUSED(val);
	printf("*** %s.%s: This function is not implemented\n", __FILE__, __FUNCTION__);
	return -1;
}

int encode_set_bitrate(ENC_HANDLE *handle, int val)
{
	OMX_VIDEO_CONFIG_BITRATETYPE tbitrate;
	memset(&tbitrate, 0, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
	tbitrate.nSize = sizeof(OMX_VIDEO_CONFIG_BITRATETYPE);
	tbitrate.nVersion.nVersion = OMX_VERSION;
	tbitrate.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	tbitrate.nEncodeBitrate = val * 1000;
	OMX_ERRORTYPE ret = OMX_SetConfig(ILC_GET_HANDLE(handle->video_encode), OMX_IndexConfigVideoBitrate, &tbitrate);
	
	if (ret != OMX_ErrorNone){
		printf("--- failed to set bitrate to %d kbps\n", val);
		return -1;
	}
	printf("+++ Set bitrate to %d kbps\n", val);

	return 0;
}

int encode_set_framerate(ENC_HANDLE *handle, int val)
{
	OMX_CONFIG_FRAMERATETYPE framerate;
	memset(&framerate, 0, sizeof(OMX_CONFIG_FRAMERATETYPE));
	framerate.nSize = sizeof(OMX_CONFIG_FRAMERATETYPE);
	framerate.nVersion.nVersion = OMX_VERSION;
	framerate.nPortIndex = OMX_VIDENC_OUTPUT_PORT;
	framerate.xEncodeFramerate = val;
	OMX_ERRORTYPE ret = OMX_SetConfig(ILC_GET_HANDLE(handle->video_encode), OMX_IndexConfigVideoFramerate, &framerate);
	
	if (ret != OMX_ErrorNone){
		printf("--- failed to set framerate to %d\n", val);
		return -1;
	}
	printf("--- Set framerate to %d\n", val);
	
	return 0;
}
*/


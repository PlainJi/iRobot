#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include "include/pack.h"
#include "include/encode.h"

extern struct enc_SPSPPS encSPSPPS;

u64 get_current_millisec(void)
{
	struct timeb tb;
	ftime(&tb);
	return 1000ULL * tb.time + tb.millitm;
}

struct pac_handle *pack_open(struct pac_param params)
{
    struct pac_handle *handle = (struct pac_handle *) malloc(
            sizeof(struct pac_handle));
    if (!handle) return NULL;

    CLEAR(*handle);
    handle->FU_index = 0;
    handle->nalu.data = NULL;
    handle->seq_num = 2;
    handle->ts_current_sample = 0;
    handle->params.max_pkt_len = params.max_pkt_len;
    handle->params.ssrc = params.ssrc;
    handle->ts_start_millisec = get_current_millisec();	// save the startup time

    printf("+++ Pack Opened\n");
    return handle;
}

void pack_close(struct pac_handle *handle)
{
	free(handle);
	printf("+++ Pack Closed\n");
}

void pack_put(struct pac_handle *handle, void *inbuf, int isize)
{
    handle->inbuf = inbuf;
    handle->next_nalu_ptr = handle->inbuf;
    handle->inbuf_size = isize;
    handle->FU_counter = 0;
    handle->last_FU_size = 0;
    handle->FU_index = 0;
    handle->inbuf_complete = 0;
    handle->nalu_complete = 1;    // start a new nalu
}

static int is_start_code4(char *buf)
{
    if (buf[0] != 0 || buf[1] != 0 || buf[2] != 0 || buf[3] != 1)    // 0x00000001
        return 0;
    else
        return 1;
}

static int is_start_code3(char *buf)
{
    if (buf[0] != 0 || buf[1] != 0 || buf[2] != 1)    // 0x000001
        return 0;
    else
        return 1;
}

int get_next_nalu(struct pac_handle *handle)
{
    if (!handle->next_nalu_ptr)    // reach data end, no next nalu
    {
        return 0;
    }

    if (is_start_code3(handle->next_nalu_ptr))    // check 0x000001 first
    {
        handle->nalu.startcodeprefix_len = 3;
    }
    else
    {
        if (is_start_code4(handle->next_nalu_ptr))    // check 0x00000001
        {
            handle->nalu.startcodeprefix_len = 4;
        }
        else
        {
            printf("!!! No any startcode found\n");
            return -1;
        }
    }

    // found the next start code
    int startcode_found = 0;
    char *cur_nalu_ptr = handle->next_nalu_ptr;    // rotate, save the next ptr
    char *next_ptr = cur_nalu_ptr + handle->nalu.startcodeprefix_len;    // skip current start code
    while (!startcode_found)
    {
        next_ptr++;

        if ((next_ptr - (char *) handle->inbuf) >= handle->inbuf_size)    // reach data end
        {
            handle->next_nalu_ptr = NULL;    // no more nalus
            break;
        }

        if (is_start_code3(next_ptr) || is_start_code4(next_ptr))    // found the next start code
        {
            handle->next_nalu_ptr = next_ptr;    // next ptr
            break;
        }
    }

    handle->nalu.data = cur_nalu_ptr + handle->nalu.startcodeprefix_len;    // exclude the start code
    handle->nalu.len = next_ptr - cur_nalu_ptr
            - handle->nalu.startcodeprefix_len;
    handle->nalu.forbidden_bit = (handle->nalu.data[0] & 0x80) >> 7;    // 1 bit, 0b1000 0000
    handle->nalu.nal_reference_idc = (handle->nalu.data[0] & 0x60) >> 5;    // 2 bit, 0b0110 0000
    handle->nalu.nal_unit_type = (handle->nalu.data[0] & 0x1f);    // 5 bit, 0b0001 1111

    return 1;
}

int pack_get(struct pac_handle *handle, void *outbuf, int bufsize, int *outsize)
{
    int ret;

    if (handle->inbuf_complete) return 0;

    memset(outbuf, 0, bufsize);    // !!! this is important, missing this may cause werid problems, like VLC displays nothing when the buf is small!
    char *tmp_outbuf = (char *) outbuf;
    // set common rtp header
    rtp_header *rtp_hdr;
    rtp_hdr = (rtp_header *) tmp_outbuf;
    rtp_hdr->payload = H264;
    rtp_hdr->version = 2;
    rtp_hdr->marker = 0;
    rtp_hdr->ssrc = htonl(handle->params.ssrc);    // constant for a RTP session

    if (handle->nalu_complete)    // current nalu complete, find the next nalu in inbuf
    {
        ret = get_next_nalu(handle);
        if (ret <= 0)    // no more nalus
        {
            handle->inbuf_complete = 1;
            return 0;
        }

        rtp_hdr->seq_no = htons(handle->seq_num++);    // increase for every RTP packet
        handle->ts_current_sample = (u32) ((get_current_millisec() - handle->ts_start_millisec) * 90);    // calculate the timestamp for a new NALU
        rtp_hdr->timestamp = htonl(handle->ts_current_sample);
        // handle the new NALU
        if (handle->nalu.len <= handle->params.max_pkt_len)    // no need to fragment
        {
            rtp_hdr->marker = 1;
            nalu_header *nalu_hdr;
            nalu_hdr = (nalu_header *) (tmp_outbuf + 12);
            nalu_hdr->F = handle->nalu.forbidden_bit;
            nalu_hdr->NRI = handle->nalu.nal_reference_idc;
            nalu_hdr->TYPE = handle->nalu.nal_unit_type;
            char *nalu_payload = tmp_outbuf + 13;    // 12 Bytes RTP header + 1 Byte NALU header
            *outsize = handle->nalu.len + 12;
            if (bufsize < *outsize)    // check size
            {
                printf("--- buffer size %d < pack size %d\n", bufsize,
                        *outsize);
                abort();
            }
            memcpy(nalu_payload, handle->nalu.data + 1, handle->nalu.len - 1);    // exclude the nalu header

            handle->nalu_complete = 1;
            return 1;
        }
        else    // fragment needed
        {
            // calculate the fragments
            if (handle->nalu.len % handle->params.max_pkt_len == 0)    // in case divide exactly
            {
                handle->FU_counter = handle->nalu.len
                        / handle->params.max_pkt_len - 1;
                handle->last_FU_size = handle->params.max_pkt_len;
            }
            else
            {
                handle->FU_counter = handle->nalu.len
                        / handle->params.max_pkt_len;
                handle->last_FU_size = handle->nalu.len
                        % handle->params.max_pkt_len;
            }
            handle->FU_index = 0;

            // it's the first FU
            rtp_hdr->marker = 0;
            fu_indicator *fu_ind = (fu_indicator *) (tmp_outbuf + 12);
            fu_ind->F = handle->nalu.forbidden_bit;
            fu_ind->NRI = handle->nalu.nal_reference_idc;
            fu_ind->TYPE = 28;    // FU_A

            fu_header *fu_hdr = (fu_header *) (tmp_outbuf + 13);
            fu_hdr->E = 0;
            fu_hdr->R = 0;
            fu_hdr->S = 1;    // start bit
            fu_hdr->TYPE = handle->nalu.nal_unit_type;
            char *nalu_payload = tmp_outbuf + 14;
            *outsize = handle->params.max_pkt_len + 14;    // RTP header + FU indicator + FU header
            if (bufsize < *outsize)
            {
                printf("--- buffer size %d < pack size %d\n", bufsize,
                        *outsize);
                abort();
            }
            memcpy(nalu_payload, handle->nalu.data + 1,
                    handle->params.max_pkt_len);

            handle->nalu_complete = 0;    // not complete
            handle->FU_index++;

            return 1;
        }
    }
    else    // send remaining FUs
    {
        rtp_hdr->seq_no = htons(handle->seq_num++);
        rtp_hdr->timestamp = htonl(handle->ts_current_sample);    // it's a continuation to the last NALU, no need to recalculate

        // check if it's the last FU
        if (handle->FU_index == handle->FU_counter)    // the last FU
        {
            rtp_hdr->marker = 1;    // the last FU
            fu_indicator *fu_ind = (fu_indicator *) (tmp_outbuf + 12);
            fu_ind->F = handle->nalu.forbidden_bit;
            fu_ind->NRI = handle->nalu.nal_reference_idc;
            fu_ind->TYPE = 28;

            fu_header *fu_hdr = (fu_header *) (tmp_outbuf + 13);
            fu_hdr->R = 0;
            fu_hdr->S = 0;
            fu_hdr->TYPE = handle->nalu.nal_unit_type;
            fu_hdr->E = 1;    // the last EU
            char *nalu_payload = tmp_outbuf + 14;
            *outsize = handle->last_FU_size - 1 + 14;
            if (bufsize < *outsize)
            {
                printf("--- buffer size %d < pack size %d\n", bufsize,
                        *outsize);
                abort();
            }
            memcpy(nalu_payload,
                    handle->nalu.data + 1
                            + handle->FU_index * handle->params.max_pkt_len,
                    handle->last_FU_size - 1);    // minus the nalu header

            handle->nalu_complete = 1;    // this nalu is complete
            handle->FU_index = 0;
            return 1;
        }
        else    // middle FUs
        {
            rtp_hdr->marker = 0;
            fu_indicator *fu_ind = (fu_indicator *) (tmp_outbuf + 12);
            fu_ind->F = handle->nalu.forbidden_bit;
            fu_ind->NRI = handle->nalu.nal_reference_idc;
            fu_ind->TYPE = 28;

            fu_header *fu_hdr = (fu_header *) (tmp_outbuf + 13);
            fu_hdr->R = 0;
            fu_hdr->S = 0;
            fu_hdr->TYPE = handle->nalu.nal_unit_type;
            fu_hdr->E = 0;

            char *nalu_payload = tmp_outbuf + 14;
            *outsize = handle->params.max_pkt_len + 14;
            if (bufsize < *outsize)
            {
                printf("--- buffer size %d < pack size %d\n", bufsize,
                        *outsize);
                abort();
            }
            memcpy(nalu_payload,
                    handle->nalu.data + 1
                            + handle->FU_index * handle->params.max_pkt_len,
                    handle->params.max_pkt_len);

            handle->FU_index++;
            return 1;
        }
    }
}

int getNetSPS(char *buf)
{
	rtp_header rtp_hdr;

	if(!encSPSPPS.init)
	{
		return 0;
	}
	memset(&rtp_hdr, 0, sizeof(rtp_hdr));
	rtp_hdr.payload = H264;
    rtp_hdr.version = 2;
    rtp_hdr.marker = 1;
	rtp_hdr.seq_no = htons(0);
	rtp_hdr.ssrc = htonl(0x11223344);
	rtp_hdr.timestamp = htonl(0);
	memcpy(buf, &rtp_hdr, 12);
	memcpy(buf+12, encSPSPPS.privateSPS+4, encSPSPPS.spsLen-4);

	return (12+encSPSPPS.spsLen-4);
}

int getNetPPS(char *buf)
{
	rtp_header rtp_hdr;

	if(!encSPSPPS.init)
	{
		return 0;
	}
	memset(&rtp_hdr, 0, sizeof(rtp_hdr));
	rtp_hdr.payload = H264;
    rtp_hdr.version = 2;
    rtp_hdr.marker = 1;
	rtp_hdr.seq_no = htons(1);
	rtp_hdr.ssrc = htonl(0x11223344);
	rtp_hdr.timestamp = htonl(0);
	memcpy(buf, &rtp_hdr, 12);
	memcpy(buf+12, encSPSPPS.privatePPS+4, encSPSPPS.ppsLen-4);

	return (12+encSPSPPS.ppsLen-4);
}


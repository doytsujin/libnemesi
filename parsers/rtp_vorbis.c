/* * 
 * This file is part of libnemesi
 *
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 * 
 * libnemesi is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libnemesi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libnemesi; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include "rtpparser.h"
#include "rtp_xiph.h"
#include "rtp_utils.h"
#include <math.h>

/**
 * @file rtp_vorbis.c
 * Vorbis depacketizer - draft 08
 */

/**
 * Local structure, contains data necessary to compose a vorbis frame out
 * of rtp fragments and set the correct timings.
 */

typedef struct {
    long offset;    //!< offset across an aggregate rtp packet
    int pkts;       //!< number of packets yet to process in the aggregate
    char *buf;      //!< constructed frame, fragments will be copied there 
    long len;       //!< buf length, it's the sum of the fragments length
    long timestamp; //!< calculated timestamp
    int id;         //!< Vorbis id, it could change across packets.
    rtp_xiph_conf *conf;        //!< configuration list
    int conf_len;
    int modes;                  //!< Internal vorbis data
    long blocksizes[2];         //!< Internal vorbis data
    int param_blockflag[64];    //!< Internal vorbis data
    long prev_bs;               //!< Internal vorbis data
    long curr_bs;               //!< Internal vorbis data
} rtp_vorbis;

static rtpparser_info vorbis_served = {
    -1,
    {"audio/vorbis", NULL}
};

//helpers
static int rtp_ilog(unsigned int v)
{
    int ret = 0;
    while (v) {
        ret++;
        v >>= 1;
    }
    return (ret);
}

static int icount(unsigned int v)
{
    int ret = 0;
    while (v) {
        ret += v & 1;
        v >>= 1;
    }
    return (ret);
}

static long maptype_quantvals(int entries, int dim)
{
    long vals = floor(pow((float) entries, 1.f / dim));

    /* the above *should* be reliable, but we'll not assume that FP is
       ever reliable when bitstream sync is at stake; verify via integer
       means that vals really is the greatest value of dim for which
       vals^b->bim <= b->entries */
    /* treat the above as an initial guess */
    while (1) {
        long acc = 1;
        long acc1 = 1;
        int i;
        for (i = 0; i < dim; i++) {
            acc *= vals;
            acc1 *= vals + 1;
        }
        if (acc <= entries && acc1 > entries) {
            return (vals);
        } else {
            if (acc > entries) {
                vals--;
            } else {
                vals++;
            }
        }
    }
}

static int cfg_parse(rtp_vorbis * vorb, uint8_t *data, int len, int off) 
{
    bit_context opb;
    int num, i, j, k = -1, channels;

    //id packet
    vorb->blocksizes[0] = 1 << (data[28] & 0x0f);
    vorb->blocksizes[1] = 1 << (data[28] >> 4);
    channels = data[11];
    //cb packet
    bit_readinit(&opb, data + off, len - off);
    if (5 != bit_read(&opb, 8))
        return RTP_PARSE_ERROR;

    for (i = 6; i > 0; i--)
        bit_read(&opb, 8);

    num = bit_read(&opb, 8) + 1;

    for (; num > 0; num--) {
        int entries, quantvals = -1, maptype, q_quant, dim;
        bit_read(&opb, 24);    //magic
        dim = bit_read(&opb, 16);    //dimensions
        entries = bit_read(&opb, 24);    //entries

        switch (bit_read(&opb, 1)) {
        case 0:    //unordered
            if (bit_read(&opb, 1)) {
                for (i = 0; i < entries; i++)
                    if (bit_read(&opb, 1))
                        bit_read(&opb, 5);
            } else
                for (i = 0; i < entries; i++)
                    bit_read(&opb, 5);
            break;
        case 1:    //ordered
            bit_read(&opb, 5);    //len
            for (i = 0; i < entries;)
                i += bit_read(&opb, rtp_ilog(entries - i));
            break;
        }
        switch ((maptype = bit_read(&opb, 4))) {
        case 0:
            break;

        case 1:
        case 2:
            bit_read(&opb, 32);
            bit_read(&opb, 32);
            q_quant = bit_read(&opb, 4) + 1;
            bit_read(&opb, 1);

            switch (maptype) {
            case 1:
                quantvals = maptype_quantvals(entries, dim);
                break;

            case 2:
                quantvals = entries * dim;
                break;
            }

            for (i = 0; i < quantvals; i++)
                bit_read(&opb, q_quant);
            break;
        }

    }

    //times
    num = bit_read(&opb, 6) + 1;
    for (; num > 0; num--)
        bit_read(&opb, 16);

    //floors
    num = bit_read(&opb, 6) + 1;
    for (; num > 0; num--)
        switch (bit_read(&opb, 16)) {

        case 0:
            bit_read(&opb, 16);
            bit_read(&opb, 16);
            bit_read(&opb, 16);
            bit_read(&opb, 6);
            bit_read(&opb, 8);
            for (i = bit_read(&opb, 4) + 1; i > 0; i--)
                bit_read(&opb, 8);
            break;
        case 1:
            {
                int pclass[31], pdim[16], rangebits,
                    partitions = bit_read(&opb, 5);
                for (i = 0; i < partitions; i++) {
                    pclass[i] = bit_read(&opb, 4);
                    if (k < pclass[i])
                        k = pclass[i];
                }
                for (j = 0; j < k + 1; j++) {
                    int subs;
                    pdim[j] = bit_read(&opb, 3) + 1;    // dim
                    subs = bit_read(&opb, 2);
                    if (subs)
                        bit_read(&opb, 8);    // book
                    for (i = 0; i < (1 << subs); i++)
                        bit_read(&opb, 8);
                }

                bit_read(&opb, 2);
                rangebits = bit_read(&opb, 4);
                for (j = 0, k = 0; j < partitions; j++) {
                    bit_read(&opb,
                         rangebits * pdim[pclass[j]]);
                }
            }
        }

    //residues
    num = bit_read(&opb, 6) + 1;
    for (; num > 0; num--) {
        int partitions, acc = 0;
        bit_read(&opb, 16);    // unpack is always the same
        bit_read(&opb, 24);    // begin
        bit_read(&opb, 24);    // end
        bit_read(&opb, 24);    // grouping
        partitions = bit_read(&opb, 6) + 1;
        bit_read(&opb, 8);    // groupbook
        for (j = 0; j < partitions; j++) {
            int cascade = bit_read(&opb, 3);
            if (bit_read(&opb, 1))
                cascade |= (bit_read(&opb, 5) << 3);
            acc += icount(cascade);
        }
        for (j = 0; j < acc; j++)
            bit_read(&opb, 8);
    }
    //maps
    num = bit_read(&opb, 6) + 1;
    for (; num > 0; num--) {
        int submaps = 1;
        bit_read(&opb, 16);    // maps is just mapping0
        if (bit_read(&opb, 1))
            submaps = bit_read(&opb, 4) + 1;
        if (bit_read(&opb, 1))
            for (i = bit_read(&opb, 8) + 1; i > 0; i--) {
                bit_read(&opb, rtp_ilog(channels - 1));
                bit_read(&opb, rtp_ilog(channels - 1));
            }
        bit_read(&opb, 2);
        if (submaps > 1)
            for (i = 0; i < channels; i++)
                bit_read(&opb, 4);
        for (i = 0; i < submaps; i++) {
            bit_read(&opb, 8);
            bit_read(&opb, 8);
            bit_read(&opb, 8);
        }
    }

    //modes
    vorb->modes = bit_read(&opb, 6) + 1;
    for (i = 0; i < vorb->modes; i++) {
        vorb->param_blockflag[i] = bit_read(&opb, 1);    //blockflag
        bit_read(&opb, 16);
        bit_read(&opb, 16);
        bit_read(&opb, 8);
    }

    return 0;        //FIXME add some checks and return -1 on failure
}

//get the blocksize from a full data packet
static long pkt_blocksize(rtp_vorbis * vorb, rtp_frame * fr)
{
    int mode;
    bit_context bc;
    bit_readinit(&bc, fr->data, fr->len);
    bit_read(&bc, 1);

    mode = bit_read(&bc, rtp_ilog(vorb->modes));

    return vorb->blocksizes[vorb->param_blockflag[mode]];
}

static int single_parse(rtp_vorbis * vorb, rtp_pkt * pkt, rtp_frame * fr,
            rtp_buff * config, rtp_ssrc * ssrc)
{

    uint32_t len = RTP_XIPH_LEN(pkt, vorb->offset);

    if (vorb->id != RTP_XIPH_ID(pkt) ||    //not the current id
        //  !cfg_cache_find(vorb,RTP_XIPH_ID(pkt)) || //XXX
        RTP_XIPH_T(pkt) != 1    //not a config packet
        )
        return RTP_PARSE_ERROR;

    if (len > fr->len) {
        fr->data = realloc(fr->data, len);
        fr->len = len;
    }

    memcpy(fr->data, RTP_XIPH_DATA(pkt, vorb->offset), fr->len);
    vorb->pkts--;
    if (vorb->pkts == 0)
        rtp_rm_pkt(ssrc);

    if (RTP_XIPH_T(pkt) == 1)
        return -1; //cfg_fixup(vorb, fr, config, RTP_XIPH_ID(pkt));
    else {
        vorb->curr_bs = pkt_blocksize(vorb, fr);
        if (vorb->prev_bs)
            fr->timestamp += (vorb->curr_bs + vorb->prev_bs) / 4;
        vorb->prev_bs = vorb->curr_bs;
    }

    return 0;
}

static int pack_parse(rtp_vorbis * vorb, rtp_pkt * pkt, rtp_frame * fr,
              rtp_buff * config, rtp_ssrc * ssrc)
{
    single_parse(vorb, pkt, fr, config, ssrc);
    vorb->offset += fr->len + 2;
    return 0;
}

static int frag_parse(rtp_vorbis * vorb, rtp_pkt * pkt, rtp_frame * fr,
              rtp_buff * config, rtp_ssrc * ssrc)
{
    int len, err = 0;

    switch (RTP_XIPH_T(pkt)) {
    case 1:
        vorb->len = 0;
    case 2:
        len = RTP_XIPH_LEN(pkt, 4);
        vorb->buf = realloc(vorb->buf, vorb->len + len);
        memcpy(vorb->buf + vorb->len, RTP_XIPH_DATA(pkt, 4), len);
        vorb->len += len;
        //FIXME return value
        break;
    case 3:
        len = RTP_XIPH_LEN(pkt, 4);
        vorb->buf = realloc(vorb->buf, vorb->len + len);
        memcpy(vorb->buf + vorb->len, RTP_XIPH_DATA(pkt, 4), len);
        vorb->len += len;

        if (vorb->len > fr->len) {
            fr->data = realloc(fr->data, vorb->len);
            fr->len = vorb->len;
        }
        memcpy(fr->data, vorb->buf, fr->len);

        if (RTP_XIPH_T(pkt) == 1)
            err = -1;//cfg_fixup(vorb, fr, config, RTP_XIPH_ID(pkt));
        else {
            vorb->curr_bs = pkt_blocksize(vorb, fr);
            if (vorb->prev_bs)
                fr->timestamp +=
                    (vorb->curr_bs + vorb->prev_bs) / 4;
            vorb->prev_bs = vorb->curr_bs;
        }
        break;
    default:
        err = -1;
        break;
    }

    rtp_rm_pkt(ssrc);

    return err;
}

static uint64_t get_v(uint8_t **cur, int len)
{
    uint64_t val = 0;
    uint8_t *cursor = *cur;
    int tmp = 128, i;
    for (i = 0; i<len && tmp&128; i++)
    {
        tmp = *cursor++;
        val= (val<<7) + (tmp&127);
    }
    *cur = cursor;
    return val;
}

static int xiphrtp_to_mkv(rtp_vorbis *vorb, uint8_t *value, int len, int id)
{
    uint8_t *cur = value, *conf;
    rtp_xiph_conf *tmp;
    int err = 1, i, count, offset = 1, off = 0, val;
    if (len) {
        // convert the format
        count = 1 + get_v(&cur, len);
        if (count != 3) {
            // corrupted packet?
            return RTP_PARSE_ERROR;
        }
        conf = malloc(len + len/255 + 64);
        for (i=0; i< count; i++) {
            val = get_v(&cur, len);
            off += val;                          // offset to the setup header
            offset += nms_xiphlacing(conf +i, val); // offset in configuration
        }
        len -= cur - value; // raw configuration length
        conf[0] = count; // XXX
        memcpy(conf + offset, cur, len);
        // parse it
        err = cfg_parse(vorb, cur, len, off);
        // append to the list
        vorb->conf_len++;
        vorb->conf = realloc(vorb->conf,
                               vorb->conf_len*sizeof(rtp_xiph_conf));
        tmp = vorb->conf + vorb->conf_len - 1;
        tmp->conf = conf;
        tmp->len = len + offset;
        tmp->id = id;
    }
    return err;
}

static int unpack_config(rtp_vorbis *vorb, char *value, int len)
{
    uint8_t buff[len];
    uint8_t *cur = buff;
    int size = nms_base64_decode(buff, value, len);
    int count, i, id;
    if (size == 0) return 1;
    count = ntohl(*(uint32_t *)buff);
    cur += 4;
    size -= 4;
    for (i = 0; i < count && size > 0; i++) {
        id = cur[0]|cur[1]<<8|cur[2]<<16;
        cur += 3;
        len = ntohs(*(uint16_t *)cur);
        cur += 2;
        if (xiphrtp_to_mkv(vorb, cur, len, id)) return 1;
        size -= len + 3 + 2;
    }
    return 0;
}

static int vorbis_init_parser(rtp_session * rtp_sess, unsigned pt)
{
    rtp_vorbis *vorb = calloc(1, sizeof(rtp_vorbis));
    rtp_pt_attrs *attrs = &rtp_sess->ptdefs[pt]->attrs;
    char *value;
    int i, err = -1;
    int len;

    if (!vorb)
        return RTP_ERRALLOC;

    memset(vorb, 0, sizeof(rtp_vorbis));

// parse the sdp to get the first configuration
    for (i=0; i < attrs->size; i++) {
        if ((value = strstr(attrs->data[i], "configuration="))) {
            value+=14;
            len = strstr(value,";")-value;
            if (len <= 0) continue;
                err = unpack_config(vorb, value, len);            
        }
        // other ways are disregarded for now.
        if (!err) break;
    }

    if (err) {
        free(vorb);
        return RTP_PARSE_ERROR;
    }

// associate it to the right payload
    rtp_sess->ptdefs[pt]->priv = vorb;

    return 0;
}

int vorbis_uninit_parser(rtp_ssrc * ssrc, unsigned pt)
{

    rtp_vorbis *vorb = ssrc->privs[pt];

    if (vorb && vorb->buf)
        free(vorb->buf);
    if (vorb)
        free(vorb);

    vorb = ssrc->rtp_sess->ptdefs[pt]->priv;

    ssrc->rtp_sess->ptdefs[pt]->priv = NULL;

    if (vorb)
        free(vorb);

    return 0;
}

/**
 * it should return a vorbis frame either by unpacking an aggregate
 * or by fetching more than a single rtp packet
 */

static int vorbis_parse(rtp_ssrc * ssrc, rtp_frame * fr, rtp_buff * config)
{
    rtp_pkt *pkt;
    size_t len;

    rtp_vorbis *vorb = ssrc->privs[fr->pt];

    if (!vorb) {
        vorb = ssrc->privs[fr->pt] = malloc(sizeof(rtp_vorbis));
        vorb =
            memcpy(vorb, ssrc->rtp_sess->ptdefs[fr->pt]->priv,
               sizeof(rtp_vorbis));
    }
    // get the current packet
    if (!(pkt = rtp_get_pkt(ssrc, &len)))
        return RTP_BUFF_EMPTY;

    // if I don't have previous work
    if (!vorb->pkts) {
        // get the number of packets stuffed in the rtp
        vorb->pkts = RTP_XIPH_PKTS(pkt);

        // some error checking
        if (vorb->pkts > 0 && (RTP_XIPH_F(pkt) || !RTP_XIPH_T(pkt)))
            return RTP_PARSE_ERROR;

        if (RTP_XIPH_F(pkt))
            return frag_parse(vorb, pkt, fr, config, ssrc);
        // single packet, easy case
        if (vorb->pkts == 1)
            return single_parse(vorb, pkt, fr, config, ssrc);
        vorb->offset = 4;
    }
    // keep parsing the current rtp packet
    return pack_parse(vorb, pkt, fr, config, ssrc);
}

RTP_PARSER_FULL(vorbis);

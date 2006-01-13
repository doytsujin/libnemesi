/* * 
 *  $Id$
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2001 by
 *  	
 *  	Giampaolo "mancho" Mancini - manchoz@inwind.it
 *	Francesco "shawill" Varano - shawill@infinto.it
 *
 *  NeMeSI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NeMeSI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NeMeSI; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <nemesi/rtp.h>

#define RET_ERR(err_level, ...)	{ \
					nmsprintf(err_level, __VA_ARGS__ ); \
					free(rtp_sess); \
					return NULL; \
				}

struct RTP_Session *init_rtp_sess(NMSsockaddr *local, NMSsockaddr *peer)
{
	struct RTP_Session *rtp_sess;
	NMSaddr nms_addr;

	if((rtp_sess=(struct RTP_Session *)calloc(1, sizeof(struct RTP_Session))) == NULL) {
		nmsprintf(NMSML_FATAL, "Cannot allocate memory!\n");
		return NULL;
	}

	// rtp_sess->rtp_tid=-1;
	// rtp_sess->dec_tid=-1;	
	rtp_sess->rtpfd=-1;
	rtp_sess->rtcpfd=-1;
	rtp_sess->local_ssrc=random32(0);
	if ( pthread_mutex_init(&rtp_sess->syn,NULL) )
		RET_ERR(NMSML_FATAL, "Cannot init mutex!\n")
	if( !(rtp_sess->transport.spec=strdup(RTP_AVP_UDP)) )
		RET_ERR(NMSML_FATAL, "Cannot duplicate string!\n")
	rtp_sess->transport.delivery=unicast;
	// --- remote address
	if ( sock_get_addr(peer->addr, &nms_addr) )
		RET_ERR(NMSML_ERR, "remote address not valid\n")
	if (rtp_transport_set(rtp_sess, RTP_TRANSPORT_SRCADDR, &nms_addr))
		RET_ERR(NMSML_ERR, "Could not set srcaddr in transport string\n")
	switch (nms_addr.family) {
		case AF_INET:
			nmsprintf(NMSML_DBG1, "IPv4 address\n");
			break;
		case AF_INET6:
			nmsprintf(NMSML_DBG1, "IPv6 address\n");
			break;
	}
	// --- local address
	if ( sock_get_addr(local->addr, &nms_addr) )
		RET_ERR(NMSML_ERR, "local address not valid\n")
	if (rtp_transport_set(rtp_sess, RTP_TRANSPORT_DSTADDR, &nms_addr))
		RET_ERR(NMSML_ERR, "Could not set dstaddr in transport string\n")
	switch (nms_addr.family) {
		case AF_INET:
			nmsprintf(NMSML_DBG1, "IPv4 local address\n");
			break;
		case AF_INET6:
			nmsprintf(NMSML_DBG1, "IPv6 local address\n");
			break;
	}
	// ---
	rtp_sess->transport.mode=play;
	rtp_sess->transport.ssrc=rtp_sess->local_ssrc;
	
	rtp_sess->sess_stats.pmembers=1;
	rtp_sess->sess_stats.members=1;
	rtp_sess->sess_stats.initial=1;
	rtp_sess->sess_stats.avg_rtcp_size=200; /* RR + SDES ~= 200 Bytes */
	rtp_sess->sess_stats.rtcp_bw=BANDWIDTH;

	return rtp_sess;
}
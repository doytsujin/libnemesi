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

#include <nemesi/rtcp.h>

int send_rtcp_rr(struct RTP_Session *rtp_sess)
{
	rtcp_pkt *pkt;
	int len;
	uint32 rr_buff[MAX_PKT_SIZE];
	struct Stream_Source *stm_src;

	memset(rr_buff, 0, MAX_PKT_SIZE*sizeof(uint32));
	pkt=(rtcp_pkt *)rr_buff;

	len = build_rtcp_rr(rtp_sess, pkt); /* in 32 bit words */
	pkt=(rtcp_pkt *)(rr_buff+len);
	len += build_rtcp_sdes(rtp_sess, pkt, (MAX_PKT_SIZE >> 2) - len); /* in 32 bit words */

	for(stm_src=rtp_sess->ssrc_queue; stm_src; stm_src=stm_src->next)
		if (stm_src->rtcptofd > 0)
			if( write(stm_src->rtcptofd, rr_buff, (len << 2)) < 0 )
				nmsprintf(NMSML_WARN, "WARNING! Error while sending RTCP pkt\n");
	
	return len;	
}


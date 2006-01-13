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

/*
 * check for ssrc of incoming packet.
 * \return SSRC_KNOWN, SSRC_NEW, SSRC_COLLISION, -1 on internal fatal error.
 * */
int ssrc_check(struct RTP_Session *rtp_sess, uint32 ssrc, struct Stream_Source **stm_src, NMSsockaddr *recfrom, enum proto_types proto_type)
{
	struct Conflict *stm_conf=rtp_sess->conf_queue;
	struct sockaddr_storage sockaddr;
	NMSsockaddr sock = { (struct sockaddr *)&sockaddr, sizeof(sockaddr) };
	uint8 local_collision;
	

	local_collision = (rtp_sess->local_ssrc == ssrc) ? SSRC_COLLISION : 0;
	pthread_mutex_lock(&rtp_sess->syn);	
	pthread_mutex_unlock(&rtp_sess->syn);	
	for(*stm_src=rtp_sess->ssrc_queue; !local_collision && *stm_src && ((*stm_src)->ssrc != ssrc); *stm_src=(*stm_src)->next);
	if(!*stm_src && !local_collision ){
		/* nuovo SSRC */
		/* inserimento in testa */
		pthread_mutex_lock(&rtp_sess->syn);	
		nmsprintf(NMSML_DBG1, "new SSRC\n");
		if ( set_stm_src(rtp_sess, stm_src, ssrc, recfrom, proto_type) < 0) {
			pthread_mutex_unlock(&rtp_sess->syn);	
			return -nmsprintf(NMSML_ERR, "Error while setting new Stream Source\n");
		}

		poinit(&((*stm_src)->po),&(rtp_sess->bp));
		pthread_mutex_unlock(&rtp_sess->syn);	
		return SSRC_NEW;
	} else {
		if (local_collision){
			
			if (proto_type == RTP)
				getsockname(rtp_sess->rtpfd, sock.addr, &sock.addr_len);
			else
				getsockname(rtp_sess->rtcpfd, sock.addr, &sock.addr_len);
			
		} else if (proto_type == RTP){
			
			if (!(*stm_src)->rtp_from.addr) {
				sockaddrdup(&(*stm_src)->rtp_from, recfrom);
				nmsprintf(NMSML_DBG1, "new SSRC for RTP\n");
				local_collision = SSRC_RTPNEW;
			}
			sock.addr = (*stm_src)->rtp_from.addr;
			sock.addr_len = (*stm_src)->rtp_from.addr_len;
			
		} else /* if (proto_type == RTCP)*/ {
			
			if (!(*stm_src)->rtcp_from.addr) {
				sockaddrdup(&(*stm_src)->rtcp_from, recfrom);
				nmsprintf(NMSML_DBG1, "new SSRC for RTCP\n");
				local_collision = SSRC_RTCPNEW;
			}
			sock.addr = (*stm_src)->rtcp_from.addr;
			sock.addr_len = (*stm_src)->rtcp_from.addr_len;

			if (!(*stm_src)->rtcp_to.addr) {
				NMSaddr nms_addr;

				if (sock_get_addr(recfrom->addr, &nms_addr))
					return -nmsprintf(NMSML_ERR, "Invalid address for received packet\n");
				
				// if ( rtcp_to_connect(*stm_src, recfrom, (rtp_sess->transport).srv_ports[1]) < 0 )
				if ( rtcp_to_connect(*stm_src, &nms_addr, (rtp_sess->transport).srv_ports[1]) < 0 )
					return -1;
			}
		}

		if( sockaddrcmp(sock.addr, sock.addr_len, recfrom->addr, recfrom->addr_len) ){
			nmsprintf(NMSML_ERR, "An identifier collision or a loop is indicated\n");
			
			/* An identifier collision or a loop is indicated */
			
			if( ssrc != rtp_sess->local_ssrc ){
				/* OPTIONAL error counter step not implemented */
				nmsprintf(NMSML_VERB, "Warning! An identifier collision or a loop is indicated.\n");
				return SSRC_COLLISION;
			}

			/* A collision or loop of partecipants's own packets */
			
			else {
				while ( stm_conf && sockaddrcmp(stm_conf->transaddr.addr, stm_conf->transaddr.addr_len, recfrom->addr, recfrom->addr_len) )
					stm_conf=stm_conf->next;
				
				if (stm_conf){
					
					/* OPTIONAL error counter step not implemented */
					
					stm_conf->time=time(NULL);
					return SSRC_COLLISION;
				} else {
					
					/* New collision, change SSRC identifier */
					
					nmsprintf(NMSML_VERB, "SSRC collision detected: getting new!\n");
					

					/* Send RTCP BYE pkt */
					/*       TODO        */

					/* choosing new ssrc */
					rtp_sess->local_ssrc=random32(0);
					rtp_sess->transport.ssrc=rtp_sess->local_ssrc;
			
					/* New entry in SSRC queue with conflicting ssrc */
					if( (stm_conf=(struct Conflict *)malloc(sizeof(struct Conflict))) == NULL)
						return -nmsprintf(NMSML_FATAL, "Cannot allocate memory!\n");

					/* inserimento in testa */
					/* insert at the beginning of Stream Sources queue */
					pthread_mutex_lock(&rtp_sess->syn);	
					if ( set_stm_src(rtp_sess, stm_src, ssrc, recfrom, proto_type) < 0) {
						pthread_mutex_unlock(&rtp_sess->syn);	
						return -nmsprintf(NMSML_ERR, "Error while setting new Stream Source\n");
					}
					poinit(&((*stm_src)->po),&(rtp_sess->bp));
					pthread_mutex_unlock(&rtp_sess->syn);	
				
					/* New entry in SSRC Conflict queue */
					sockaddrdup(&stm_conf->transaddr, &sock);
					stm_conf->time=time(NULL);
					stm_conf->next=rtp_sess->conf_queue;
					rtp_sess->conf_queue=stm_conf;
				}
				
			}
		}
	}
	
	return local_collision;
}
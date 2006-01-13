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

#include <nemesi/rtsp.h>

struct RTSP_Medium *rtsp_med_create(int fd)
{
	struct RTSP_Medium *rtsp_m;
	struct sockaddr_storage localaddr, peeraddr;
	NMSsockaddr local = { (struct sockaddr *)&localaddr, sizeof(localaddr) };
	NMSsockaddr peer = { (struct sockaddr *)&peeraddr, sizeof(peeraddr) };

	getsockname(fd, (struct sockaddr *)local.addr, &local.addr_len);
	getpeername(fd, (struct sockaddr *)peer.addr, &peer.addr_len);
	
	if ( (rtsp_m=(struct RTSP_Medium *)malloc(sizeof(struct RTSP_Medium))) == NULL ) {
		nmsprintf(NMSML_FATAL, "Cannot allocate memory.\n");
		return NULL;
	}

	if((rtsp_m->rtp_sess=init_rtp_sess(&local, &peer)) == NULL)
		return NULL;
	rtsp_m->next=NULL;
	rtsp_m->filename=NULL;

	rtsp_m->medium_info=NULL;

	return rtsp_m;
}
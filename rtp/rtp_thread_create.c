/* * 
 *  ./rtp/rtp_thread_create.c: $Revision: 1.3 $ -- $Date: 2002/11/28 12:00:48 $
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

int rtp_thread_create(struct RTP_Session *rtp_sess_head)
{
	struct RTP_Session *rtp_sess;
	int n;
	pthread_attr_t rtp_attr;
	pthread_t rtp_tid;

	pthread_attr_init(&rtp_attr);
	if (pthread_attr_setdetachstate(&rtp_attr, PTHREAD_CREATE_JOINABLE) != 0){
		uiprintf("Cannot set RTP Thread attributes!\n");
		return 1;
	}

	if ((n=pthread_create(&rtp_tid, &rtp_attr, &rtp, (void *)rtp_sess_head)) > 0){
		uiprintf("Error: %s\n", strerror(n));
		return 1;
	}
	for (rtp_sess=rtp_sess_head; rtp_sess; rtp_sess=rtp_sess->next)
		rtp_sess->rtp_tid=rtp_tid;
	
	return 0;
}

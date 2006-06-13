/* * 
 *  $Id$
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2006 by
 *  	
 *  	Giampaolo "mancho" Mancini - giampaolo.mancini@polito.it
 *	Francesco "shawill" Varano - francesco.varano@polito.it
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

#include <nemesi/decoder.h>
#ifdef ENABLE_DECODER_2

int (*decoders[128])(char *, int, nms_output *);

void *decoder(void *args)
{
	struct rtp_thread *rtp_th=(struct rtp_thread *)args;
	rtp_session *rtp_sess_head; // =rtp_th->rtp_sess_head;
	rtp_ssrc *stm_src;
	// time vars
	struct timeval startime;
	struct timeval tvstart, tvstop;
	// timestamp in ms vars
	double ts_min_next = 0, ts_next;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	/* pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); */

	pthread_cleanup_push(dec_clean, (void *)nms_outc /*audio_buffer */);
	
	pthread_mutex_lock(&(rtp_th->syn));
	pthread_mutex_unlock(&(rtp_th->syn));
	rtp_sess_head=rtp_th->rtp_sess_head;

	gettimeofday(&startime, NULL);
	
	while(!pthread_mutex_trylock(&(rtp_th->syn))) {
		gettimeofday(&tvstart, NULL);
		for (stm_src=rtp_active_ssrc_queue(rtp_sess_head); stm_src; stm_src=rtp_next_active_ssrc(stm_src)) {
				if ( (ts_next=rtp_get_next_ts(stm_src)) < 0 )
					continue;
		}
		gettimeofday(&tvstop, NULL);
	}
	
	pthread_cleanup_pop(1);
	
	return NULL;
}
#endif // ENABLE_DECODER_2

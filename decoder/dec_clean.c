/* * 
 *  ./decoder/dec_clean.c: $Revision: 1.4 $ -- $Date: 2003/01/15 11:17:59 $
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

#include <nemesi/decoder.h>

void dec_clean(void *args)
{
	struct audio_buff *audio_buffer = (struct audio_buff *)args;

	pthread_mutex_unlock(&(audio_buffer->syn));
	/* chiudiamo eventualmente il file aperto per salvare lo steam invece
	che eseguirlo */
	close_file(); 
	audio_pause();
	empty_audio_buffer();
	uiprintf("Decoder Thread R.I.P.\n");
}

/* * 
 *  $Id$
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2001 by
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

#include <stdlib.h>

#include <nemesi/sdp.h>

int sdp_session_close(SDP_Session_info *session)
{
	SDP_Medium_info *sdp_m, *sdp_m_prev;
	SDP_attr *sdp_attr, *sdp_attr_prev;

	if (session) {
		for(sdp_m=session->media_info_queue; sdp_m; sdp_m_prev=sdp_m, sdp_m=sdp_m->next, free(sdp_m_prev))
			for(sdp_attr=sdp_m->attr_list; sdp_attr; sdp_attr_prev=sdp_attr, sdp_attr=sdp_attr->next, free(sdp_attr_prev));

		/* the last two lines are equivalent to these, but aren't they more beautiful ? ;-)
		sdp_m=session->media_info_queue;
		while (sdp_m) {
			sdp_attr=sdp_m->attr_list;
			while(sdp_attr) {
				sdp_attr_prev=sdp_attr;
				sdp_attr=sdp_attr->next;
				free(sdp_attr_prev);
			}
			sdp_m_prev=sdp_m;
			sdp_m=sdp_m->next;
			free(sdp_m_prev);
		}
		*/

		free(session);
	}
	return 0;
}


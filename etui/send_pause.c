/* * 
 *  ./etui/send_pause.c: $Revision: 1.3 $ -- $Date: 2002/11/28 12:00:47 $
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

#include <nemesi/etui.h>

int send_pause(struct RTSP_args *rtsp_args, char ch)
{

	pthread_mutex_lock(&(rtsp_args->comm_mutex));
		switch(ch){
		case 'z':
			rtsp_args->comm->opcode= PAUSE;
			write(rtsp_args->pipefd[1], "z", 1);
			break;
		case 's':
			rtsp_args->comm->opcode= STOP;
			write(rtsp_args->pipefd[1], "s", 1);
			break;
		default:
			break;
		}
		*(rtsp_args->comm->arg)='\0';
		rtsp_args->rtsp_th->busy=1;
	pthread_mutex_unlock(&(rtsp_args->comm_mutex));

	return 0;
}

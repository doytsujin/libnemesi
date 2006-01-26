/* * 
 *  $Id:server_create.c 267 2006-01-12 17:19:45Z shawill $
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

#include <nemesi/wsocket.h>
#include <nemesi/version.h>

int server_create(char *host, char *port, int *sock)
{
	int n;
	struct addrinfo *res, *ressave;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
#ifdef IPV6
	hints.ai_family = AF_UNSPEC;
#else
	hints.ai_family = AF_INET;
#endif
	hints.ai_socktype = SOCK_DGRAM;

	if ((n = gethostinfo(&res, host, port, &hints)) != 0)
		return nms_printf(NMSML_ERR, "(%s) %s\n", PROG_NAME, gai_strerror(n));

	ressave = res;

	do {
		if ((*sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
			continue;

		if (bind(*sock, res->ai_addr, res->ai_addrlen) == 0)
			break;

		if (close(*sock) < 0)
			return nms_printf(NMSML_ERR, "(%s) %s\n", PROG_NAME, strerror(errno));


	} while ((res = res->ai_next) != NULL);

	freeaddrinfo(ressave);

	return res ? 0 : 1;
}
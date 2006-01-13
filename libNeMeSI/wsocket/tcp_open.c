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

#include <nemesi/wsocket.h>

/**
 * The function opens a TCP connection.
 * It creates a SOCK_STREAM type socket and, through it, connects to the address <tt>name</tt>.
 * \param name address to which we want to connect.
 * \param namelen length of address <tt>name</tt>.
 * \return descriptor of created socket.
 */
int tcp_open(struct sockaddr *name, int namelen)
{
	int f;

	if ((f = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return nmsprintf(NMSML_ERR, "socket() error in tcp_open.\n");

	if (connect(f, name, namelen) < 0)
		return nmsprintf(NMSML_ERR, "connect() error in tcp_open.\n");

	return f;
}
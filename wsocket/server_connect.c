/* * 
 *  $Id:server_connect.c 267 2006-01-12 17:19:45Z shawill $
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2001 by
 *      
 *      Giampaolo "mancho" Mancini - manchoz@inwind.it
 *    Francesco "shawill" Varano - shawill@infinto.it
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

/* DOC++ */
/**
 * Esegue la connessione al "server" tramite la porta "port".
 * @param server stringa contenente il nome del server.
 * @param port porta alla quale agganciare il socket.
 * @param sock variabile che conterra' il descrittore del socket ottenuto.
 * @return 1 se la connessione e' avvenuta con successo, 0 altrimenti.
 * @see tcp_open
 */
int server_connect(char *host, char *port, int *sock,
           enum sock_types sock_type)
{
    int n, connect_new;
    struct addrinfo *res, *ressave;
    struct addrinfo hints;
#ifdef HAVE_SCTP_NEMESI
    struct sctp_initmsg initparams;
    struct sctp_event_subscribe subscribe;
#endif

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_CANONNAME;
#ifdef IPV6
    hints.ai_family = AF_UNSPEC;
#else
    hints.ai_family = AF_INET;
#endif

    switch (sock_type) {
    case SCTP:
#ifndef HAVE_SCTP_NEMESI
        return nms_printf(NMSML_ERR,
                  "%s: SCTP protocol not compiled in\n",
                  PROG_NAME);
        break;
#endif    // else go down to TCP case (SCTP and TCP are both SOCK_STREAM type)
    case TCP:
        hints.ai_socktype = SOCK_STREAM;
        break;
    case UDP:
        hints.ai_socktype = SOCK_DGRAM;
        break;
    default:
        return nms_printf(NMSML_ERR,
                  "%s: Unknown socket type specified\n",
                  PROG_NAME);
        break;
    }

    if ((n = gethostinfo(&res, host, port, &hints)) != 0)
        return nms_printf(NMSML_ERR, "%s: %s\n", PROG_NAME,
                  gai_strerror(n));

    ressave = res;

    connect_new = (*sock < 0);

    do {
#ifdef HAVE_SCTP_NEMESI
        if (sock_type == SCTP)
            res->ai_protocol = IPPROTO_SCTP;
#endif // TODO: remove this code when SCTP will be supported from getaddrinfo()

        if (connect_new && (*sock =
             socket(res->ai_family, res->ai_socktype,
                res->ai_protocol)) < 0)
            continue;

#ifdef HAVE_SCTP_NEMESI
        if (sock_type == SCTP) {
            // Enable the propagation of packets headers
            memset(&subscribe, 0, sizeof(subscribe));
            subscribe.sctp_data_io_event = 1;
            if (setsockopt(*sock, SOL_SCTP, SCTP_EVENTS, &subscribe,
                    sizeof(subscribe)) < 0)
                return nms_printf(NMSML_ERR, "setsockopts(SCTP_EVENTS) error in sctp_open.\n");

            // Setup number of streams to be used for SCTP connection
            memset(&initparams, 0, sizeof(initparams));
            initparams.sinit_max_instreams = MAX_SCTP_STREAMS;
            initparams.sinit_num_ostreams = MAX_SCTP_STREAMS;
            if (setsockopt(*sock, SOL_SCTP, SCTP_INITMSG, &initparams,
                    sizeof(initparams)) < 0)
                return nms_printf(NMSML_ERR, "setsockopts(SCTP_INITMSG) error in sctp_open.\n");
        }
#endif

        if (connect(*sock, res->ai_addr, res->ai_addrlen) == 0)
            break;
        if (connect_new) {
            if (close(*sock) < 0)
                return nms_printf(NMSML_ERR, "(%s) %s", PROG_NAME,
                          strerror(errno));
            else
                *sock = -1;
        }

    } while ((res = res->ai_next) != NULL);

    freeaddrinfo(ressave);

    if (!res)
        return nms_printf(NMSML_ERR,
                  "Server connect error for \"%s:%s\"", host,
                  port);

    return 0;
}
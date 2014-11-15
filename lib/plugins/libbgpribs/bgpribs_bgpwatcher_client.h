/*
 * bgpcorsaro
 *
 * Chiara Orsini, CAIDA, UC San Diego
 * corsaro-info@caida.org
 *
 * Copyright (C) 2014 The Regents of the University of California.
 *
 * This file is part of bgpcorsaro.
 *
 * bgpcorsaro is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bgpcorsaro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bgpcorsaro.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __BGPRIBS_BGPWATCHER_CLIENT_H
#define __BGPRIBS_BGPWATCHER_CLIENT_H

#include "config.h"

#ifdef WITH_BGPWATCHER

#include <bgpwatcher_client.h>

/** @file
 *
 * @brief Header file that exposes the structures needed
 * to handle the bgpwatcher_client within the bgpribs
 * plugin.
 *
 * @author Chiara Orsini
 *
 */

/** Set of data structures that are required to
 *  send peers and prefixes tables from this client
 *  to the server */
typedef struct bw_client {  
  /** bgpwatcher client */
  bgpwatcher_client_t *client;
  /** turn communication on */
  uint8_t bwatcher_on;
  uint8_t ipv4_full_only;
  uint32_t ipv4_full_size;
  uint8_t ipv6_full_only;
  uint32_t ipv6_full_size;
  // TODO: comment
  uint8_t interests;
  uint8_t intents;
  int peer_id;
} bw_client_t;


/** Allocate memory for the client and prepare
 *  structures for communication
 * @return a pointer to the bw_client structure, or
 *  NULL if an error occurred
 */
bw_client_t *bw_client_create();

/** Create a connection to the bgpwatcher server
 *  and allocate memory for peer and prefix tables
 *
 * @return a pointer to the bw_client structure, or
 *  NULL if an error occurred
 */
int bw_client_start(bw_client_t *bwc);


/** decide to send just the full feed ipv4 peers,
 *  i.e. pfx table size above full_size
 */
void bw_client_set_ipv4full(bw_client_t *bwc, uint32_t full_size);

/** decide to send just the full feed ipv6 peers,
 *  i.e. pfx table size above full_size
 */
void bw_client_set_ipv6full(bw_client_t *bwc, uint32_t full_size);

/** Close the connection to the bgpwatcher server
 *  and deallocate the memory used for peer and
 *  prefix tables
 *
 * @param bwc   bw_client to destroy
 *
 */
void bw_client_destroy(bw_client_t * bwc);


#endif


#endif /* __BGPRIBS_BGPWATCHER_CLIENT_H */

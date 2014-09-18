/*
 * bgpwatcher
 *
 * Alistair King, CAIDA, UC San Diego
 * corsaro-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 *
 * This file is part of bgpwatcher.
 *
 * bgpwatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bgpwatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bgpwatcher.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __BGPWATCHER_SERVER_H
#define __BGPWATCHER_SERVER_H

#include <czmq.h>
#include <stdint.h>

#include <bgpwatcher_common.h>

/** @file
 *
 * @brief Header file that exposes the (protected) interface of the bgpwatcher
 * server. This interface is only used by bgpwatcher
 *
 * @author Alistair King
 *
 */

/**
 * @name Public Constants
 *
 * @{ */

/* shared constants are in bgpwatcher_common.h */

/** @} */

/**
 * @name Public Enums
 *
 * @{ */

/** Enumeration of table types */
typedef enum {

  /** Prefix Table */
  BGPWATCHER_SERVER_TABLE_TYPE_PREFIX = 0,

  /** Peer Table */
  BGPWATCHER_SERVER_TABLE_TYPE_PEER = 1,

} bgpwatcher_server_table_type_t;

/** Enumeration of error codes
 *
 * @note these error codes MUST be <= 0
 */
typedef enum {

  /** No error has occured */
  BGPWATCHER_SERVER_ERR_NONE         = 0,

  /** bgpwatcher_server failed to initialize */
  BGPWATCHER_SERVER_ERR_INIT_FAILED  = -1,

  /** bgpwatcher_server failed to start */
  BGPWATCHER_SERVER_ERR_START_FAILED = -2,

  /** bgpwatcher_server was interrupted */
  BGPWATCHER_SERVER_ERR_INTERRUPT    = -3,

  /** unhandled error */
  BGPWATCHER_SERVER_ERR_UNHANDLED    = -4,

  /** protocol error */
  BGPWATCHER_SERVER_ERR_PROTOCOL     = -5,

  /** malloc error */
  BGPWATCHER_SERVER_ERR_MALLOC       = -6,

} bgpwatcher_server_err_code_t;

/** @} */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

typedef struct bgpwatcher_server bgpwatcher_server_t;

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** Information about a client given to bgpwatcher when a client connects or
 *  disconnects
 */
typedef struct bgpwatcher_server_client_info {
  /** Client name (collector name) */
  char *name;
} bgpwatcher_server_client_info_t;

typedef struct bgpwatcher_server_callbacks {

  /** Signals that a new client has connected
   *
   * @param server      pointer to the server instance originating the signal
   * @param client      pointer to a client info structure (server owns this)
   * @param user        pointer to user data given at init time
   * @return 0 if signal successfully handled, -1 otherwise
   */
  int (*client_connect)(bgpwatcher_server_t *server,
			bgpwatcher_server_client_info_t *client,
			void *user);

  /** Signals that a client has disconnected or timed out
   *
   * @param server      pointer to the server instance originating the signal
   * @param client      pointer to a client info structure (server owns this)
   * @param user        pointer to user data given at init time
   * @return 0 if signal successfully handled, -1 otherwise
   */
  int (*client_disconnect)(bgpwatcher_server_t *server,
			   bgpwatcher_server_client_info_t *client,
			   void *user);

  /** Signals that a prefix record has been received
   *
   * @param server      pointer to the server instance originating the signal
   * @param table_id    unique id for the table that the record corresponds to
   * @param record      pointer to a prefix record structure (server owns this)
   * @param user        pointer to user data given at init time
   * @return 0 if signal successfully handled, -1 otherwise
   */
  int (*recv_pfx_record)(bgpwatcher_server_t *server,
			 uint64_t table_id,
			 bgpwatcher_pfx_record_t *record,
			 void *user);

  /** Signals that a peer record has been received
   *
   * @param server      pointer to the server instance originating the signal
   * @param table_id    unique id for the table that the record corresponds to
   * @param record      pointer to a peer record structure (server owns this)
   * @param user        pointer to user data given at init time
   * @return 0 if signal successfully handled, -1 otherwise
   */
  int (*recv_peer_record)(bgpwatcher_server_t *server,
			  uint64_t table_id,
			  bgpwatcher_peer_record_t *record,
			  void *user);

  /** Signals that all records for the given table have been received
   *
   * @param server      pointer to the server instance originating the signal
   * @param table_id    unique id for the table that has been completed
   * @param table_type  type of the table that has been completed
   * @param user        pointer to user data given at init time
   * @return 0 if signal successfully handled, -1 otherwise
   */
  int (*table_end)(bgpwatcher_server_t *server,
		   uint64_t table_id,
		   bgpwatcher_server_table_type_t *table_type,
		   void *user);

  /** User data passed along with each callback */
  void *user;

} bgpwatcher_server_callbacks_t;

struct bgpwatcher_server {

  /** Error status */
  bgpwatcher_err_t err;

  /** 0MQ context pointer */
  zctx_t *ctx;

  /** URI to listen for clients on */
  char *client_uri;

  /** Socket to bind to for client connections */
  void *client_socket;

  /** List of clients that are connected */
  zlist_t *clients;

  /** Time (in ms) between heartbeats sent to clients */
  uint64_t heartbeat_interval;

  /** Time (in ms) to send the next heartbeat to clients */
  uint64_t heartbeat_next;

  /** The number of heartbeats that can go by before a client is declared
      dead */
  int heartbeat_liveness;

  /** Indicates that the server should shutdown at the next opportunity */
  int shutdown;

  /* Functions to call when we get a message from a client */
  bgpwatcher_server_callbacks_t *callbacks;

};

/** @} */

/** Initialize a new BGP Watcher server instance
 *
 * @param callbacks     pointer to a filled-out bgpwatcher server callback
 *                      structure
 * @return a pointer to a bgpwatcher server instance if successful, NULL if an
 * error occurred.
 */
bgpwatcher_server_t *bgpwatcher_server_init(
				      bgpwatcher_server_callbacks_t *callbacks);

/** Start the given bgpwatcher server instance
 *
 * @param server       pointer to a bgpwatcher server instance to start
 * @return 0 if the server started successfully, -1 otherwise.
 *
 * This function will block and run until the server is stopped. Control will
 * return to the calling library only by way of the callback functions specified
 * in the call to bgpwatcher_server_init.
 */
int bgpwatcher_server_start(bgpwatcher_server_t *server);

/** Prints the error status (if any) to standard error and clears the error
 * state
 *
 * @param server       pointer to bgpwatcher server instance to print error for
 */
void bgpwatcher_server_perr(bgpwatcher_server_t *server);

/** Stop the given bgpwatcher server instance at the next safe occasion.
 *
 * This is useful to initiate a clean shutdown if you are handling signals in
 * bgpwatcher. Call this from within your signal handler. It should also be
 * called from bgpwatcher_stop to pass the signal through.
 *
 * @param watcher       pointer to the bgpwatcher instance to stop
 */
void bgpwatcher_server_stop(bgpwatcher_server_t *server);

/** Free the given bgpwatcher server instance
 *
 * @param server       pointer to the bgpwatcher server instance to free
 */
void bgpwatcher_server_free(bgpwatcher_server_t *server);

/** Set the URI for the server to listen for client connections on
 *
 * @param server        pointer to a bgpwatcher server instance to update
 * @param uri           pointer to a uri string
 * @return 0 if the uri was set successfully, -1 otherwise
 *
 * @note defaults to BGPWATCHER_CLIENT_URI_DEFAULT
 */
int bgpwatcher_server_set_client_uri(bgpwatcher_server_t *server,
				      const char *uri);

/** Set the heartbeat interval
 *
 * @param server        pointer to a bgpwatcher server instance to update
 * @param interval_ms   time in ms between heartbeats
 *
 * @note defaults to BGPWATCHER_HEARTBEAT_INTERVAL_DEFAULT
 */
void bgpwatcher_server_set_heartbeat_interval(bgpwatcher_server_t *server,
					      uint64_t interval_ms);

/** Set the heartbeat liveness
 *
 * @param server        pointer to a bgpwatcher server instance to update
 * @param beats         number of heartbeats that can go by before a server is
 *                      declared dead
 *
 * @note defaults to BGPWATCHER_HEARTBEAT_LIVENESS_DEFAULT
 */
void bgpwatcher_server_set_heartbeat_liveness(bgpwatcher_server_t *server,
					      int beats);

#endif

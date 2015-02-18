/*
 * This file is part of bgpwatcher
 *
 * Copyright (C) 2015 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * All rights reserved.
 *
 * This code has been developed by CAIDA at UC San Diego.
 * For more information, contact bgpstream-info@caida.org
 *
 * This source code is proprietary to the CAIDA group at UC San Diego and may
 * not be redistributed, published or disclosed without prior permission from
 * CAIDA.
 *
 * Report any bugs, questions or comments to bgpstream-info@caida.org
 *
 */

#ifndef __BGPWATCHER_VIEW_H
#define __BGPWATCHER_VIEW_H

#include "bl_peersign_map.h"

#include "bgpwatcher_common.h" /* < pfx_peer_info_t */

/** @file
 *
 * @brief Header file that exposes the public interface of bgpwatcher view.
 *
 * @author Alistair King
 *
 */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

/** Opaque handle to an instance of a BGP View table.
 *
 * All interaction with a view must be done through the view API exposed in
 * bgpwatcher_view.h
 */
typedef struct bgpwatcher_view bgpwatcher_view_t;

/** Opaque handle for iterating over fields of a BGP View table. */
typedef struct bgpwatcher_view_iter bgpwatcher_view_iter_t;

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** Callback for destroying a custom user structure associated with a pfx
 * @param user    user pointer to destroy
 */
typedef void (bgpwatcher_view_destroy_user_cb) (void* user);

/** @} */

/**
 * @name Public Enums
 *
 * @{ */

typedef enum {

  /** Iterate over the IPv4 prefixes in the view */
  BGPWATCHER_VIEW_ITER_FIELD_V4PFX      = 1,

  /** Iterate over the IPv6 prefixes in the view */
  BGPWATCHER_VIEW_ITER_FIELD_V6PFX      = 2,

  /** Iterate over the Peer information (peerid=>(collector,IP)) in the view */
  BGPWATCHER_VIEW_ITER_FIELD_PEER       = 3,

  /** Iterate over the peers for the current v4 pfx */
  BGPWATCHER_VIEW_ITER_FIELD_V4PFX_PEER = 4,

  /** Iterate over the peers for the current v6 pfx */
  BGPWATCHER_VIEW_ITER_FIELD_V6PFX_PEER = 5,

} bgpwatcher_view_iter_field_t;

/** @} */

/** Create a new BGP View
 *
 * A BGP View holds a snapshot of the aggregated prefix information.
 * Basically, it maps from prefix -> peers -> prefix info
 *
 * @return a pointer to the view if successful, NULL otherwise
 */
bgpwatcher_view_t *bgpwatcher_view_create();

/** Create a new BGP View, reusing an existing peersigns table
 *
 * @param peersigns     pointer to a peersigns map to share
 *
 * A BGP View holds a snapshot of the aggregated prefix information.
 * Basically, it maps from prefix -> peers -> prefix info
 *
 * @return a pointer to the view if successful, NULL otherwise
 */
bgpwatcher_view_t *bgpwatcher_view_create_shared(bl_peersign_map_t *peersigns);

/** @todo create a nice high-level api for accessing information in the view */

/** Destroy the given BGP Watcher View
 *
 * @param view          pointer to the view to destroy
 */
void bgpwatcher_view_destroy(bgpwatcher_view_t *view);

/** Empty a view
 *
 * @param view          view to clear
 *
 * This does not actually free any memory, it just marks prefix and peers as
 * dirty so that future inserts can re-use the memory allocation. It does *not*
 * clear the peersigns table.
 */
void bgpwatcher_view_clear(bgpwatcher_view_t *view);

/** Destroy all the per-pfx user data
 *
 * @param view          view to destroy user data for
 * @param call_back     function that actually destroy the specific user structure
 *                      used in this view
 */
void bgpwatcher_view_destroy_user(bgpwatcher_view_t *view,
				  bgpwatcher_view_destroy_user_cb *call_back);

/** Dump the given BGP View to stdout
 *
 * @param view        pointer to a view structure
 */
void bgpwatcher_view_dump(bgpwatcher_view_t *view);

/**
 * @name Simple Accessor Functions
 *
 * @{ */

/** Get the total number of IPv4 prefixes in the view
 *
 * @param view          pointer to a view structure
 * @return the number of IPv4 prefixes in the view
 */
uint32_t bgpwatcher_view_v4pfx_size(bgpwatcher_view_t *view);

/** Get the total number of IPv6 prefixes in the view
 *
 * @param view          pointer to a view structure
 * @return the number of IPv6 prefixes in the view
 */
uint32_t bgpwatcher_view_v6pfx_size(bgpwatcher_view_t *view);

/** Get the total number of prefixes (v4+v6) in the view
 *
 * @param view          pointer to a view structure
 * @return the number of prefixes in the view
 */
uint32_t bgpwatcher_view_pfx_size(bgpwatcher_view_t *view);

/** Get the number of peers in the view
 *
 * @param view          pointer to a view structure
 * @return the number of peers in the view
 */
uint32_t bgpwatcher_view_peer_size(bgpwatcher_view_t *view);

/** Get the BGP time that the view represents
 *
 * @param view          pointer to a view structure
 * @return the time that the view represents
 */
uint32_t bgpwatcher_view_time(bgpwatcher_view_t *view);

/** @} */

/**
 * @name View Iterator Functions
 *
 * @{ */

/** Create a new view iterator
 *
 * @param               Pointer to the view to create iterator for
 * @return pointer to an iterator if successful, NULL otherwise
 */
bgpwatcher_view_iter_t *bgpwatcher_view_iter_create(bgpwatcher_view_t *view);

/** Destroy the given iterator
 *
 * @param               Pointer to the iterator to destroy
 */
void bgpwatcher_view_iter_destroy(bgpwatcher_view_iter_t *iter);

/** Reset the given iterator to the first item for the given field
 *
 * @param iter          Pointer to an iterator structure
 * @param field         The iterator field to reset
 */
void bgpwatcher_view_iter_first(bgpwatcher_view_iter_t *iter,
				bgpwatcher_view_iter_field_t field);

/** Check if the given iterator has reached the end of the items for the given
 * field.
 *
 * @param iter          Pointer to an iterator structure
 * @param field         The iterator field to check
 * @return 0 if the iterator is pointing to a valid item, 1 if all items have
 * been iterated over.
 *
 * @note the various `get` functions will return invalid results when this
 * function returns 1
 */
int bgpwatcher_view_iter_is_end(bgpwatcher_view_iter_t *iter,
				bgpwatcher_view_iter_field_t field);

/** Advance the provided iterator to the next prefix in the given view
 *
 * @param iter          Pointer to an iterator structure
 * @param field         The iterator field to advance
 *
 * @note this function will have no effect if bgpwatcher_view_iter_is_end
 * returns non-zero for the given field.
 */
void bgpwatcher_view_iter_next(bgpwatcher_view_iter_t *iter,
			       bgpwatcher_view_iter_field_t field);

/** Get the total number of items in the iterator for the given field
 *
 * @param iter          Pointer to an iterator structure
 * @param field         The iterator field to get the size of
 * @return the number of items for the given field
 *
 * @note this function can be called at any time using a valid iterator for the
 * v4pfxs, v6pfxs, and peers fields, but must only be called while the
 * corresponding top-level (v4pfx or v6pfx) iterator is valid to get the size
 * for the v4pfx_peers and v6pfx_peers fields respectively.
 */
uint64_t bgpwatcher_view_iter_size(bgpwatcher_view_iter_t *iter,
				   bgpwatcher_view_iter_field_t field);

/** Get the current v4 prefix for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the prefix that the iterator's v4pfx field is currently pointed at,
 *         NULL if the iterator is not initialized, or has reached the end of
 *         the v4 prefixes.
 */
bl_ipv4_pfx_t *bgpwatcher_view_iter_get_v4pfx(bgpwatcher_view_iter_t *iter);

/** Get the current v6 prefix for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the prefix that the iterator's v6pfx field is currently pointed at,
 *         NULL if the iterator is not initialized, or has reached the end of
 *         the v6 prefixes.
 */
bl_ipv6_pfx_t *bgpwatcher_view_iter_get_v6pfx(bgpwatcher_view_iter_t *iter);

/** Get the current v4 prefix user pointer for the given iterator
 *
 * @param iter      Pointer to an iterator structure
 * @return the user pointer that the iterator's v4pfx field is currently pointed at,
 *         NULL if the iterator is not initialized, or has reached the end of
 *         the v4 prefixes.
 */
void *bgpwatcher_view_iter_get_v4pfx_user(bgpwatcher_view_iter_t *iter);

/** Get the current v6 prefix user pointer for the given iterator
 *
 * @param iter      Pointer to an iterator structure
 * @return the user pointer that the iterator's v4pfx field is currently pointed at,
 *         NULL if the iterator is not initialized, or has reached the end of
 *         the v6 prefixes.
 */
void *bgpwatcher_view_iter_get_v6pfx_user(bgpwatcher_view_iter_t *iter);

/** Set the current v4 prefix user pointer for the given iterator
 *
 * @param iter      Pointer to an iterator structure
 * @param user      Pointer to store per-pfx information on consumers
 */
void bgpwatcher_view_iter_set_v4pfx_user(bgpwatcher_view_iter_t *iter, void *user);

/** Set the current v6 prefix user pointer for the given iterator
 *
 * @param iter      Pointer to an iterator structure
 * @param user      Pointer to store per-pfx information on consumers
 */
void bgpwatcher_view_iter_set_v6pfx_user(bgpwatcher_view_iter_t *iter, void *user);


/** Get the current peer ID for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the peer ID that the iterator's peer field is currently pointed at,
 *         NULL if the iterator is not initialized, or has reached the end of
 *         the peers
 */
bl_peerid_t
bgpwatcher_view_iter_get_peerid(bgpwatcher_view_iter_t *iter);

/** Get the current peer signature for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the peer signature that the iterator's peer field is currently
 *         pointed at, NULL if the iterator is not initialized, or has reached
 *         the end of the peers
 */
bl_peer_signature_t *
bgpwatcher_view_iter_get_peersig(bgpwatcher_view_iter_t *iter);

/** Get the current peer's IPv4 prefix count for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the number of prefixes observed by the peer currently pointed at by
 *         the iterator's peer field. -1 if the iterator is not initialized, or
 *         as reached the end of the peers.
 */
int
bgpwatcher_view_iter_get_peer_v4pfx_cnt(bgpwatcher_view_iter_t *iter);

/** Get the current peer's IPv6 prefix count for the given iterator
 *
 * @param iter          Pointer to an iterator structure
 * @return the number of prefixes observed by the peer currently pointed at by
 *         the iterator's peer field. -1 if the iterator is not initialized, or
 *         as reached the end of the peers.
 */
int
bgpwatcher_view_iter_get_peer_v6pfx_cnt(bgpwatcher_view_iter_t *iter);

/** Get the current peer ID (key) for the current v4pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the peer ID that the iterator's v4pfx_peer field is currently pointed
 *         at, NULL if the iterator is not initialized, or has reached the end
 *         of the peers for the current v4 prefix
 *
 * @note this returns the current peer (BGPWATCHER_VIEW_ITER_FIELD_V4PFX_PEER)
 * for the current prefix (BGPWATCHER_VIEW_ITER_FIELD_V4PFX).
 *
 * @note the peer ID is only meaningful *within* a view.
 */
bl_peerid_t
bgpwatcher_view_iter_get_v4pfx_peerid(bgpwatcher_view_iter_t *iter);

/** Get the current peer ID (key) for the current v6pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the peer ID that the iterator's v6pfx_peer field is currently pointed
 *         at, NULL if the iterator is not initialized, or has reached the end
 *         of the peers for the current v6 prefix
 *
 * @note this returns the current peer (BGPWATCHER_VIEW_ITER_FIELD_V6PFX_PEER)
 * for the current prefix (BGPWATCHER_VIEW_ITER_FIELD_V6PFX).
 *
 * @note the peer ID is only meaningful *within* a view.
 */
bl_peerid_t
bgpwatcher_view_iter_get_v6pfx_peerid(bgpwatcher_view_iter_t *iter);

/** Get the current peer signature (key) for the current v4pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the peer signature info that the iterator's v4pfx_peer field is
 *         currently pointed at, NULL if the iterator is not initialized, or has
 *         reached the end of the peers for the current v4 prefix
 *
 * @note this returns the current peer (BGPWATCHER_VIEW_ITER_FIELD_V4PFX_PEER)
 * for the current prefix (BGPWATCHER_VIEW_ITER_FIELD_V4PFX).
 */
bl_peer_signature_t *
bgpwatcher_view_iter_get_v4pfx_peersig(bgpwatcher_view_iter_t *iter);

/** Get the current peer signature (key) for the current v6pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the peer signature info that the iterator's v6pfx_peer field is
 *         currently pointed at, NULL if the iterator is not initialized, or has
 *         reached the end of the peers for the current v6 prefix
 *
 * @note this returns the current peer (BGPWATCHER_VIEW_ITER_FIELD_V6PFX_PEER)
 * for the current prefix (BGPWATCHER_VIEW_ITER_FIELD_V6PFX).
 */
bl_peer_signature_t *
bgpwatcher_view_iter_get_v6pfx_peersig(bgpwatcher_view_iter_t *iter);

/** Get the current pfx info (value) for the current v4pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the prefix-peer info that the iterator's peer field is currently
 *         pointed at, NULL if the iterator is not initialized, or has reached
 *         the end of the peers for the current prefix
 *
 * @note this returns the pfxinfo for the current peer
 * (BGPWATCHER_VIEW_ITER_FIELD_V4PFX_PEER) for the current prefix
 * (BGPWATCHER_VIEW_ITER_FIELD_V4PFX).
 */
bgpwatcher_pfx_peer_info_t *
bgpwatcher_view_iter_get_v4pfx_pfxinfo(bgpwatcher_view_iter_t *iter);

/** Get the current pfx info (value) for the current v6pfx.
 *
 * @param iter          Pointer to a valid iterator structure
 * @return the prefix-peer info that the iterator's peer field is currently
 *         pointed at, NULL if the iterator is not initialized, or has reached
 *         the end of the peers for the current prefix
 *
 * @note this returns the pfxinfo for the current peer
 * (BGPWATCHER_VIEW_ITER_FIELD_V6PFX_PEER) for the current prefix
 * (BGPWATCHER_VIEW_ITER_FIELD_V6PFX).
 */
bgpwatcher_pfx_peer_info_t *
bgpwatcher_view_iter_get_v6pfx_pfxinfo(bgpwatcher_view_iter_t *iter);

/** @} */

#endif /* __BGPWATCHER_VIEW_H */

/*
 * Copyright (C) 2014 The Regents of the University of California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __BGPSTREAM_BGPDUMP_H_
#define __BGPSTREAM_BGPDUMP_H_

#include "bgpstream_utils.h"
#include "bgpstream_record.h"
#include "bgpstream_elem.h"

/** Write the string representation of the record/elem into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param elem          pointer to a BGP Stream Record to convert to string
 * @param elem          pointer to a BGP Stream Elem to convert to string
 * @return pointer to the start of the buffer if successful, NULL otherwise
 */
char *bgpstream_record_elem_bgpdump_snprintf(char *buf, size_t len,
                                     bgpstream_record_t *record,
                                     bgpstream_elem_t *elem);

/** Write the string representation of the record type into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param type          record type to convert to string
 * @return the number of characters that would have been written if len was
 * unlimited
 */
int bgpstream_record_type_bgpdump_snprintf(char *buf, size_t len,
                                   bgpstream_record_type_t type);

/** Write the string representation of the elem type into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param type          BGP Stream Elem type to convert to string
 * @return the number of characters that would have been written if len was
 * unlimited
 */
int bgpstream_elem_type_bgpdump_snprintf(char *buf, size_t len,
                                 bgpstream_elem_type_t type);

/** Write the string representation of the elem into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param elem          pointer to a BGP Stream Elem to convert to string
 * @return pointer to the start of the buffer if successful, NULL otherwise
 */
char *bgpstream_elem_bgpdump_snprintf(char *buf, size_t len,
                              const bgpstream_elem_t *elem);
#endif // __BGPSTREAM_BGPDUMP_H_

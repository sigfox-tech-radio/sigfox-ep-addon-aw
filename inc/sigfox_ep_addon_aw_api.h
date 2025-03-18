/*!*****************************************************************
 * \file    sigfox_ep_addon_aw_api.h
 * \brief   Sigfox End-Point Atlas WiFi addon API.
 *******************************************************************
 * \copyright
 *
 * Copyright (c) 2022, UnaBiz SAS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1 Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  2 Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  3 Neither the name of UnaBiz SAS nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************/

#ifndef __SIGFOX_EP_ADDON_AW_API_H__
#define __SIGFOX_EP_ADDON_AW_API_H__

#ifndef SIGFOX_EP_DISABLE_FLAGS_FILE
#include "sigfox_ep_flags.h"
#endif
#include "sigfox_types.h"

/*** SIGFOX EP ADDON AW API macros ***/

#define SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_CHAR    17
#define SIGFOX_EP_ADDON_AW_API_SSID_SIZE_CHAR           32

#define SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES    12

/*** SIGFOX EP ADDON AW API structures ***/

#ifdef SIGFOX_EP_ERROR_CODES
/*!******************************************************************
 * \enum SIGFOX_EP_ADDON_AW_status_t
 * \brief Sigfox EP ADDON AW error codes.
 *******************************************************************/
typedef enum {
    // Core addon errors.
    SIGFOX_EP_ADDON_AW_API_SUCCESS = 0,
    SIGFOX_EP_ADDON_AW_API_ERROR_NULL_PARAMETER,
    SIGFOX_EP_ADDON_AW_API_ERROR_MAC_ADDRESS_SEPARATOR,
    SIGFOX_EP_ADDON_AW_API_ERROR_MAC_ADDRESS_FORMAT,
    SIGFOX_EP_ADDON_AW_API_ERROR_ACCESS_POINT_LIST_SIZE,
    SIGFOX_EP_ADDON_AW_API_ERROR_NONE_VALID_ACCESS_POINT,
    SIGFOX_EP_ADDON_AW_API_ERROR_SORTING,
    SIGFOX_EP_ADDON_AW_API_ERROR_LAST
} SIGFOX_EP_ADDON_AW_API_status_t;
#else
typedef void SIGFOX_EP_ADDON_AW_API_status_t;
#endif

/*!******************************************************************
 * \enum SIGFOX_EP_ADDON_AW_API_access_point_status_t
 * \brief Access point status after filtering function.
 *******************************************************************/
typedef enum {
    SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW = 0,
    SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_FILTERED_OUT,
    SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_VALID,
    SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_SENT,
    SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_LAST
} SIGFOX_EP_ADDON_AW_API_access_point_status_t;

/*!******************************************************************
 * \enum SIGFOX_EP_ADDON_AW_API_filter_t
 * \brief MAC addresses filtering options.
 *******************************************************************/
typedef enum {
    SIGFOX_EP_ADDON_AW_API_FILTER_LOCALLY_ADMINISTERED = 0,
    SIGFOX_EP_ADDON_AW_API_FILTER_SSID_EMPTY,
    SIGFOX_EP_ADDON_AW_API_FILTER_SSID_BLACK_LIST,
    SIGFOX_EP_ADDON_AW_API_FILTER_LAST
} SIGFOX_EP_ADDON_AW_API_filter_t;

/*!******************************************************************
 * \enum SIGFOX_EP_ADDON_AX_API_sorting_t
 * \brief MAC addresses sorting options (after filtering).
 *******************************************************************/
typedef enum {
    SIGFOX_EP_ADDON_AW_API_SORTING_NONE = 0,
    SIGFOX_EP_ADDON_AW_API_SORTING_RSSI,
    SIGFOX_EP_ADDON_AW_API_SORTING_LAST
} SIGFOX_EP_ADDON_AW_API_sorting_t;

/*!******************************************************************
 * \struct SIGFOX_EP_ADDON_AW_API_access_point_t
 * \brief Sigfox EP ADDON AW access point structure.
 *******************************************************************/
typedef struct {
    sfx_u8 mac_address[SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_CHAR]; // ASCII format: "xx:xx:xx:xx:xx"
    sfx_u8 ssid[SIGFOX_EP_ADDON_AW_API_SSID_SIZE_CHAR];
    sfx_s16 rssi_dbm;
    SIGFOX_EP_ADDON_AW_API_access_point_status_t status;
} SIGFOX_EP_ADDON_AW_API_access_point_t;

/*!******************************************************************
 * \struct SIGFOX_EP_ADDON_AW_API_input_data_t
 * \brief Sigfox EP ADDON AW input data structure.
 *******************************************************************/
typedef struct {
    SIGFOX_EP_ADDON_AW_API_access_point_t **access_point_list;
    sfx_u8 access_point_list_size;
} SIGFOX_EP_ADDON_AW_API_input_data_t;

/*** SIGFOX EP ADDON AW API functions ***/

/*!******************************************************************
 * \fn SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_set_filter(sfx_u8 filters, SIGFOX_EP_ADDON_AW_API_sorting_t sorting)
 * \brief Configure the access points filtering function.
 * \param[in]   filters: Filters to enable (bitfield indexed on @ref SIGFOX_EP_ADDON_AW_API_filter_t)
 * \param[in]   sorting: Sorting method for the remaining access points (if none, the first are selected).
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_set_filter(sfx_u8 filters, SIGFOX_EP_ADDON_AW_API_sorting_t sorting);

/*!******************************************************************
 * \fn SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_build_ul_payload(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data, sfx_u8 *ul_payload, sfx_u8 *nb_mac_ul_payload)
 * \brief Build a Sigfox Atlas WiFi payload.
 * \param[in]   input_data: Pointer to the input data from WiFi module.
 * \param[out]  ul_payload: Pointer to the built 12-bytes uplink payload (to send with Sigfox EP library to perform Atlas WiFi geolocation).
 * \param[out]  nb_mac_ul_payload: Pointer to the effective number of valid MAC addresses used in the payload (0, 1 or 2).
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_build_ul_payload(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data, sfx_u8 *ul_payload, sfx_u8 *nb_mac_ul_payload);

#if ((defined SIGFOX_EP_UL_PAYLOAD_SIZE) && (SIGFOX_EP_UL_PAYLOAD_SIZE != SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES))
#error "12 bytes payload size must be supported to use this Atlas WiFi addon"
#endif

#endif /* __SIGFOX_EP_ADDON_AW_API_H__ */

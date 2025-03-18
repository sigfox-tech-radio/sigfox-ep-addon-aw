/*!*****************************************************************
 * \file    sigfox_ep_addon_aw_api.c
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

#include "sigfox_ep_addon_aw_api.h"

#ifndef SIGFOX_EP_DISABLE_FLAGS_FILE
#include "sigfox_ep_flags.h"
#endif
#include "sigfox_types.h"

/*** SIGFOX EP ADDON AW API local macros ***/

#define SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES   6

#define SIGFOX_EP_ADDON_AW_API_IG_BYTE_INDEX            0
#define SIGFOX_EP_ADDON_AW_API_IG_BIT_MASK              0x01

#define SIGFOX_EP_ADDON_AW_API_UL_BYTE_INDEX            0
#define SIGFOX_EP_ADDON_AW_API_UL_BIT_MASK              0x02

#define SIGFOX_EP_ADDON_AW_API_SSID_BLACK_LIST_SIZE     5

#define SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD        (SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES / SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES)

#define SIGFOX_EP_ADDON_AW_API_BEST_INDEX_NONE          0xFF

#define SIGFOX_EP_ADDON_AW_API_NULL_CHAR                '\0'
#define SIGFOX_EP_ADDON_AW_API_MAC_BYTE_SEPARATOR_CHAR  ':'

#define SIGFOX_EP_ADDON_AW_UPPERCASE_OFFSET             0x20

#define SIGFOX_EP_ADDON_AW_API_S16_MIN                  (-32768)

/*** SIGFOX EP ADDON AW API local functions declaration ***/

static void _filter_locally_administered(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid);
static void _filter_ssid_empty(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid);
static void _filter_ssid_black_list(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid);

static void _sort_none(SIGFOX_EP_ADDON_AW_API_input_data_t *input_list);
static void _sort_rssi(SIGFOX_EP_ADDON_AW_API_input_data_t *input_list);

/*** SIGFOX EP ADDON AW API local structures ***/

typedef void (*SIGFOX_EP_ADDON_AW_API_filter_cb_t)(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid);
typedef void (*SIGFOX_EP_ADDON_AW_API_sort_cb_t)(SIGFOX_EP_ADDON_AW_API_input_data_t *input_list);

/*******************************************************************/
typedef struct {
    sfx_u8 filters;
    SIGFOX_EP_ADDON_AW_API_sorting_t sorting;
    sfx_u8 best_index[SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD];

} SIGFOX_EP_ADDON_AW_API_context_t;

/*** SIGFOX EP ADDON AW API local global variables ***/

static const SIGFOX_EP_ADDON_AW_API_filter_cb_t SIGFOX_EP_ADDON_AW_API_FILTER[SIGFOX_EP_ADDON_AW_API_FILTER_LAST] = {
    &_filter_locally_administered,
    &_filter_ssid_empty,
    &_filter_ssid_black_list,
};

static const SIGFOX_EP_ADDON_AW_API_sort_cb_t SIGFOX_EP_ADDON_AW_API_SORT[SIGFOX_EP_ADDON_AW_API_SORTING_LAST] = {
    &_sort_none,
    &_sort_rssi,
};

static const sfx_u8 SIGFOX_EP_ADDON_AW_API_SSID_PHONE[] = "phone";
static const sfx_u8 SIGFOX_EP_ADDON_AW_API_SSID_HUAWEI[] = "huawei";
static const sfx_u8 SIGFOX_EP_ADDON_AW_API_SSID_SAMSUNG[] = "samsung";
static const sfx_u8 SIGFOX_EP_ADDON_AW_API_SSID_ANDROID[] = "android";
static const sfx_u8 SIGFOX_EP_ADDON_AW_API_SSID_APPLE[] = "apple";

static const sfx_u8* const SIGFOX_EP_ADDON_AW_API_SSID_BLACK_LIST[SIGFOX_EP_ADDON_AW_API_SSID_BLACK_LIST_SIZE] = {
    SIGFOX_EP_ADDON_AW_API_SSID_PHONE,
    SIGFOX_EP_ADDON_AW_API_SSID_HUAWEI,
    SIGFOX_EP_ADDON_AW_API_SSID_SAMSUNG,
    SIGFOX_EP_ADDON_AW_API_SSID_ANDROID,
    SIGFOX_EP_ADDON_AW_API_SSID_APPLE,
};

static SIGFOX_EP_ADDON_AW_API_context_t sigfox_ep_addon_aw_api_ctx = {
    .filters = 0,
    .sorting = SIGFOX_EP_ADDON_AW_API_SORTING_NONE,
};

/*** SIGFOX EP ADDON AW API local functions ***/

/*******************************************************************/
static sfx_bool _compare_string(sfx_u8 *str, sfx_u8 *ref) {
    // Local variables.
    sfx_bool is_equal = SIGFOX_TRUE;
    sfx_u8 idx = 0;
    // Loop on all characters.
    while (ref[idx] != SIGFOX_EP_ADDON_AW_API_NULL_CHAR) {
        // Check if all characters are the same.
        if ((str[idx] != ref[idx]) || (str[idx] == SIGFOX_EP_ADDON_AW_API_NULL_CHAR)) {
           is_equal = SIGFOX_FALSE;
           break;
        }
        // Increment character index.
        idx++;
    }
    return is_equal;
}

/*******************************************************************/
static sfx_bool _search_substring(sfx_u8 *full_string, sfx_u8 *substring) {
    // Local variables.
    sfx_bool substring_found = SIGFOX_FALSE;
    sfx_u8 idx = 0;
    // Loop on all characters.
    while (full_string[idx] != SIGFOX_EP_ADDON_AW_API_NULL_CHAR) {
        // Compare with new shift.
        substring_found = _compare_string(&(full_string[idx]), substring);
        // Directly exit as soon as a black listed name is found.
        if (substring_found == SIGFOX_TRUE) break;
        // Increment character index.
        idx++;
    }
    return substring_found;
}

/*******************************************************************/
static SIGFOX_EP_ADDON_AW_API_status_t _ascii_to_value(sfx_u8 ascii_character, sfx_u8 *value) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
    // Reset value.
    (*value) = 0;
    // Convert to uppercase.
    if (ascii_character > 'F') {
        ascii_character -= SIGFOX_EP_ADDON_AW_UPPERCASE_OFFSET;
    }
    // Case of digit.
    if ((ascii_character >= '0') && (ascii_character <= '9')) {
        (*value) = (ascii_character - '0');
    }
    // Case of letter.
    else if ((ascii_character >= 'A') && (ascii_character <= 'F')) {
        (*value) = (ascii_character - 'A' + 10);
    }
    else {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_MAC_ADDRESS_FORMAT);
    }
errors:
    SIGFOX_RETURN();
}

/*******************************************************************/
static SIGFOX_EP_ADDON_AW_API_status_t _mac_address_ascii_to_bytes_array(sfx_u8 *mac_address_ascii, sfx_u8 *mac_address_bytes) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
    sfx_u8 digit_value = 0;
    sfx_u8 char_idx = 0;
    sfx_u8 byte_idx = 0;
    // Reset output.
    for (byte_idx = 0; byte_idx < SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES; byte_idx++) {
        mac_address_bytes[byte_idx] = 0x00;
    }
#ifdef SIGFOX_EP_PARAMETERS_CHECK
    // Check byte separator.
    for (char_idx = 2; char_idx < SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_CHAR; char_idx += 3) {
        // Exit if separator is not found.
        if (mac_address_ascii[char_idx] != SIGFOX_EP_ADDON_AW_API_MAC_BYTE_SEPARATOR_CHAR) {
            SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_MAC_ADDRESS_SEPARATOR);
        }
    }
#endif
    byte_idx = 0;
    // Convert ASCII to bytes.
    for (char_idx = 0; char_idx < SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_CHAR; char_idx += 3) {
        // First digit.
#ifdef SIGFOX_EP_ERROR_CODES
        status = _ascii_to_value (mac_address_ascii[char_idx], &digit_value);
        SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
        _ascii_to_value (mac_address_ascii[char_idx], &digit_value);
#endif
        mac_address_bytes[byte_idx] = ((digit_value << 4) & 0xF0);
        // Second digit.
#ifdef SIGFOX_EP_ERROR_CODES
        status = _ascii_to_value (mac_address_ascii[char_idx + 1], &digit_value);
        SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
        _ascii_to_value (mac_address_ascii[char_idx], &digit_value);
#endif
        mac_address_bytes[byte_idx] |= ((digit_value << 0) & 0x0F);
        // Increment output byte index.
        byte_idx++;
    }
#if ((defined SIGFOX_EP_PARAMETERS_CHECK) || (defined SIGFOX_EP_ERROR_CODES))
errors:
#endif
    SIGFOX_RETURN();
}

/*******************************************************************/
static sfx_bool _mac_address_is_reserved(sfx_u8 *mac_address_bytes) {
    // Local variables.
    sfx_bool is_reserved = SIGFOX_TRUE;
    sfx_u8 idx = 0;
    // Check reserved addresses.
    for (idx = 0; idx < SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES; idx++) {
        if ((mac_address_bytes[idx] != 0x00) && (mac_address_bytes[idx] != 0xFF)) {
            is_reserved = SIGFOX_FALSE;
            break;
        }
    }
    return is_reserved;
}

/*******************************************************************/
static sfx_bool _mac_address_is_multicast(sfx_u8 *mac_address_bytes) {
    // Local variables.
    sfx_bool is_multicast = SIGFOX_FALSE;
    // Check IG bit.
    if ((mac_address_bytes[SIGFOX_EP_ADDON_AW_API_IG_BYTE_INDEX] & SIGFOX_EP_ADDON_AW_API_IG_BIT_MASK) != 0) {
        is_multicast = SIGFOX_TRUE;
    }
    return is_multicast;
}

/*******************************************************************/
static sfx_bool _mac_address_is_locally_administered(sfx_u8 *mac_address_bytes) {
    // Local variables.
    sfx_bool is_locally_administered = SIGFOX_FALSE;
    // Check IG bit.
    if ((mac_address_bytes[SIGFOX_EP_ADDON_AW_API_UL_BYTE_INDEX] & SIGFOX_EP_ADDON_AW_API_UL_BIT_MASK) != 0) {
        is_locally_administered = SIGFOX_TRUE;
    }
    return is_locally_administered;
}

/*******************************************************************/
static void _filter_ssid_empty(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid) {
    // Unused parameter.
    SIGFOX_UNUSED(mac_address_bytes);
    // Reset output flag.
    (*access_point_is_valid) = SIGFOX_TRUE;
    // Check if SSID exists.
    if (access_point->ssid == SIGFOX_NULL) {
        (*access_point_is_valid) = SIGFOX_FALSE;
        return;
    }
    // Check if SSID is empty.
    if (access_point->ssid[0] == SIGFOX_EP_ADDON_AW_API_NULL_CHAR) {
        (*access_point_is_valid) = SIGFOX_FALSE;
        return;
    }
}

/*******************************************************************/
static void _filter_ssid_black_list(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid) {
    // Unused parameter.
    SIGFOX_UNUSED(mac_address_bytes);
    // Local variables.
    sfx_u8 ssid_lowercase[SIGFOX_EP_ADDON_AW_API_SSID_SIZE_CHAR];
    sfx_u8 ssid_char = SIGFOX_EP_ADDON_AW_API_NULL_CHAR;
    sfx_u8 ssid_end = 0;
    sfx_u8 idx = 0;
    sfx_u8 char_idx = 0;
    // Reset output flag.
    (*access_point_is_valid) = SIGFOX_TRUE;
    // Check if SSID exists.
    if (access_point->ssid == SIGFOX_NULL) {
        return;
    }
    // Loop on SSID black list.
    for (idx = 0; idx < SIGFOX_EP_ADDON_AW_API_SSID_BLACK_LIST_SIZE; idx++) {
        // Build lowercase SSID.
        ssid_end = 0;
        for (char_idx = 0; char_idx < SIGFOX_EP_ADDON_AW_API_SSID_SIZE_CHAR; char_idx++) {
            // Reset byte and read initial character.
            ssid_lowercase[char_idx] = SIGFOX_EP_ADDON_AW_API_NULL_CHAR;
            ssid_char = (access_point->ssid[char_idx]);
            // Check source string length.
            if (ssid_char == SIGFOX_EP_ADDON_AW_API_NULL_CHAR) {
                ssid_end = 1;
            }
            // Check end.
            if (ssid_end == 0) {
                ssid_lowercase[char_idx] = (((ssid_char >= 'A') && (ssid_char <= 'Z')) ? (ssid_char + SIGFOX_EP_ADDON_AW_UPPERCASE_OFFSET) : (ssid_char));
            }
        }
        // Perform case insensitive comparison.
        if (_search_substring(ssid_lowercase, (sfx_u8*) SIGFOX_EP_ADDON_AW_API_SSID_BLACK_LIST[idx]) == SIGFOX_TRUE) {
            (*access_point_is_valid) = SIGFOX_FALSE;
            return;
        }
    }
}

/*******************************************************************/
static void _filter_locally_administered(SIGFOX_EP_ADDON_AW_API_access_point_t *access_point, sfx_u8 *mac_address_bytes, sfx_bool *access_point_is_valid) {
    // Unused parameter.
    SIGFOX_UNUSED(access_point);
    // Reset output flag.
    (*access_point_is_valid) = SIGFOX_TRUE;
    // Check MAC address type.
    if (_mac_address_is_locally_administered(mac_address_bytes) == SIGFOX_TRUE) {
        (*access_point_is_valid) = SIGFOX_FALSE;
    }
}

/*******************************************************************/
static SIGFOX_EP_ADDON_AW_API_status_t _filter_list(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
    sfx_u8 mac_address_bytes[SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES];
    SIGFOX_EP_ADDON_AW_API_access_point_t *access_point;
    sfx_bool access_point_is_valid = SIGFOX_FALSE;
    sfx_u8 ap_idx = 0;
    sfx_u8 filter_idx = 0;
    // Loop on all access points.
    for (ap_idx = 0; ap_idx < (input_data->access_point_list_size); ap_idx++) {
        // Update access point pointer.
        access_point = (input_data->access_point_list[ap_idx]);
#ifdef SIGFOX_EP_PARAMETERS_CHECK
        if (access_point == SIGFOX_NULL) {
            SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_NULL_PARAMETER);
        }
#endif
        // Check if access point has not already been processed.
        if (access_point->status != SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW) {
            continue;
        }
        // Reset result.
        access_point->status = SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_FILTERED_OUT;
        // Convert ASCII to bytes array.
#ifdef SIGFOX_EP_ERROR_CODES
        status = _mac_address_ascii_to_bytes_array((access_point->mac_address), mac_address_bytes);
        SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
        _mac_address_ascii_to_bytes_array((access_point->mac_address), mac_address_bytes);
#endif
        // Mandatory filters: do not keep reserved and multicast addresses.
        if ((_mac_address_is_reserved(mac_address_bytes) == SIGFOX_TRUE) || (_mac_address_is_multicast(mac_address_bytes) == SIGFOX_TRUE)) {
            continue;
        }
        // Set valid flag to true in case none filter is enabled.
        access_point_is_valid = SIGFOX_TRUE;
        // Optional filters.
        for (filter_idx = 0; filter_idx < SIGFOX_EP_ADDON_AW_API_FILTER_LAST; filter_idx++) {
            // Check mask.
            if ((sigfox_ep_addon_aw_api_ctx.filters & (1 << filter_idx)) != 0) {
                // Execute filter function.
                SIGFOX_EP_ADDON_AW_API_FILTER[filter_idx](access_point, mac_address_bytes, &access_point_is_valid);
                // Directly exit as soon as an active filter fails.
                if (access_point_is_valid == SIGFOX_FALSE) {
                    break;
                }
            }
        }
        if (access_point_is_valid == SIGFOX_TRUE) {
            access_point->status = SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_VALID;
        }
    }
#if ((defined SIGFOX_EP_PARAMETERS_CHECK) || (defined SIGFOX_EP_ERROR_CODES))
errors:
#endif
    SIGFOX_RETURN();
}

/*******************************************************************/
static void _sort_none(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data) {
    // Local variables.
    sfx_u8 ap_idx = 0;
    sfx_u8 ap_select_count = 0;
    // Loop on all access points.
    for (ap_idx = 0; ap_idx < (input_data->access_point_list_size); ap_idx++) {
        // Select elements in initial list order.
        if (input_data->access_point_list[ap_idx]->status == SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_VALID) {
            sigfox_ep_addon_aw_api_ctx.best_index[ap_select_count] = ap_idx;
            ap_select_count++;
        }
        if (ap_select_count >= SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD) {
            break;
        }
    }
}

/*******************************************************************/
static void _sort_rssi(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data) {
    // Local variables.
    sfx_s16 rssi_dbm_max = 0;
    sfx_u8 rssi_dbm_max_idx = 0;
    sfx_u8 ap_idx = 0;
    sfx_bool new_ap_idx = SIGFOX_TRUE;
    sfx_u8 ap_select_count = 0;
    sfx_u8 search_idx = 0;
    sfx_u8 idx = 0;
    // Search loop.
    for (search_idx = 0; search_idx < SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD; search_idx++) {
        // Reset reference.
        rssi_dbm_max = SIGFOX_EP_ADDON_AW_API_S16_MIN;
        rssi_dbm_max_idx = SIGFOX_EP_ADDON_AW_API_BEST_INDEX_NONE;
        // Loop on all access points.
        for (ap_idx = 0; ap_idx < (input_data->access_point_list_size); ap_idx++) {
            // Directly exit if the access point has been filtered out.
            if (input_data->access_point_list[ap_idx]->status != SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_VALID) continue;
            // Compare to reference.
            if ((input_data->access_point_list[ap_idx]->rssi_dbm) > rssi_dbm_max) {
                // Check if index has not already been selected.
                new_ap_idx = SIGFOX_TRUE;
                for (idx = 0; idx < SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD; idx++) {
                    if (ap_idx == sigfox_ep_addon_aw_api_ctx.best_index[idx]) {
                        new_ap_idx = SIGFOX_FALSE;
                        break;
                    }
                }
                if (new_ap_idx == SIGFOX_TRUE) {
                    // Store new reference.
                    rssi_dbm_max = (input_data->access_point_list[ap_idx]->rssi_dbm);
                    rssi_dbm_max_idx = ap_idx;
                }
            }
        }
        sigfox_ep_addon_aw_api_ctx.best_index[ap_select_count] = rssi_dbm_max_idx;
        ap_select_count++;
    }
}

/*******************************************************************/
static SIGFOX_EP_ADDON_AW_API_status_t _sort_list(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
    sfx_u8 idx = 0;
    // Reset selected indexes.
    for (idx = 0; idx < SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD; idx++) {
        sigfox_ep_addon_aw_api_ctx.best_index[idx] = SIGFOX_EP_ADDON_AW_API_BEST_INDEX_NONE;
    }
    // Execute sorting function.
    SIGFOX_EP_ADDON_AW_API_SORT[sigfox_ep_addon_aw_api_ctx.sorting](input_data);
    SIGFOX_RETURN();
}

/*** SIGFOX EP ADDON AW API functions ***/

/*******************************************************************/
SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_set_filter(sfx_u8 filters, SIGFOX_EP_ADDON_AW_API_sorting_t sorting) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
#ifdef SIGFOX_EP_PARAMETERS_CHECK
    // Check parameters.
    if (sorting >= SIGFOX_EP_ADDON_AW_API_SORTING_LAST) {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_SORTING);
    }
#endif
    // Update local context.
    sigfox_ep_addon_aw_api_ctx.filters = filters;
    sigfox_ep_addon_aw_api_ctx.sorting = sorting;
#ifdef SIGFOX_EP_PARAMETERS_CHECK
errors:
#endif
    SIGFOX_RETURN();
}

/*******************************************************************/
SIGFOX_EP_ADDON_AW_API_status_t SIGFOX_EP_ADDON_AW_API_build_ul_payload(SIGFOX_EP_ADDON_AW_API_input_data_t *input_data, sfx_u8 *ul_payload, sfx_u8 *nb_mac_ul_payload) {
    // Local variables.
#ifdef SIGFOX_EP_ERROR_CODES
    SIGFOX_EP_ADDON_AW_API_status_t status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
#endif
    sfx_u8 mac_address_bytes[SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES];
    sfx_u8 ap_idx = 0;
    sfx_u8 best_idx = 0;
    sfx_u8 byte_idx = 0;
#ifdef SIGFOX_EP_PARAMETERS_CHECK
    if ((input_data == SIGFOX_NULL) || (ul_payload == SIGFOX_NULL) || (nb_mac_ul_payload == SIGFOX_NULL)) {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_NULL_PARAMETER);
    }
    if (input_data->access_point_list == SIGFOX_NULL) {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_NULL_PARAMETER);
    }
    if (input_data->access_point_list_size == 0) {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_ACCESS_POINT_LIST_SIZE);
    }
#endif
    // Reset payload and MAC address count.
    for (byte_idx = 0; byte_idx < SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES; byte_idx ++) {
        ul_payload[byte_idx] = 0x00;
    }
    (*nb_mac_ul_payload) = 0;
    // Apply filters.
#ifdef SIGFOX_EP_ERROR_CODES
    status = _filter_list(input_data);
    SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
    _filter_list(input_data);
#endif
    // Sort list.
#ifdef SIGFOX_EP_ERROR_CODES
    status = _sort_list(input_data);
    SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
    _sort_list(input_data);
#endif
    // Select the best indexes.
    for (ap_idx = 0; ap_idx < SIGFOX_EP_ADDON_AW_API_NB_MAC_UL_PAYLOAD; ap_idx++) {
        // Read best index.
        best_idx = sigfox_ep_addon_aw_api_ctx.best_index[ap_idx];
        // Check best index.
        if (best_idx != SIGFOX_EP_ADDON_AW_API_BEST_INDEX_NONE) {
            // Convert ASCII to bytes array.
#ifdef SIGFOX_EP_ERROR_CODES
            status = _mac_address_ascii_to_bytes_array((input_data->access_point_list[best_idx]->mac_address), mac_address_bytes);
            SIGFOX_CHECK_STATUS(SIGFOX_EP_ADDON_AW_API_SUCCESS);
#else
            _mac_address_ascii_to_bytes_array((input_data->access_point_list[best_idx]->mac_address), mac_address_bytes);
#endif
            // Fill payload.
            for (byte_idx = 0; byte_idx < SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES; byte_idx++) {
                ul_payload[(SIGFOX_EP_ADDON_AW_API_MAC_ADDRESS_SIZE_BYTES * (*nb_mac_ul_payload)) + byte_idx] = mac_address_bytes[byte_idx];
            }
            // Update access point status.
            input_data->access_point_list[best_idx]->status = SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_SENT;
            // Update MAC address count.
            (*nb_mac_ul_payload)++;
        }
    }
    // Check if at least one valid MAC address has been found.
    if ((*nb_mac_ul_payload) == 0) {
        SIGFOX_EXIT_ERROR(SIGFOX_EP_ADDON_AW_API_ERROR_NONE_VALID_ACCESS_POINT);
    }
errors:
    SIGFOX_RETURN();
}

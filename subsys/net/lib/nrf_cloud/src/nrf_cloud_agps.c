/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gps.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <nrf_modem_gnss.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud_agps.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include <stdio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_agps, CONFIG_NRF_CLOUD_GPS_LOG_LEVEL);

#include "nrf_cloud_codec.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_agps_schema_v1.h"

#define AGPS_JSON_TYPES_KEY		"types"

extern void agps_print(enum nrf_cloud_agps_type type, void *data);

static K_SEM_DEFINE(agps_injection_active, 1, 1);

static int fd = -1;
static bool agps_print_enabled;
static const struct device *gps_dev;
static struct gps_agps_request processed;
static atomic_t request_in_progress;

static enum gps_agps_type type_lookup_socket2gps[] = {
	[NRF_GNSS_AGPS_UTC_PARAMETERS]	= GPS_AGPS_UTC_PARAMETERS,
	[NRF_GNSS_AGPS_EPHEMERIDES]	= GPS_AGPS_EPHEMERIDES,
	[NRF_GNSS_AGPS_ALMANAC]		= GPS_AGPS_ALMANAC,
	[NRF_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION]
					= GPS_AGPS_KLOBUCHAR_CORRECTION,
	[NRF_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION]
					= GPS_AGPS_NEQUICK_CORRECTION,
	[NRF_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS]
					= GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS,
	[NRF_GNSS_AGPS_LOCATION]	= GPS_AGPS_LOCATION,
	[NRF_GNSS_AGPS_INTEGRITY]	= GPS_AGPS_INTEGRITY,
};

void agps_print_enable(bool enable)
{
	agps_print_enabled = enable;
}

bool nrf_cloud_agps_request_in_progress(void)
{
	return atomic_get(&request_in_progress) != 0;
}

#if IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)
static int json_add_types_array(cJSON *const obj, enum gps_agps_type *types,
				const size_t type_count)
{
	__ASSERT_NO_MSG(obj != NULL);
	__ASSERT_NO_MSG(types != NULL);

	cJSON *array;

	if (!type_count) {
		return -EINVAL;
	}

	array = cJSON_AddArrayToObject(obj, AGPS_JSON_TYPES_KEY);
	if (!array) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < type_count; i++) {
		cJSON_AddItemToArray(array, cJSON_CreateNumber(types[i]));
	}

	if (cJSON_GetArraySize(array) != type_count) {
		cJSON_DeleteItemFromObject(obj, AGPS_JSON_TYPES_KEY);
		return -ENOMEM;
	}

	return 0;
}
#endif /* IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) */

int nrf_cloud_agps_request(const struct gps_agps_request request)
{
#if IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)
	int err;
	enum gps_agps_type types[9];
	size_t type_count = 0;
	cJSON *data_obj;
	cJSON *agps_req_obj;

	atomic_set(&request_in_progress, 0);
	memset(&processed, 0, sizeof(processed));

	if (request.utc) {
		types[type_count++] = GPS_AGPS_UTC_PARAMETERS;
	}

#if !defined(CONFIG_NRF_CLOUD_PGPS)
	if (request.sv_mask_ephe) {
		types[type_count++] = GPS_AGPS_EPHEMERIDES;
	}

	if (request.sv_mask_alm) {
		types[type_count++] = GPS_AGPS_ALMANAC;
	}
#endif

	if (request.klobuchar) {
		types[type_count++] = GPS_AGPS_KLOBUCHAR_CORRECTION;
	}

	if (request.nequick) {
		types[type_count++] = GPS_AGPS_NEQUICK_CORRECTION;
	}

	if (request.system_time_tow) {
		types[type_count++] = GPS_AGPS_GPS_TOWS;
		types[type_count++] = GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS;
	}

	if (request.position) {
		types[type_count++] = GPS_AGPS_LOCATION;
	}

	if (request.integrity) {
		types[type_count++] = GPS_AGPS_INTEGRITY;
	}

	if (type_count == 0) {
		LOG_INF("No A-GPS data types requested");
		return 0;
	}

	/* Create request JSON containing a data object */
	agps_req_obj = json_create_req_obj(NRF_CLOUD_JSON_APPID_VAL_AGPS,
					   NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	data_obj = cJSON_AddObjectToObject(agps_req_obj, NRF_CLOUD_JSON_DATA_KEY);

	if (!agps_req_obj || !data_obj) {
		err = -ENOMEM;
		goto cleanup;
	}

	/* Add modem info and A-GPS types to the data object */
	err = nrf_cloud_json_add_modem_info(data_obj);
	if (err) {
		LOG_ERR("Failed to add modem info to A-GPS request: %d", err);
		goto cleanup;
	}
	err = json_add_types_array(data_obj, types, type_count);
	if (err) {
		LOG_ERR("Failed to add types array to A-GPS request %d", err);
		goto cleanup;
	}

	err = json_send_to_cloud(agps_req_obj);
	if (!err) {
		atomic_set(&request_in_progress, 1);
	}

cleanup:
	cJSON_Delete(agps_req_obj);

	return err;
#else /* IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) */

	LOG_ERR("CONFIG_NRF_CLOUD_MQTT must be enabled in order to use this API");

	return -ENOTSUP;
#endif
}

int nrf_cloud_agps_request_all(void)
{
	struct gps_agps_request request = {
		.sv_mask_ephe = 0xFFFFFFFF,
		.sv_mask_alm = 0xFFFFFFFF,
		.utc = 1,
		.klobuchar = 1,
		.system_time_tow = 1,
		.position = 1,
		.integrity = 1,
	};

	return nrf_cloud_agps_request(request);
}

/* Convert nrf_socket A-GPS type to GPS API type. */
static inline enum gps_agps_type type_socket2gps(
	nrf_gnss_agps_data_type_t type)
{
	return type_lookup_socket2gps[type];
}

static int send_to_modem(void *data, size_t data_len,
			 nrf_gnss_agps_data_type_t type)
{
	int err;

	if (agps_print_enabled) {
		agps_print(type, data);
	}

	if (gps_dev) {
		/* GPS driver */
		err = gps_agps_write(gps_dev, type_socket2gps(type), data, data_len);
	} else if (fd != -1) {
		/* GNSS socket */
		err = nrf_sendto(fd, data, data_len, 0, &type, sizeof(type));
		if (err < 0) {
			err = -errno;
		} else {
			err = 0;
		}
	} else {
		/* GNSS API */
		err = nrf_modem_gnss_agps_write(data, data_len, type);
	}

	return err;
}

static int copy_utc(nrf_gnss_agps_data_utc_t *dst,
		    struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->a1		= src->utc->a1;
	dst->a0		= src->utc->a0;
	dst->tot	= src->utc->tot;
	dst->wn_t	= src->utc->wn_t;
	dst->delta_tls	= src->utc->delta_tls;
	dst->wn_lsf	= src->utc->wn_lsf;
	dst->dn		= src->utc->dn;
	dst->delta_tlsf	= src->utc->delta_tlsf;

	return 0;
}

static int copy_almanac(nrf_gnss_agps_data_almanac_t *dst,
			struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->sv_id	= src->almanac->sv_id;
	dst->wn		= src->almanac->wn;
	dst->toa	= src->almanac->toa;
	dst->ioda	= src->almanac->ioda;
	dst->e		= src->almanac->e;
	dst->delta_i	= src->almanac->delta_i;
	dst->omega_dot	= src->almanac->omega_dot;
	dst->sv_health	= src->almanac->sv_health;
	dst->sqrt_a	= src->almanac->sqrt_a;
	dst->omega0	= src->almanac->omega0;
	dst->w		= src->almanac->w;
	dst->m0		= src->almanac->m0;
	dst->af0	= src->almanac->af0;
	dst->af1	= src->almanac->af1;

	return 0;
}

static int copy_ephemeris(nrf_gnss_agps_data_ephemeris_t *dst,
			  struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->sv_id	= src->ephemeris->sv_id;
	dst->health	= src->ephemeris->health;
	dst->iodc	= src->ephemeris->iodc;
	dst->toc	= src->ephemeris->toc;
	dst->af2	= src->ephemeris->af2;
	dst->af1	= src->ephemeris->af1;
	dst->af0	= src->ephemeris->af0;
	dst->tgd	= src->ephemeris->tgd;
	dst->ura	= src->ephemeris->ura;
	dst->fit_int	= src->ephemeris->fit_int;
	dst->toe	= src->ephemeris->toe;
	dst->w		= src->ephemeris->w;
	dst->delta_n	= src->ephemeris->delta_n;
	dst->m0		= src->ephemeris->m0;
	dst->omega_dot	= src->ephemeris->omega_dot;
	dst->e		= src->ephemeris->e;
	dst->idot	= src->ephemeris->idot;
	dst->sqrt_a	= src->ephemeris->sqrt_a;
	dst->i0		= src->ephemeris->i0;
	dst->omega0	= src->ephemeris->omega0;
	dst->crs	= src->ephemeris->crs;
	dst->cis	= src->ephemeris->cis;
	dst->cus	= src->ephemeris->cus;
	dst->crc	= src->ephemeris->crc;
	dst->cic	= src->ephemeris->cic;
	dst->cuc	= src->ephemeris->cuc;

	return 0;
}

static int copy_klobuchar(nrf_gnss_agps_data_klobuchar_t *dst,
			  struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->alpha0	= src->ion_correction.klobuchar->alpha0;
	dst->alpha1	= src->ion_correction.klobuchar->alpha1;
	dst->alpha2	= src->ion_correction.klobuchar->alpha2;
	dst->alpha3	= src->ion_correction.klobuchar->alpha3;
	dst->beta0	= src->ion_correction.klobuchar->beta0;
	dst->beta1	= src->ion_correction.klobuchar->beta1;
	dst->beta2	= src->ion_correction.klobuchar->beta2;
	dst->beta3	= src->ion_correction.klobuchar->beta3;

	return 0;
}

static int copy_location(nrf_gnss_agps_data_location_t *dst,
			 struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->latitude		= src->location->latitude;
	dst->longitude		= src->location->longitude;
	dst->altitude		= src->location->altitude;
	dst->unc_semimajor	= src->location->unc_semimajor;
	dst->unc_semiminor	= src->location->unc_semiminor;
	dst->orientation_major	= src->location->orientation_major;
	dst->unc_altitude	= src->location->unc_altitude;
	dst->confidence		= src->location->confidence;

	return 0;
}

static int copy_time_and_tow(nrf_gnss_agps_data_system_time_and_sv_tow_t *dst,
			     struct nrf_cloud_apgs_element *src)
{
	if ((src == NULL) || (dst == NULL)) {
		return -EINVAL;
	}

	dst->date_day		= src->time_and_tow->date_day;
	dst->time_full_s	= src->time_and_tow->time_full_s;
	dst->time_frac_ms	= src->time_and_tow->time_frac_ms;
	dst->sv_mask		= src->time_and_tow->sv_mask;

	if (src->time_and_tow->sv_mask == 0U) {
		LOG_DBG("SW TOW mask is zero, not copying TOW array");
		memset(dst->sv_tow, 0, sizeof(dst->sv_tow));

		return 0;
	}

	for (size_t i = 0; i < NRF_CLOUD_AGPS_MAX_SV_TOW; i++) {
		dst->sv_tow[i].flags = src->time_and_tow->sv_tow[i].flags;
		dst->sv_tow[i].tlm = src->time_and_tow->sv_tow[i].tlm;
	}

	return 0;
}

static int agps_send_to_modem(struct nrf_cloud_apgs_element *agps_data)
{
	atomic_set(&request_in_progress, 0);

	switch (agps_data->type) {
	case NRF_CLOUD_AGPS_UTC_PARAMETERS: {
		nrf_gnss_agps_data_utc_t utc;

		processed.utc = true;
		copy_utc(&utc, agps_data);
#if defined(CONFIG_NRF_CLOUD_PGPS)
		nrf_cloud_pgps_set_leap_seconds(utc.delta_tls);
#endif
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_UTC_PARAMETERS");

		return send_to_modem(&utc, sizeof(utc),
				     NRF_GNSS_AGPS_UTC_PARAMETERS);
	}
	case NRF_CLOUD_AGPS_EPHEMERIDES: {
		nrf_gnss_agps_data_ephemeris_t ephemeris;

		processed.sv_mask_ephe |= (1 << (agps_data->ephemeris->sv_id - 1));
#if defined(CONFIG_NRF_CLOUD_PGPS)
		if (agps_data->ephemeris->health ==
		    NRF_CLOUD_PGPS_EMPTY_EPHEM_HEALTH) {
			LOG_DBG("Skipping empty ephemeris for sv %u",
				agps_data->ephemeris->sv_id);
			return 0;
		}
#endif
		copy_ephemeris(&ephemeris, agps_data);
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_EPHEMERIDES %d",
			agps_data->ephemeris->sv_id);

		return send_to_modem(&ephemeris, sizeof(ephemeris),
				     NRF_GNSS_AGPS_EPHEMERIDES);
	}
	case NRF_CLOUD_AGPS_ALMANAC: {
		nrf_gnss_agps_data_almanac_t almanac;

		processed.sv_mask_alm |= (1 << (agps_data->almanac->sv_id - 1));
		copy_almanac(&almanac, agps_data);
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_ALMANAC %d",
			agps_data->almanac->sv_id);

		return send_to_modem(&almanac, sizeof(almanac),
				     NRF_GNSS_AGPS_ALMANAC);
	}
	case NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION: {
		nrf_gnss_agps_data_klobuchar_t klobuchar;

		processed.klobuchar = true;
		copy_klobuchar(&klobuchar, agps_data);
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION");

		return send_to_modem(&klobuchar, sizeof(klobuchar),
				NRF_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION);
	}
	case NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK: {
		nrf_gnss_agps_data_system_time_and_sv_tow_t time_and_tow;

		processed.system_time_tow = true;
		copy_time_and_tow(&time_and_tow, agps_data);
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK");

		return send_to_modem(&time_and_tow, sizeof(time_and_tow),
				     NRF_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS);
	}
	case NRF_CLOUD_AGPS_LOCATION: {
		nrf_gnss_agps_data_location_t location = {0};

		processed.position = true;
		copy_location(&location, agps_data);
#if defined(CONFIG_NRF_CLOUD_PGPS)
		nrf_cloud_pgps_set_location_normalized(location.latitude,
						  location.longitude);
#endif
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_LOCATION");

		return send_to_modem(&location, sizeof(location),
				     NRF_GNSS_AGPS_LOCATION);
	}
	case NRF_CLOUD_AGPS_INTEGRITY:
		LOG_DBG("A-GPS type: NRF_CLOUD_AGPS_INTEGRITY");

		processed.integrity = true;
		return send_to_modem(agps_data->integrity,
				     sizeof(agps_data->integrity),
				     NRF_GNSS_AGPS_INTEGRITY);
	default:
		LOG_WRN("Unknown AGPS data type: %d", agps_data->type);
		break;
	}

	return 0;
}

static size_t get_next_agps_element(struct nrf_cloud_apgs_element *element,
				    const char *buf)
{
	static uint16_t elements_left_to_process;
	static enum nrf_cloud_agps_type element_type;
	size_t len = 0;

	/* Check if there are more elements left in the array to process.
	 * The element type is only given once before the array, and not for
	 * each element.
	 */
	if (elements_left_to_process == 0) {
		element->type =
			(enum nrf_cloud_agps_type)buf[NRF_CLOUD_AGPS_BIN_TYPE_OFFSET];
		element_type = element->type;
		elements_left_to_process =
			*(uint16_t *)&buf[NRF_CLOUD_AGPS_BIN_COUNT_OFFSET] - 1;
		len += NRF_CLOUD_AGPS_BIN_TYPE_SIZE +
			NRF_CLOUD_AGPS_BIN_COUNT_SIZE;
	} else {
		element->type = element_type;
		elements_left_to_process -= 1;
	}

	switch (element->type) {
	case NRF_CLOUD_AGPS_UTC_PARAMETERS:
		element->utc = (struct nrf_cloud_agps_utc *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_utc);
		break;
	case NRF_CLOUD_AGPS_EPHEMERIDES:
		element->ephemeris = (struct nrf_cloud_agps_ephemeris *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_ephemeris);
		break;
	case NRF_CLOUD_AGPS_ALMANAC:
		element->almanac = (struct nrf_cloud_agps_almanac *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_almanac);
		break;
	case NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION:
		element->ion_correction.klobuchar =
			(struct nrf_cloud_agps_klobuchar *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_klobuchar);
		break;
	case NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK:
		element->time_and_tow =
			(struct nrf_cloud_agps_system_time *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_system_time) -
			sizeof(element->time_and_tow->sv_tow) + 4;
		break;
	case NRF_CLOUD_AGPS_GPS_TOWS:
		element->tow =
			(struct nrf_cloud_agps_tow_element *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_tow_element);
		break;
	case NRF_CLOUD_AGPS_LOCATION:
		element->location = (struct nrf_cloud_agps_location *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_location);
		break;
	case NRF_CLOUD_AGPS_INTEGRITY:
		element->integrity =
			(struct nrf_cloud_agps_integrity *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_integrity);
		break;
	default:
		LOG_DBG("Unhandled A-GPS data type: %d", element->type);
		return 0;
	}

	return len;
}

int nrf_cloud_agps_process(const char *buf, size_t buf_len, const int *socket)
{
	int err;
	struct nrf_cloud_apgs_element element = {0};
	struct nrf_cloud_agps_system_time sys_time = {0};
	uint32_t sv_mask = 0;
	size_t parsed_len = 0;
	uint8_t version;

	version = buf[NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_INDEX];
	parsed_len += NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_SIZE;

	if (version != NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION) {
		LOG_ERR("Cannot parse schema version: %d", version);
		return -EBADMSG;
	}

	LOG_DBG("Received AGPS data. Schema version: %d, length: %d",
		version, buf_len);

	err = k_sem_take(&agps_injection_active, K_FOREVER);
	if (err) {
		LOG_ERR("A-GPS injection already active.");
		return err;
	}

	LOG_DBG("A-GPS_injection_active LOCKED");

	if (socket) {
		LOG_DBG("Using user-provided socket, fd %d", fd);

		gps_dev = NULL;
		fd = *socket;
	} else {
		gps_dev = device_get_binding("NRF9160_GPS");
		if (gps_dev != NULL) {
			LOG_DBG("Using GPS driver to input assistance data");
		} else {
			LOG_DBG("Using GNSS API to input assistance data");
		}
	}

	while (parsed_len < buf_len) {
		size_t element_size =
			get_next_agps_element(&element, &buf[parsed_len]);

		if (element_size == 0) {
			LOG_DBG("Parsing finished\n");
			break;
		}

		parsed_len += element_size;

		LOG_DBG("Parsed_len: %d", parsed_len);

		if (element.type == NRF_CLOUD_AGPS_GPS_TOWS) {
			memcpy(&sys_time.sv_tow[element.tow->sv_id - 1],
				element.tow,
				sizeof(sys_time.sv_tow[0]));
			if (element.tow->flags || element.tow->tlm) {
				sv_mask |= 1 << (element.tow->sv_id - 1);
			}

			LOG_DBG("TOW %d copied", element.tow->sv_id - 1);

			continue;
		} else if (element.type == NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK) {
			memcpy(&sys_time, element.time_and_tow,
				sizeof(sys_time) - sizeof(sys_time.sv_tow));
			sys_time.sv_mask = sv_mask | element.time_and_tow->sv_mask;
			LOG_DBG("TOWs copied, bitmask: 0x%08x",
				sys_time.sv_mask);
			element.time_and_tow = &sys_time;
		}

		err = agps_send_to_modem(&element);
		if (err) {
			LOG_ERR("Failed to send data to modem, error: %d", err);
			break;
		}
	}

	LOG_DBG("A-GPS_inject_active UNLOCKED");
	k_sem_give(&agps_injection_active);

	return err;
}

void nrf_cloud_agps_processed(struct gps_agps_request *received_elements)
{
	if (received_elements) {
		memcpy(received_elements, &processed, sizeof(struct gps_agps_request));
	}
}

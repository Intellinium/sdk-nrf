/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <shell/shell.h>
#include <getopt.h>

#include "gnss.h"

extern const struct shell *shell_global;

static bool gnss_running;

static int print_help(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		shell_error(shell, "%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}

static int cmd_gnss(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_start(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (gnss_running) {
		shell_error(shell, "start: GNSS already running");
		return -ENOEXEC;
	}

	err = gnss_start();
	if (!err) {
		gnss_running = true;
	} else {
		shell_error(shell, "start: starting GNSS failed, err %d", err);
	}

	return err;
}

static int cmd_gnss_stop(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (!gnss_running) {
		shell_error(shell, "stop: GNSS not running");
		return -ENOEXEC;
	}

	err = gnss_stop();
	if (!err) {
		gnss_running = false;
	} else {
		shell_error(shell, "stop: stopping GNSS failed, err %d", err);
	}

	return err;
}

static int cmd_gnss_delete(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_delete_ephe(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_delete_data(GNSS_DATA_DELETE_EPHEMERIDES);
}

static int cmd_gnss_delete_all(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_delete_data(GNSS_DATA_DELETE_ALL);
}

static int cmd_gnss_mode(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_mode_cont(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_continuous_mode();
}

static int cmd_gnss_mode_single(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int timeout;

	timeout = atoi(argv[1]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		shell_error(
			shell,
			"single: invalid timeout value %d",
			timeout);
		return -EINVAL;
	}

	return gnss_set_single_fix_mode(timeout);
}

static int cmd_gnss_mode_periodic(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int interval;
	int timeout;

	interval = atoi(argv[1]);
	if (interval <= 0) {
		shell_error(
			shell,
			"periodic: invalid interval value %d, the value must be greater than 0",
			interval);
		return -EINVAL;
	}

	timeout = atoi(argv[2]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		shell_error(
			shell,
			"periodic: invalid timeout value %d, the value must be 0...65535",
			timeout);
		return -EINVAL;
	}

	return gnss_set_periodic_fix_mode(interval, timeout);
}

static int cmd_gnss_mode_periodic_gnss(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int interval;
	int timeout;

	interval = atoi(argv[1]);
	if (interval < 10 || interval > UINT16_MAX) {
		shell_error(
			shell,
			"periodic_gnss: invalid interval value %d, the value must be 10...65535",
			interval);
		return -EINVAL;
	}

	timeout = atoi(argv[2]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		shell_error(
			shell,
			"periodic_gnss: invalid timeout value %d, the value must be 0...65535",
			timeout);
		return -EINVAL;
	}

	return gnss_set_periodic_fix_mode_gnss(interval, timeout);
}

static int cmd_gnss_dynamics(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_dynamics_general(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_GENERAL);
}

static int cmd_gnss_dynamics_stationary(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_STATIONARY);
}

static int cmd_gnss_dynamics_pedestrian(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_PEDESTRIAN);
}

static int cmd_gnss_dynamics_automotive(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_AUTOMOTIVE);
}

static int cmd_gnss_config(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}


static int cmd_gnss_config_system(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int value;
	uint8_t system_mask;

	system_mask = 0x1; /* GPS bit always enabled */

	/* QZSS */
	value = atoi(argv[1]);
	if (value == 1) {
		system_mask |= 0x4;
	}

	return gnss_set_system_mask(system_mask);
}

static int cmd_gnss_config_elevation(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int elevation;

	if (argc != 2) {
		shell_error(shell, "elevation: wrong parameter count");
		shell_print(shell, "elevation: <angle>");
		shell_print(
			shell,
			"angle:\tElevation threshold angle (in degrees, default 5). "
			"Satellites with elevation angle less than the threshold are excluded.");
		return -EINVAL;
	}

	elevation = atoi(argv[1]);

	if (elevation < 0 || elevation > 90) {
		shell_error(shell, "elevation: invalid elevation value %d", elevation);
		return -EINVAL;
	}

	return gnss_set_elevation_threshold(elevation);
}

static int cmd_gnss_config_use_case(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	uint8_t value;
	bool low_accuracy_enabled = false;
	bool scheduled_downloads_disabled = false;

	value = atoi(argv[1]);
	if (value == 1) {
		low_accuracy_enabled = true;
	}

	value = atoi(argv[2]);
	if (value == 1) {
		scheduled_downloads_disabled = true;
	}

	return gnss_set_use_case(low_accuracy_enabled, scheduled_downloads_disabled);
}

static int cmd_gnss_config_nmea(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	uint8_t value;
	uint16_t nmea_mask;
	uint16_t nmea_mask_bit;

	nmea_mask = 0;
	nmea_mask_bit = 1;
	for (int i = 0; i < 5; i++) {
		value = atoi(argv[i + 1]);
		if (value == 1) {
			nmea_mask |= nmea_mask_bit;
		}
		nmea_mask_bit = nmea_mask_bit << 1;
	}

	return gnss_set_nmea_mask(nmea_mask);
}

static int cmd_gnss_config_powersave(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_config_powersave_off(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_DISABLED);
}

static int cmd_gnss_config_powersave_perf(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_PERFORMANCE);
}

static int cmd_gnss_config_powersave_power(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_POWER);
}

static int cmd_gnss_config_qzss(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_config_qzss_nmea(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_config_qzss_nmea_standard(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_qzss_nmea_mode(GNSS_QZSS_NMEA_MODE_STANDARD);
}

static int cmd_gnss_config_qzss_nmea_custom(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_qzss_nmea_mode(GNSS_QZSS_NMEA_MODE_CUSTOM);
}

static int cmd_gnss_config_qzss_mask(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int value;
	uint16_t qzss_mask;
	uint16_t qzss_mask_bit;

	qzss_mask = 0;
	qzss_mask_bit = 1;
	for (int i = 0; i < 10; i++) {
		value = atoi(argv[i + 1]);
		if (value == 1) {
			qzss_mask |= qzss_mask_bit;
		}
		qzss_mask_bit = qzss_mask_bit << 1;
	}

	return gnss_set_qzss_mask(qzss_mask);
}

static int cmd_gnss_config_timing(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_config_timing_rtc(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_timing_source(GNSS_TIMING_SOURCE_RTC);
}

static int cmd_gnss_config_timing_tcxo(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_timing_source(GNSS_TIMING_SOURCE_TCXO);
}

static int cmd_gnss_priority(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_priority_enable(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_priority_time_windows(true);
}

static int cmd_gnss_priority_disable(const struct shell *shell, size_t argc, char **argv)
{
	if (!gnss_running) {
		shell_error(shell, "%s: start GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	return gnss_set_priority_time_windows(false);
}

static int cmd_gnss_agps_automatic(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_agps_automatic_enable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agps_automatic(true);
}

static int cmd_gnss_agps_automatic_disable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agps_automatic(false);
}

static int cmd_gnss_agps(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_agps_inject(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_inject_agps_data();
}

static int cmd_gnss_agps_filter(const struct shell *shell, size_t argc, char **argv)
{
	bool ephe_enabled;
	bool alm_enabled;
	bool utc_enabled;
	bool klob_enabled;
	bool neq_enabled;
	bool time_enabled;
	bool pos_enabled;
	bool int_enabled;

	if (argc != 9) {
		shell_error(shell, "filter: wrong parameter count");
		shell_print(shell,
			    "filter: <ephe> <alm> <utc> <klob> <neq> <time> <pos> <integrity>");
		shell_print(shell, "ephe:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "alm:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "utc:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "klob:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "neq:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "time:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "pos:\n  0 = disabled\n  1 = enabled");
		shell_print(shell, "integrity:\n  0 = disabled\n  1 = enabled");
		return -EINVAL;
	}

	ephe_enabled = atoi(argv[1]) == 1 ? true : false;
	alm_enabled = atoi(argv[2]) == 1 ? true : false;
	utc_enabled = atoi(argv[3]) == 1 ? true : false;
	klob_enabled = atoi(argv[4]) == 1 ? true : false;
	neq_enabled = atoi(argv[5]) == 1 ? true : false;
	time_enabled = atoi(argv[6]) == 1 ? true : false;
	pos_enabled = atoi(argv[7]) == 1 ? true : false;
	int_enabled = atoi(argv[8]) == 1 ? true : false;

	return gnss_set_agps_data_enabled(ephe_enabled, alm_enabled, utc_enabled,
					  klob_enabled, neq_enabled, time_enabled,
					  pos_enabled, int_enabled);
}

static int cmd_gnss_1pps(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

static int cmd_gnss_1pps_enable(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int interval;
	int pulse_width;

	interval = atoi(argv[1]);
	if (interval < 0 || interval > 1800) {
		shell_error(
			shell,
			"start: invalid interval value %d, the value must be 0...1800",
			interval);
		return -EINVAL;
	}

	pulse_width = atoi(argv[2]);
	if (pulse_width < 1 || pulse_width > 500) {
		shell_error(
			shell,
			"start: invalid pulse width value %d, the value must be 1...500",
			interval);
		return -EINVAL;
	}

	struct gnss_1pps_mode mode = { 0 };

	mode.enable = true;
	mode.pulse_interval = interval;
	mode.pulse_width = pulse_width;
	mode.apply_start_time = false;

	return gnss_set_1pps_mode(&mode);
}

/* Parses a date string in format "dd.mm.yyyy" */
static int parse_date_string(char *date_string, uint16_t *year, uint8_t *month, uint8_t *day)
{
	char number[5];
	int value;

	if (strlen(date_string) != 10) {
		return -1;
	}

	/* Day */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 1 && value <= 31) {
		*day = value;
	} else {
		return -1;
	}

	if (*date_string != '.') {
		return -1;
	}
	date_string++;

	/* Month */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 1 && value <= 12) {
		*month = value;
	} else {
		return -1;
	}

	if (*date_string != '.') {
		return -1;
	}
	date_string++;

	/* Year */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = *date_string;
	date_string++;
	number[3] = *date_string;
	number[4] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 4000) {
		*year = value;
	} else {
		return -1;
	}

	return 0;
}

/* Parses a time string in format "hh:mm:ss" */
static int parse_time_string(char *time_string, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	char number[3];
	int value;

	if (strlen(time_string) != 8) {
		return -1;
	}

	/* Hour */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	time_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 23) {
		*hour = value;
	} else {
		return -1;
	}

	if (*time_string != ':') {
		return -1;
	}
	time_string++;

	/* Minute */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	time_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 59) {
		*minute = value;
	} else {
		return -1;
	}

	if (*time_string != ':') {
		return -1;
	}
	time_string++;

	/* Second */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 59) {
		*second = value;
	} else {
		return -1;
	}

	return 0;
}

static int cmd_gnss_1pps_enable_at(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	int interval;
	int pulse_width;

	interval = atoi(argv[1]);
	if (interval < 0 || interval > 1800) {
		shell_error(
			shell,
			"start_at: invalid interval value %d, the value must be 0...1800",
			interval);
		return -EINVAL;
	}

	pulse_width = atoi(argv[2]);
	if (pulse_width < 1 || pulse_width > 500) {
		shell_error(
			shell,
			"start_at: invalid pulse width value %d, the value must be 1...500",
			interval);
		return -EINVAL;
	}

	struct gnss_1pps_mode mode = { 0 };

	mode.enable = true;
	mode.pulse_interval = interval;
	mode.pulse_width = pulse_width;
	mode.apply_start_time = true;

	/* Parse date */
	if (parse_date_string(argv[3], &mode.year, &mode.month, &mode.day) != 0) {
		shell_error(
			shell,
			"start_at: invalid date string %s",
			argv[3]);
		return -EINVAL;
	}

	/* Parse time */
	if (parse_time_string(argv[4], &mode.hour, &mode.minute, &mode.second) != 0) {
		shell_error(
			shell,
			"start_at: invalid time string %s",
			argv[4]);
		return -EINVAL;
	}

	return gnss_set_1pps_mode(&mode);
}

static int cmd_gnss_1pps_disable(const struct shell *shell, size_t argc, char **argv)
{
	if (gnss_running) {
		shell_error(shell, "%s: stop GNSS to execute command", argv[0]);
		return -ENOEXEC;
	}

	struct gnss_1pps_mode mode = { 0 };

	mode.enable = false;

	return gnss_set_1pps_mode(&mode);
}

static int cmd_gnss_output(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	int pvt_level;
	int nmea_level;
	int event_level;

	if (argc != 4) {
		shell_error(shell, "output: wrong parameter count");
		shell_print(shell, "output: <pvt level> <nmea level> <event level>");
		shell_print(shell, "pvt level:\n  0 = no PVT output\n  1 = PVT output");
		shell_print(shell, "  2 = PVT output with SV information (default)");
		shell_print(shell, "nmea level:\n  0 = no NMEA output (default)\n"
				   "  1 = NMEA output");
		shell_print(shell, "event level:\n  0 = no event output (default)\n"
				   "  1 = event output");
		return -EINVAL;
	}

	pvt_level = atoi(argv[1]);
	nmea_level = atoi(argv[2]);
	event_level = atoi(argv[3]);

	err = gnss_set_pvt_output_level(pvt_level);
	if (err) {
		shell_error(shell_global, "output: invalid PVT output level");
	}
	err = gnss_set_nmea_output_level(nmea_level);
	if (err) {
		shell_error(shell_global, "output: invalid NMEA output level");
	}
	err = gnss_set_event_output_level(event_level);
	if (err) {
		shell_error(shell_global, "output: invalid event output level");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_delete,
	SHELL_CMD_ARG(ephe, NULL, "Delete ephemerides (forces a warm start).",
		      cmd_gnss_delete_ephe, 1, 0),
	SHELL_CMD_ARG(all, NULL, "Delete all data (forces a cold start).",
		      cmd_gnss_delete_all, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_mode,
	SHELL_CMD_ARG(cont, NULL, "Continuous tracking mode (default).", cmd_gnss_mode_cont, 1, 0),
	SHELL_CMD_ARG(single, NULL, "<timeout>\nSingle fix mode.", cmd_gnss_mode_single, 2, 0),
	SHELL_CMD_ARG(periodic, NULL,
		      "<interval> <timeout>\nPeriodic fix mode controlled by application.",
		      cmd_gnss_mode_periodic, 3, 0),
	SHELL_CMD_ARG(periodic_gnss, NULL,
		      "<interval> <timeout>\nPeriodic fix mode controlled by GNSS.",
		      cmd_gnss_mode_periodic_gnss, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_dynamics,
	SHELL_CMD_ARG(general, NULL, "General purpose.", cmd_gnss_dynamics_general, 1, 0),
	SHELL_CMD_ARG(stationary, NULL, "Optimize for stationary use.",
		      cmd_gnss_dynamics_stationary, 1, 0),
	SHELL_CMD_ARG(pedestrian, NULL, "Optimize for pedestrian use.",
		      cmd_gnss_dynamics_pedestrian, 1, 0),
	SHELL_CMD_ARG(automotive, NULL, "Optimize for automotive use.",
		      cmd_gnss_dynamics_automotive, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_powersave,
	SHELL_CMD_ARG(off, NULL, "Power saving off (default).",
		      cmd_gnss_config_powersave_off, 1, 0),
	SHELL_CMD_ARG(perf, NULL, "Power saving without significant performance degradation.",
		      cmd_gnss_config_powersave_perf, 1, 0),
	SHELL_CMD_ARG(power, NULL, "Power saving with acceptable performance degradation.",
		      cmd_gnss_config_powersave_power, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_qzss_nmea,
	SHELL_CMD_ARG(standard, NULL, "Standard NMEA reporting.",
		      cmd_gnss_config_qzss_nmea_standard, 1, 0),
	SHELL_CMD_ARG(custom, NULL, "Custom NMEA reporting.",
		      cmd_gnss_config_qzss_nmea_custom, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_qzss,
	SHELL_CMD(nmea, &sub_gnss_config_qzss_nmea, "QZSS NMEA mode.", cmd_gnss_config_qzss_nmea),
	SHELL_CMD_ARG(mask, NULL,
		      "<193> <194> <195> <196> <197> <198> <199> <200> <201> <202>\n"
		      "QZSS NMEA mask for PRNs 193...202. 0 = disabled, 1 = enabled.",
		      cmd_gnss_config_qzss_mask, 11, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_timing,
	SHELL_CMD_ARG(rtc, NULL, "RTC (default).", cmd_gnss_config_timing_rtc, 1, 0),
	SHELL_CMD_ARG(tcxo, NULL, "TCXO.", cmd_gnss_config_timing_tcxo, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config,
	SHELL_CMD_ARG(system, NULL,
		      "<QZSS enabled>\n"
		      "System mask. 0 = disabled, 1 = enabled. GPS L1 C/A is always enabled.",
		      cmd_gnss_config_system, 2, 0),
	SHELL_CMD(elevation, NULL, "<angle>\nElevation threshold angle.",
		  cmd_gnss_config_elevation),
	SHELL_CMD_ARG(use_case, NULL,
		      "<low accuracy allowed> <scheduled downloads disabled>\n"
		      "Use case configuration. 0 = option disabled, 1 = option enabled "
		      "(default all disabled).",
		      cmd_gnss_config_use_case, 3, 0),
	SHELL_CMD_ARG(nmea, NULL,
		      "<GGA enabled> <GLL enabled> <GSA enabled> <GSV enabled> <RMC enabled>\n"
		      "NMEA mask. 0 = disabled, 1 = enabled (default all enabled).",
		      cmd_gnss_config_nmea, 6, 0),
	SHELL_CMD(powersave, &sub_gnss_config_powersave, "Continuous tracking power saving mode.",
		  cmd_gnss_config_powersave),
	SHELL_CMD(qzss, &sub_gnss_config_qzss, "QZSS configuration.", cmd_gnss_config_qzss),
	SHELL_CMD(timing, &sub_gnss_config_timing, "Timing source during sleep.",
		  cmd_gnss_config_timing),
	SHELL_SUBCMD_SET_END
	);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_priority,
	SHELL_CMD_ARG(enable, NULL, "Enable priority time window requests.",
		      cmd_gnss_priority_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable priority time window requests.",
		      cmd_gnss_priority_disable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_agps_automatic,
	SHELL_CMD_ARG(enable, NULL, "Enable automatic fetching of AGPS data.",
		      cmd_gnss_agps_automatic_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable automatic fetching of AGPS data (default).",
		      cmd_gnss_agps_automatic_disable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_agps,
	SHELL_CMD(automatic, &sub_gnss_agps_automatic,
		  "Enable/disable automatic fetching of AGPS data.", cmd_gnss_agps_automatic),
	SHELL_CMD_ARG(inject, NULL, "Fetch and inject AGPS data to GNSS.",
		      cmd_gnss_agps_inject, 1, 0),
	SHELL_CMD(filter, NULL,
		  "<ephe> <alm> <utc> <klob> <neq> <time> <pos> <integrity>\nSet filter for "
		  "allowed AGPS data.\n0 = disabled, 1 = enabled (default all enabled).",
		  cmd_gnss_agps_filter),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_1pps,
	SHELL_CMD_ARG(enable, NULL, "<interval (s)> <pulse length (ms)>\nEnable 1PPS.",
		      cmd_gnss_1pps_enable, 3, 0),
	SHELL_CMD_ARG(enable_at, NULL,
		      "<interval (s)> <pulse length (ms)> <dd.mm.yyyy> <hh:mm:ss>\n"
		      "Enable 1PPS at a specific time (UTC).", cmd_gnss_1pps_enable_at, 5, 0),
	SHELL_CMD(disable, NULL, "Disable 1PPS.", cmd_gnss_1pps_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss,
	SHELL_CMD_ARG(start, NULL, "Start GNSS.", cmd_gnss_start, 1, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop GNSS.", cmd_gnss_stop, 1, 0),
	SHELL_CMD(delete, &sub_gnss_delete, "Delete GNSS data.", cmd_gnss_delete),
	SHELL_CMD(mode, &sub_gnss_mode, "Set tracking mode.", cmd_gnss_mode),
	SHELL_CMD(dynamics, &sub_gnss_dynamics, "Set dynamics mode.", cmd_gnss_dynamics),
	SHELL_CMD(config, &sub_gnss_config, "Set GNSS configuration.", cmd_gnss_config),
	SHELL_CMD(priority, &sub_gnss_priority, "Enable/disable priority time window requests.",
		  cmd_gnss_priority),
	SHELL_CMD(agps, &sub_gnss_agps, "AGPS configuration and commands.", cmd_gnss_agps),
	SHELL_CMD(1pps, &sub_gnss_1pps, "1PPS control.", cmd_gnss_1pps),
	SHELL_CMD(output, NULL, "<pvt level> <nmea level> <event level>\nSet output levels.",
		  cmd_gnss_output),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(gnss, &sub_gnss, "Commands for controlling GNSS.", cmd_gnss);

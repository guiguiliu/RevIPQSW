/*
 * Copyright (c) 2014,2016-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "debug.h"
#include "nl_loop.h"
#include "cnss_qmi_client.h"
#include "wlfw_qmi_client.h"
#include "cnss_gw_update.h"
#include "cnss_user.h"
#include <linux/prctl.h>
#ifdef ANDROID
#include "cutils/misc.h"
#include <cutils/properties.h>

#include <sys/capability.h>
#include <sys/prctl.h>
#endif
#include <cnss_dp.h>

#include "cnss_genl.h"

#define DAEMONIZE	0x1
#define LOG_FILE	0x2
#define SYS_LOG		0x4
#define LOGCAT		0x8
#define VERSION_PATH    "/sys/devices/soc.0/6300000.qcom,cnss/wlan_setup"
#define CLD_2_0_MODULE  "/system/lib/modules/qcacld20.ko"
#define CLD_3_0_MODULE  "/system/lib/modules/qcacld30.ko"
#define FW_SETUP_PATH   "/sys/devices/soc.0/6300000.qcom,cnss/fw_image_setup"
#define QCA_CLD_ROME_2_0 0x20
#define QCA_CLD_ROME_3_0 0x30
#define FW_DES_MISSION_MODE 0x02

int wsvc_debug_level = MSG_DEFAULT;
pthread_mutex_t wait_for_qmi_thread;
static char *progname = NULL;
/*
 * The glibc doesn't provide a header file declaration for init_module API.
 * Hence adding it here
 */
extern int init_module(void *, unsigned long, const char *);

static void usage(void)
{
	fprintf(stderr, "\nusage: %s [options] \n"
		"   -i, --interface specify interface name like integrated, pci0, pci1 (optional)\n"
		"		    use interface \"none\" for starting daemon without connecting to any interface\n"
		"   -n, --nodaemon  do not run as a daemon\n"
		"   -d              show more debug messages (-dd for more)\n"
#ifdef CONFIG_DEBUG_FILE
		"   -f <path/file>  Log output to file \n"
#endif
#ifdef CONFIG_DEBUG_SYSLOG
		"   -s              Log output to syslog \n"
#endif
#ifdef CONFIG_DEBUG_LOGCAT
		"   -l              Log output to logcat \n"
#endif
		"       --help      display this help and exit\n"
		, progname);
	exit(EXIT_FAILURE);
}
static void wlan_service_sighandler(int signum)
{
	wsvc_printf_info("Caught Signal: %d", signum);
	nl_loop_terminate();
#ifndef CONFIG_WLAN_MSG_SVC
	pthread_mutex_unlock(&wait_for_qmi_thread);
#endif
	return;
}

static int wlan_service_setup_sighandler(void)
{
	struct sigaction sig_act;

	memset(&sig_act, 0, sizeof(sig_act));
	sig_act.sa_handler = wlan_service_sighandler;
	sigemptyset(&sig_act.sa_mask);

	sigaction(SIGQUIT, &sig_act, NULL);
	sigaction(SIGTERM, &sig_act, NULL);
	sigaction(SIGINT, &sig_act, NULL);
	sigaction(SIGHUP, &sig_act, NULL);

	return 0;
}

#ifdef CONFIG_DUAL_LOAD
static int wlan_insmod(const char *filename, const char *args)
{
	void *module;
	unsigned int size;
	int ret;

	module = load_file(filename, &size);

	if (!module) {
		wsvc_perror("Failed to load driver");
		return -1;
	}

	ret = init_module(module, size, args);
	free(module);

	return ret;
}

static int wlan_load_driver(int val)
{
	const char *WLAN_MODULE_NAME = CLD_2_0_MODULE;

	switch (val) {
	case QCA_CLD_ROME_2_0:
		WLAN_MODULE_NAME = CLD_2_0_MODULE;
		break;
	case QCA_CLD_ROME_3_0:
		WLAN_MODULE_NAME = CLD_3_0_MODULE;
		break;
	default:
		break;
	}

	if (wlan_insmod(WLAN_MODULE_NAME, "") < 0) {
		wsvc_perror("Failed to load WLAN Module");
		return -1;
	}

	return 0;
}

static int wlan_setup_fw_desc()
{
	int fd, val, ret = 0;

	fd = open(VERSION_PATH, O_RDWR);

	if (fd < 0) {
		wsvc_printf_err("Failed to Open:%s", FW_SETUP_PATH);
		return -1;
	}

	val = FW_DES_MISSION_MODE;

	if (write(fd, &val, sizeof(int)) < 0) {
		wsvc_printf_err("Failed to write:%s", FW_SETUP_PATH);
		ret = -1;
	}

	close(fd);
	return ret;
}

static int wlan_version_setup()
{
	int fd, val, len, ret = 0;

	fd = open(VERSION_PATH, O_RDONLY);

	if (fd < 0) {
		wsvc_printf_err("Failed to open:%s", VERSION_PATH);
		return -1;
	}

	len = read(fd, &val, sizeof(int));

	if (len <= 0) {
		wsvc_printf_err("Failed to read:%s\n", VERSION_PATH);
		ret = -1;
		goto end;
	}

	if (wlan_setup_fw_desc()) {
		wsvc_perror("Failed to setup Firmware Descriptor\n");
		ret = -1;
		goto end;
	}

	val = atoi((char *)&val);

	if (wlan_load_driver(val)) {
		wsvc_perror("Failed to load driver");
		ret = -1;
	}
end:
	close(fd);
	return ret;
}
#endif
#ifdef ANDROID
void configure_default_debug_level(void)
{
	wsvc_debug_level = property_get_int32("persist.vendor.cnss-daemon.debug_level",
					      wsvc_debug_level);
}
#else
void configure_default_debug_level(void)
{
}
#endif
#ifdef CONFIG_WLAN_MSG_SVC
int process_wlan_messages(void)
{
	return nl_loop_run();
}
#else
/*
*  This is meant for WIN, where wlan messages are not applicable.
*  The mutex wait here is done to hold the main thread from exiting,
*  WIN is interested only on the QMI features of this daemon.
*/
int process_wlan_messages(void)
{
	int rc = 0;

	rc = pthread_mutex_init(&wait_for_qmi_thread, NULL);
	if (rc != 0) {
		wsvc_printf_err("Failed to init mutex, ret %d", rc);
		goto out;
	}
	pthread_mutex_lock(&wait_for_qmi_thread);
	/* unlock of the above lock will be done at wlan_service_sighandler */
	pthread_mutex_lock(&wait_for_qmi_thread);
	pthread_mutex_unlock(&wait_for_qmi_thread);
	rc = pthread_mutex_destroy(&wait_for_qmi_thread);
	if (rc != 0) {
		wsvc_printf_err("Failed to destroy mutex, ret %d", rc);
		return -EINVAL;
	}
	return 0;
out:
	wsvc_printf_err("Failed to init mutex, ret %d", rc);
	return -EINVAL;
}
#endif

int main(int argc, char *argv[])
{
	int c;
	int flag = DAEMONIZE;
	uint8_t index, num_interfaces = 0;
	static struct option options[] =
	{
		{"help", no_argument, NULL, 'h'},
		{"nodaemon", no_argument, NULL, 'n'},
		{"interface", required_argument, NULL, 'i'},
		{0, 0, 0, 0}
	};
	char *log_file = NULL;
	char *interface_name = NULL;
	uint32_t instance_id = 0;
	uint8_t skip_qmi_service = 0;

	progname = argv[0];

	while (1) {
		c = getopt_long(argc, argv, "hdnf:sli:", options, NULL);

		if (c < 0)
			break;

		switch (c) {
		case 'n':
			flag &= ~DAEMONIZE;
			break;
		case 'd':
#ifdef CONFIG_DEBUG
			wsvc_debug_level++;
#else
			wsvc_printf_err("Debugging disabled, "
					"please build with -DCONFIG_DEBUG");
			exit(EXIT_FAILURE);
#endif
			break;
#ifdef CONFIG_DEBUG_FILE
		case 'f':
			log_file = optarg;
			flag |= LOG_FILE;
			break;
#endif /* CONFIG_DEBUG_FILE */
#ifdef CONFIG_DEBUG_SYSLOG
		case 's':
			flag |= SYS_LOG;
			break;
#endif /* CONFIG_DEBUG_SYSLOG */
#ifdef CONFIG_DEBUG_LOGCAT
		case 'l':
			flag |= LOGCAT;
			break;
#endif /* CONFIG_DEBUG_LOGCAT */
#ifdef ICNSS_QMI
		case 'i':
			interface_name = optarg;
			if (!strncmp(interface_name, "integrated", 10)) {
				instance_id = HAWKEYE_ID;
			} else if (!strncmp(interface_name, "pci0", 4)) {
				instance_id = PINE_1_ID;
			} else if (!strncmp(interface_name, "pci1", 4)) {
				instance_id = PINE_2_ID;
			} else if (!strncmp(interface_name, "none", 4)) {
				skip_qmi_service = 1;
				break;
			} else {
				break;
			}

			if (num_interfaces < MAX_NUM_RADIOS) {
				g_instance_id_array[num_interfaces++] =
						instance_id;
			}
			break;
#endif
		case 'h':
		default:
			usage();
			break;
		}
	}

	if (optind < argc)
		usage();

	configure_default_debug_level();
	wsvc_debug_init();

	if (flag & SYS_LOG)
		wsvc_debug_open_syslog();
	else if (flag & LOG_FILE)
		wsvc_debug_open_file(log_file);

	wlan_service_setup_sighandler();

#ifdef CONFIG_DUAL_LOAD
	if (wlan_version_setup()) {
		wsvc_printf_err("Failed to setup driver");
		exit(EXIT_FAILURE);
	}
#endif

	if (flag & DAEMONIZE && daemon(0, 0)) {
		wsvc_perror("daemon");
		exit(EXIT_FAILURE);
	}

	if (nl_loop_init()) {
		wsvc_printf_err("Failed to init nl_loop");
		goto out;
	}

	if (cnss_gw_update_init()) {
		wsvc_printf_err("Failed to init gw update loop");
		goto gw_init_fail;
	}

	if (cnss_user_socket_init()) {
		wsvc_printf_err("Failed to init user interface");
		goto register_fail;
	}

	if (0 != wlan_service_start()) {
		wsvc_printf_err("Failed to start wlan service");
		goto register_fail;
	}

	if (wlan_dp_service_start()) {
		wsvc_printf_err("Failed to start wlan datapath service");
		goto register_fail;
	}

	if (cnss_genl_init()) {
		wsvc_printf_err("Failed to init genl between daemon and platform");
		goto register_fail;
	}

	if (!num_interfaces && !skip_qmi_service)
		num_interfaces = 1;

	for (index = 0; index < num_interfaces; index++) {
		if (wlfw_start(SVC_START, index) != 0) {
			wsvc_printf_err("Failed to start wlfw service");
			goto register_fail;
		}
	}
	process_wlan_messages();

	for (index = 0; index < num_interfaces; index++)
		wlfw_stop(SVC_EXIT, index);

	cnss_genl_exit();
	wlan_dp_service_stop();
	wlan_service_stop();

register_fail:
	nl_loop_deinit();

gw_init_fail:
	cnss_gw_update_deinit();

out:
	wsvc_debug_close_syslog();
	wsvc_debug_close_file();
	exit(EXIT_SUCCESS);
}

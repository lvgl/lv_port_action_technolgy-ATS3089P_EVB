/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(log_backend_net, CONFIG_LOG_DEFAULT_LEVEL);

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_output.h>
#include <logging/log_msg.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

/* Set this to 1 if you want to see what is being sent to server */
#define DEBUG_PRINTING 0

#if DEBUG_PRINTING
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#if defined(CONFIG_NET_IPV6) || CONFIG_NET_HOSTNAME_ENABLE
#define MAX_HOSTNAME_LEN NET_IPV6_ADDR_LEN
#else
#define MAX_HOSTNAME_LEN NET_IPV4_ADDR_LEN
#endif

static char dev_hostname[MAX_HOSTNAME_LEN + 1];

static uint8_t output_buf[CONFIG_LOG_BACKEND_NET_MAX_BUF_SIZE];
static bool net_init_done;
struct sockaddr server_addr;
static bool panic_mode;

const struct log_backend *log_backend_net_get(void);

NET_PKT_SLAB_DEFINE(syslog_tx_pkts, CONFIG_LOG_BACKEND_NET_MAX_BUF);
NET_PKT_DATA_POOL_DEFINE(syslog_tx_bufs,
			 ROUND_UP(CONFIG_LOG_BACKEND_NET_MAX_BUF_SIZE /
				  CONFIG_NET_BUF_DATA_SIZE, 1) *
			 CONFIG_LOG_BACKEND_NET_MAX_BUF);

static struct k_mem_slab *get_tx_slab(void)
{
	return &syslog_tx_pkts;
}

struct net_buf_pool *get_data_pool(void)
{
	return &syslog_tx_bufs;
}

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	struct net_context *ctx = (struct net_context *)output_ctx;
	int ret = -ENOMEM;

	if (ctx == NULL) {
		return length;
	}

	ret = net_context_send(ctx, data, length, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		goto fail;
	}

	DBG(data);
fail:
	return length;
}

LOG_OUTPUT_DEFINE(log_output_net, line_out, output_buf, sizeof(output_buf));

static int do_net_init(void)
{
	struct sockaddr *local_addr = NULL;
	struct sockaddr_in6 local_addr6;
	struct sockaddr_in local_addr4;
	socklen_t server_addr_len;
	struct net_context *ctx;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4) && server_addr.sa_family == AF_INET) {
		local_addr = (struct sockaddr *)&local_addr4;
		server_addr_len = sizeof(struct sockaddr_in);
		local_addr4.sin_port = 0U;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && server_addr.sa_family == AF_INET6) {
		local_addr = (struct sockaddr *)&local_addr6;
		server_addr_len = sizeof(struct sockaddr_in6);
		local_addr6.sin6_port = 0U;
	}

	if (local_addr == NULL) {
		DBG("Server address unknown\n");
		return -EINVAL;
	}

	local_addr->sa_family = server_addr.sa_family;

	ret = net_context_get(server_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &ctx);
	if (ret < 0) {
		DBG("Cannot get context (%d)\n", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_HOSTNAME_ENABLE)) {
		(void)strncpy(dev_hostname, net_hostname_get(), MAX_HOSTNAME_LEN);

	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   server_addr.sa_family == AF_INET6) {
		const struct in6_addr *src;

		src = net_if_ipv6_select_src_addr(
			NULL, &net_sin6(&server_addr)->sin6_addr);
		if (src) {
			net_addr_ntop(AF_INET6, src, dev_hostname,
				      MAX_HOSTNAME_LEN);

			net_ipaddr_copy(&local_addr6.sin6_addr, src);
		} else {
			goto unknown;
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   server_addr.sa_family == AF_INET) {
		const struct in_addr *src;

		src = net_if_ipv4_select_src_addr(
				  NULL, &net_sin(&server_addr)->sin_addr);

		if (src) {
			net_addr_ntop(AF_INET, src, dev_hostname,
				      MAX_HOSTNAME_LEN);

			net_ipaddr_copy(&local_addr4.sin_addr, src);
		} else {
			goto unknown;
		}

	} else {
	unknown:
		DBG("Cannot setup local context\n");
		return -EINVAL;
	}

	ret = net_context_bind(ctx, local_addr, server_addr_len);
	if (ret < 0) {
		DBG("Cannot bind context (%d)\n", ret);
		return ret;
	}

	(void)net_context_connect(ctx, &server_addr, server_addr_len,
				  NULL, K_NO_WAIT, NULL);

	/* We do not care about return value for this UDP connect call that
	 * basically does nothing. Calling the connect is only useful so that
	 * we can see the syslog connection in net-shell.
	 */

	net_context_setup_pools(ctx, get_tx_slab, get_data_pool);

	log_output_ctx_set(&log_output_net, ctx);
	log_output_hostname_set(&log_output_net, dev_hostname);

	return 0;
}

static void send_output(const struct log_backend *const backend,
			struct log_msg *msg)
{
	if (panic_mode) {
		return;
	}

	if (!net_init_done && do_net_init() == 0) {
		net_init_done = true;
	}

	log_msg_get(msg);

	log_output_msg_process(&log_output_net, msg,
			       LOG_OUTPUT_FLAG_FORMAT_SYSLOG |
			       LOG_OUTPUT_FLAG_TIMESTAMP |
			(IS_ENABLED(CONFIG_LOG_BACKEND_NET_SYST_ENABLE) ?
			LOG_OUTPUT_FLAG_FORMAT_SYST : 0));

	log_msg_put(msg);
}

static void process(const struct log_backend *const backend,
		    union log_msg2_generic *msg)
{
	uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYSLOG | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (panic_mode) {
		return;
	}

	if (!net_init_done && do_net_init() == 0) {
		net_init_done = true;
	}

	log_output_msg2_process(&log_output_net, &msg->log, flags);
}

static void init_net(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
	int ret;

	net_sin(&server_addr)->sin_port = htons(514);

	ret = net_ipaddr_parse(CONFIG_LOG_BACKEND_NET_SERVER,
			       sizeof(CONFIG_LOG_BACKEND_NET_SERVER) - 1,
			       &server_addr);
	if (ret == 0) {
		LOG_ERR("Cannot configure syslog server address");
		return;
	}

	log_backend_deactivate(log_backend_net_get());
}

static void panic(struct log_backend const *const backend)
{
	panic_mode = true;
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_FORMAT_SYSLOG |
		LOG_OUTPUT_FLAG_TIMESTAMP |
		(IS_ENABLED(CONFIG_LOG_BACKEND_NET_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0);
	uint32_t key;

	if (!net_init_done && do_net_init() == 0) {
		net_init_done = true;
	}

	key = irq_lock();
	log_output_string(&log_output_net, src_level,
			  timestamp, fmt, ap, flags);
	irq_unlock(key);
}

const struct log_backend_api log_backend_net_api = {
	.panic = panic,
	.init = init_net,
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.put = IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? send_output : NULL,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ?
							sync_string : NULL,
	/* Currently we do not send hexdumps over network to remote server
	 * in CONFIG_LOG_IMMEDIATE mode. This is just to save resources,
	 * this can be revisited if needed.
	 */
	.put_sync_hexdump = NULL,
};

/* Note that the backend can be activated only after we have networking
 * subsystem ready so we must not start it immediately.
 */
LOG_BACKEND_DEFINE(log_backend_net, log_backend_net_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_NET_AUTOSTART));

const struct log_backend *log_backend_net_get(void)
{
	return &log_backend_net;
}

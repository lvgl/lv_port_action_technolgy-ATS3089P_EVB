/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POLL_H_
#define ZEPHYR_INCLUDE_POSIX_POLL_H_

#include <net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#define pollfd zsock_pollfd

#define POLLIN ZSOCK_POLLIN
#define POLLOUT ZSOCK_POLLOUT
#define POLLERR ZSOCK_POLLERR
#define POLLHUP ZSOCK_POLLHUP
#define POLLNVAL ZSOCK_POLLNVAL

static inline int poll(struct pollfd *fds, int nfds, int timeout)
{
	return zsock_poll(fds, nfds, timeout);
}

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_POLL_H_ */

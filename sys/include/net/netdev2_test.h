/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_netdev2_test    netdev2 dummy test driver
 * @ingroup     drivers_netdev
 * @brief       Provides a test dummy for the netdev2 interface.
 *
 * See the following simple packet traversal timer for an example. Note that
 * this example assumes that the stack doesn't require any option values from
 * the device and that the stack doesn't create packets on its own or looses
 * packets (neither of those can be assumed for in-production stacks).
 *
 * ~~~~~~~~~~~ {.c}
 * #include <stdint.h>
 *
 * #include "mutex.h"
 * #include "net/af.h"
 * #include "net/conn/udp.h"
 * #include "net/ipv6/addr.h"
 * #include "net/netdev2_test.h"
 * #include "xtimer.h"
 *
 * #define PKT_NUMBER   (1000)
 *
 * static netdev2_test_t dev;
 * static uint32_t last_start;
 * static uint32_t sum = 0;
 * static mutex_t wait = MUTEX_INIT;
 *
 * int _send_timer(netdev2_t *dev, const struct iovec *vector, int count)
 * {
 *     (void)dev;
 *     (void)vector;
 *     (void)count;
 *
 *     sum += (xtimer_now_usec() - last_start);
 *     mutex_unlock(&wait);
 * }
 *
 * int main(void) {
 *     ipv6_addr_t dst = IPV6_ADDR_UNSPECIFIED;
 *
 *     netdev2_test_setup(&dev, NULL);
 *     dev->driver->init((netdev2_t *)&dev)
 *     // initialize stack and connect `dev` to it
 *     // ...
 *     mutex_lock(&wait);
 *     for (int i = 0; i < PKT_NUMBER; i++) {
 *         last_start = xtimer_now_usec();
 *         conn_udp_sendto("abcd", sizeof("abcd"), NULL, 0, &dst, sizeof(dst),
 *                         AF_INET6, 0xcafe, 0xcafe);
 *         mutex_lock(&wait);
 *     }
 *     printf("Average send packet traversal time: %u\n", sum / PKT_NUMBER);
 *     mutex_unlock(&wait);
 *     return 0;
 * }
 * ~~~~~~~~~~~
 *
 * To provide options to the stack, the
 * @ref netdev2_test_t::get_cbs "get callbacks" for the specific options need
 * to be set.
 * To catch lost packets and additional sent by the stack the send handler needs
 * to be adapted accordingly.
 *
 * @{
 *
 * @file
 * @brief   @ref sys_netdev2_test definitions
 *
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 */
#ifndef NETDEV2_TEST_H
#define NETDEV2_TEST_H

#include "mutex.h"

#ifdef MODULE_NETDEV2_IEEE802154
#include "net/netdev2/ieee802154.h"
#endif

#include "net/netdev2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Callback type to handle send command
 *
 * @param[in] dev       network device descriptor
 * @param[in] vector    io vector array to send
 * @param[in] count     number of entries in vector
 *
 * @return  number of bytes sent
 * @return  <= 0 on error
 */
typedef int (*netdev2_test_send_cb_t)(netdev2_t *dev,
                                      const struct iovec *vector,
                                      int count);

/**
 * @brief   Callback type to handle receive command
 *
 * @param[in] dev       network device descriptor
 * @param[out] buf      buffer to write into or `NULL`
 * @param[in] len       maximum number of bytes to read
 * @param[out] info     status information for the received packet. Might
 *                      be of different type for different netdev2 devices.
 *                      May be NULL if not needed or applicable
 *
 * @return <=0 on error
 * @return number of bytes read if buf != NULL
 * @return currently received packet size if buf == NULL
 */
typedef int (*netdev2_test_recv_cb_t)(netdev2_t *dev, char *buf, int len,
                                      void *info);

/**
 * @brief   Callback type to handle device initialization
 *
 * @param[in] dev       network device descriptor
 *
 * @return <= on error
 * @return 0 on success
 */
typedef int (*netdev2_test_init_cb_t)(netdev2_t *dev);

/**
 * @brief   Callback type to handle user-space ISR events
 *
 * @param[in] dev       network device descriptor
 */
typedef void (*netdev2_test_isr_cb_t)(netdev2_t *dev);

/**
 * @brief   Callback type to handle get commands
 *
 * @param[in] dev       network device descriptor
 * @param[out] value    pointer to store the option's value in
 * @param[in] max_len   maximal amount of bytes that fit into @p value
 *
 * @return  number of bytes written to @p value
 * @return  <0 on error
 */
typedef int (*netdev2_test_get_cb_t)(netdev2_t *dev, void *value,
                                     size_t max_len);

/**
 * @brief   Callback type to handle set commands
 *
 * @param[in] dev       network device descriptor
 * @param[out] value    value to set
 * @param[in] value_len the length of @p value
 *
 * @return  number of bytes used from @p value
 * @return  <0 on error
 */
typedef int (*netdev2_test_set_cb_t)(netdev2_t *dev, void *value,
                                     size_t value_len);

/**
 * @brief   Device descriptor for @ref sys_netdev2_test devices
 *
 * @extends netdev2_t
 */
typedef struct {
    /**
     * @brief   netdev2 fields
     * @{
     */
#ifdef  MODULE_NETDEV2_IEEE802154
    netdev2_ieee802154_t netdev;    /**< superclass */
#else                               /* MODULE_NETDEV2_IEEE802154 */
    netdev2_t netdev;               /**< superclass */
#endif  /* MODULE_NETDEV2_IEEE802154 */
    /** @} */

    /**
     * @brief   device specific fields
     * @{
     */
    netdev2_test_send_cb_t send_cb;                 /**< callback to handle send command */
    netdev2_test_recv_cb_t recv_cb;                 /**< callback to handle receive command */
    netdev2_test_init_cb_t init_cb;                 /**< callback to handle initialization events */
    netdev2_test_isr_cb_t isr_cb;                   /**< callback to handle ISR events */
    netdev2_test_get_cb_t get_cbs[NETOPT_NUMOF];    /**< callback to handle get command */
    netdev2_test_set_cb_t set_cbs[NETOPT_NUMOF];    /**< callback to handle set command */
    void *state;                                    /**< external state for the device */
    mutex_t mutex;                                  /**< mutex for the device */
    /** @} */
} netdev2_test_t;

/**
 * @brief   override send callback
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] send_cb   a send callback
 */
static inline void netdev2_test_set_send_cb(netdev2_test_t *dev,
                                            netdev2_test_send_cb_t send_cb)
{
    mutex_lock(&dev->mutex);
    dev->send_cb = send_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   override receive callback
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] recv_cb   a receive callback
 */
static inline void netdev2_test_set_recv_cb(netdev2_test_t *dev,
                                            netdev2_test_recv_cb_t recv_cb)
{
    mutex_lock(&dev->mutex);
    dev->recv_cb = recv_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   override initialization callback
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] init_cb   an initialization callback
 */
static inline void netdev2_test_set_init_cb(netdev2_test_t *dev,
                                            netdev2_test_init_cb_t init_cb)
{
    mutex_lock(&dev->mutex);
    dev->init_cb = init_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   override ISR event handler callback
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] isr_cb    an ISR event handler callback
 */
static inline void netdev2_test_set_isr_cb(netdev2_test_t *dev,
                                           netdev2_test_isr_cb_t isr_cb)
{
    mutex_lock(&dev->mutex);
    dev->isr_cb = isr_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   override get callback for a certain option type
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] opt       an option type
 * @param[in] get_cb    a get callback for @p opt
 */
static inline void netdev2_test_set_get_cb(netdev2_test_t *dev, netopt_t opt,
                                           netdev2_test_get_cb_t get_cb)
{
    mutex_lock(&dev->mutex);
    dev->get_cbs[opt] = get_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   override get callback for a certain option type
 *
 * @param[in] dev       a @ref sys_netdev2_test device
 * @param[in] opt       an option type
 * @param[in] set_cb    a set callback for @p opt
 */
static inline void netdev2_test_set_set_cb(netdev2_test_t *dev, netopt_t opt,
                                           netdev2_test_set_cb_t set_cb)
{
    mutex_lock(&dev->mutex);
    dev->set_cbs[opt] = set_cb;
    mutex_unlock(&dev->mutex);
}

/**
 * @brief   Setup a given @ref sys_netdev2_test device
 *
 * @param[in] dev   a @ref sys_netdev2_test device to initialize
 * @param[in] state external state for the device
 */
void netdev2_test_setup(netdev2_test_t *dev, void *state);

/**
 * @brief   Resets all callbacks for the device to NULL
 *
 * @param[in] dev   a @ref sys_netdev2_test device to initialize
 */
void netdev2_test_reset(netdev2_test_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* NETDEV2_TEST_H */
/** @} */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_THREAD_IZEM_H_INCLUDED
#define MPL_THREAD_IZEM_H_INCLUDED

#include <lock/zm_lock.h>
#include <cond/zm_cond.h>

typedef zm_lock_t MPL_thread_mutex_t;
typedef zm_cond_t MPL_thread_cond_t;

/*
 *    Mutexes
 */

#define MPL_thread_mutex_create(mutex_ptr_, err_ptr_)                   \
    do {                                                                \
        int err__;                                                      \
        err__ = zm_lock_init(mutex_ptr_);                               \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_lock_init", err__,        \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_mutex_destroy(mutex_ptr_, err_ptr_)                  \
    do {                                                                \
        err__ = zm_lock_destroy(mutex_ptr_);                            \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_lock_destroy", err__,     \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_mutex_lock(mutex_ptr_, err_ptr_)                     \
    do {                                                                \
        int err__;                                                      \
        err__ = zm_lock_acquire(mutex_ptr_);                            \
        if (unlikely(err__)) {                                          \
            MPL_internal_sys_error_printf("zm_lock_acquire", err__,     \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        }                                                               \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_mutex_unlock(mutex_ptr_, err_ptr_)                   \
    do {                                                                \
        int err__;                                                      \
        err__ = zm_lock_release(mutex_ptr_);                            \
        if (unlikely(err__)) {                                          \
            MPL_internal_sys_error_printf("zm_lock_release", err__,     \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        }                                                               \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

/*
 * Condition Variables
 */

#define MPL_thread_cond_create(cond_ptr_, err_ptr_)                     \
    do {                                                                \
        int err__;							                            \
        err__ = zm_cond_init((cond_ptr_), NULL);                        \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_cond_init", err__,        \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_cond_destroy(cond_ptr_, err_ptr_)                    \
    do {                                                                \
        int err__;							                            \
        err__ = zm_cond_destroy(cond_ptr_);                             \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_cond_destroy", err__,     \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)		    \
    do {                                                                \
        int err__;                                                      \
        err__ = zm_cond_wait((cond_ptr_), mutex_ptr_);                  \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_cond_wait", err__,        \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_cond_broadcast(cond_ptr_, err_ptr_)                  \
    do {                                                                \
        int err__;							                            \
        err__ = zm_cond_bcast(cond_ptr_);			                    \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("zm_cond_broadcast", err__,   \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPL_thread_cond_signal(cond_ptr_, err_ptr_)                     \
    do {                                                                \
        int err__;							                            \
        err__ = zm_cond_signal(cond_ptr_);                              \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_signal", err__, \
                                    "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#endif /* MPL_THREAD_IZEM_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_PROGRESS_H_INCLUDED
#define CH4_PROGRESS_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_test(void)
{
    int mpi_errno = MPI_SUCCESS, made_progress, i, countdown, backoff, err;
    MPIR_Per_thread_t *per_thread = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_TEST);

    if (OPA_load_int(&MPIDI_CH4_Global.active_progress_hooks)) {
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
        for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
            if (MPIDI_CH4_Global.progress_hooks[i].active == TRUE) {
                MPIR_Assert(MPIDI_CH4_Global.progress_hooks[i].func_ptr != NULL);
                mpi_errno = MPIDI_CH4_Global.progress_hooks[i].func_ptr(&made_progress);
                if (mpi_errno) {
                    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
    }

    MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                         MPIR_Per_thread, per_thread, &err);

    MPIR_Assert(err == 0);

    countdown = per_thread->countdown;
    backoff = per_thread->cur_backoff;

    /* todo: progress unexp_list */
    for (i = 0; i < MPIDI_CH4_Global.n_netmod_eps; i++) {
        MPIDI_DISPATCH_PROGRESS(TEST, i, (&countdown), (&backoff), mpi_errno);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    per_thread->countdown = countdown;
    per_thread->cur_backoff = backoff;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_SHM_progress(0);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

    MPID_THREAD_CS_EXIT(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_ENTER(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_test_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_test_req(MPIR_Request *req, int *countdown, int *backoff)
{
    int mpi_errno;
    mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST_REQ);

    MPIDI_DISPATCH_PROGRESS(TEST, 0, countdown, backoff, mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_SHM_progress(0);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST_REQ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_poke(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_POKE);

    ret = MPID_Progress_test();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_START);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_START);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_end(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_END);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_END);
    return;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_wait(MPID_Progress_state * state)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);

    ret = MPID_Progress_test();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);
    return ret;
}

#define MPIR_CVAR_CH4_GLOBAL_PROGRESS_PATIENCE (1 << 10)

#undef FUNCNAME
#define FUNCNAME MPID_Progress_wait_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_wait_req(MPID_Progress_state * state, MPIR_Request *req)
{
    int ret, global_progress_patience, countdown = 0, backoff = 1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);

    global_progress_patience = MPIR_CVAR_CH4_GLOBAL_PROGRESS_PATIENCE;
    do {
        /* local progress */
        ret = MPID_Progress_test_req(req, &countdown, &backoff);
        if (unlikely(ret))
            MPIR_ERR_POP(ret);
        if (MPIR_Request_is_complete(req))
            break;

        if (unlikely(--global_progress_patience <= 0)) {
            /* global progress */
            ret = MPID_Progress_test();
            if (unlikely(ret))
                MPIR_ERR_POP(ret);
            global_progress_patience = MPIR_CVAR_CH4_GLOBAL_PROGRESS_PATIENCE;
        }

        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    } while (1);


    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);

  fn_exit:
    return ret;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_reset()
{
    int err;
    MPIR_Per_thread_t *per_thread = NULL;

    MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                 MPIR_Per_thread, per_thread, &err);
    MPIR_Assert(err == 0);
    if(unlikely(per_thread->cur_backoff != 1)) {
        per_thread->countdown = 0;
        per_thread->cur_backoff = 1;
    }

    return err;
}


#undef FUNCNAME
#define FUNCNAME MPID_Progress_register
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_register(int (*progress_fn) (int *), int *id)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_REGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_REGISTER);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (MPIDI_CH4_Global.progress_hooks[i].func_ptr == NULL) {
            MPIDI_CH4_Global.progress_hooks[i].func_ptr = progress_fn;
            MPIDI_CH4_Global.progress_hooks[i].active = FALSE;
            break;
        }
    }

    if (i >= MAX_PROGRESS_HOOKS)
        goto fn_fail;

    OPA_incr_int(&MPIDI_CH4_Global.active_progress_hooks);

    (*id) = i;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_REGISTER);
    return mpi_errno;
  fn_fail:
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                     "MPID_Progress_register", __LINE__,
                                     MPI_ERR_INTERN, "**progresshookstoomany", 0);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_deregister
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_deregister(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEREGISTER);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].func_ptr = NULL;
    MPIDI_CH4_Global.progress_hooks[id].active = FALSE;

    OPA_decr_int(&MPIDI_CH4_Global.active_progress_hooks);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_activate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_activate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_ACTIVATE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].active == FALSE);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].active = TRUE;

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_deactivate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_deactivate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEACTIVATE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].active == TRUE);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].active = FALSE;

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    return mpi_errno;
}

#if defined(MPICH_THREAD_USE_MDTA) && !defined(MPIDI_CH4_MT_HANDOFF)

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[])
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_Thread_sync_t *sync = NULL;


    MPIR_Thread_sync_alloc(&sync, count);

    MPID_THREAD_CS_ENTER(EP, MPIDI_CH4_Global.ep_locks[0]);

    /* Fix up number of pending requests and attach the sync. */
    for (i = 0; i < count; i++) {
        if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
            MPIR_Thread_sync_signal(sync, 0);
        } else {
            MPIR_Request_attach_sync(request_ptrs[i], sync);
        }
    }

    /* Wait on the synchronization object. */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__EP
    MPIR_Thread_sync_wait(sync, &MPIDI_CH4_Global.ep_locks[0]);
#else
    MPIR_Thread_sync_wait(sync, &MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#endif

    MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[0]);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Thread_sync_t *sync = NULL;


    MPIR_Thread_sync_alloc(&sync, 1);

    MPID_THREAD_CS_ENTER(EP, MPIDI_CH4_Global.ep_locks[0]);

    if (request_ptr == NULL || MPIR_Request_is_complete(request_ptr)) {
        MPIR_Thread_sync_signal(sync, 0);
    } else {
        MPIR_Request_attach_sync(request_ptr, sync);
    }

    /* Wait on the synchronization object. */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__EP
    MPIR_Thread_sync_wait(sync, &MPIDI_CH4_Global.ep_locks[0]);
#else
    MPIR_Thread_sync_wait(sync, &MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#endif

    MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[0]);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Wait_done()
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Thread_sync_t *sync = NULL;

    MPID_THREAD_CS_ENTER(EP, MPIDI_CH4_Global.ep_locks[0]);

    MPIR_Thread_sync_get(&sync);

    MPIR_Thread_sync_free(sync);

    MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[0]);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else

#define MPID_Waitall(count, request_ptrs)
#define MPID_Wait(request_ptr)
#define MPID_Wait_done()

#endif


#endif /* CH4_PROGRESS_H_INCLUDED */

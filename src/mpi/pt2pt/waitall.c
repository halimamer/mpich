/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPIR_REQUEST_PTR_ARRAY_SIZE)
/* use a larger default size of 64 in order to enhance SQMR performance */
#define MPIR_REQUEST_PTR_ARRAY_SIZE 64
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Waitall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitall = PMPI_Waitall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitall  MPI_Waitall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitall as PMPI_Waitall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) __attribute__((weak,alias("PMPI_Waitall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitall
#define MPI_Waitall PMPI_Waitall


/* The "fastpath" version of MPIR_Request_complete.  It only handles
 * MPIR_REQUEST_KIND__SEND and MPIR_REQUEST_KIND__RECV kinds, and it does not attempt to
 * deal with status structures under the assumption that bleeding fast code will
 * pass either MPI_STATUS_IGNORE or MPI_STATUSES_IGNORE as appropriate.  This
 * routine (or some a variation of it) is an unfortunately necessary stunt to
 * get high message rates on key benchmarks for high-end systems.
 */
#undef FUNCNAME
#define FUNCNAME request_complete_fastpath
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int request_complete_fastpath(MPI_Request *request, MPIR_Request *request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(request_ptr->kind == MPIR_REQUEST_KIND__SEND || request_ptr->kind == MPIR_REQUEST_KIND__RECV);

    if (request_ptr->kind == MPIR_REQUEST_KIND__SEND) {
        /* FIXME: are Ibsend requests added to the send queue? */
        MPII_SENDQ_FORGET(request_ptr);
    }

    /* the completion path for SEND and RECV is the same at this time, modulo
     * the SENDQ hook above */
    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_free(request_ptr);
    *request = MPI_REQUEST_NULL;

    /* avoid normal fn_exit/fn_fail jump pattern to reduce jumps and compiler confusion */
    return mpi_errno;
}

#ifdef MPICH_THREAD_USE_MDTA

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_Thread_sync_t *sync = NULL;


    MPIR_Thread_sync_alloc(&sync, count);

    /* Fix up number of pending requests and attach the sync. */
    for (i = 0; i < count; i++) {
        if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
            MPIR_Thread_sync_signal(sync, 0);
        } else {
            MPIR_Request_attach_sync(request_ptrs[i], sync);
        }
    }

    /* Wait on the synchronization object. */
    MPIR_Thread_sync_wait(sync);

    MPIR_Thread_sync_free(sync);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else

#define MPID_Waitall(count, request_ptrs, array_of_statuses)

#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Waitall_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Waitall_impl(int count, MPI_Request array_of_requests[],
                      MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request * request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i, j;
    int n_completed;
    int active_flag;
    int rc;
    int n_greqs;
    int proc_failure = FALSE;
    int disabled_anysource = FALSE;
    const int ignoring_statuses = (array_of_statuses == MPI_STATUSES_IGNORE);
    int optimize = ignoring_statuses; /* see NOTE-O1 */
    MPIR_CHKLMEM_DECL(1);

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIR_CHKLMEM_MALLOC(request_ptrs, MPIR_Request **, count * sizeof(MPIR_Request *), mpi_errno, "request pointers");
    }

    n_greqs = 0;
    n_completed = 0;
    for (i = 0; i < count; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPIR_Request_valid_ptr( request_ptrs[i], mpi_errno );
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_ERR_CHKANDJUMP1((request_ptrs[i]->kind == MPIR_REQUEST_KIND__MPROBE),
                                         mpi_errno, MPI_ERR_ARG, "**msgnotreq", "**msgnotreq %d", i);
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif
            if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND)
            {
                optimize = FALSE;
            }

            if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST)
                ++n_greqs;

            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptrs[i]) &&
                        !MPIR_Request_is_complete(request_ptrs[i]) &&
                        !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                disabled_anysource = TRUE;
            }
	}
	else
	{
	    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
	    MPIR_Status_set_empty(status_ptr);
	    request_ptrs[i] = NULL;
	    n_completed += 1;
            optimize = FALSE;
	}
    }
    
    if (n_completed == count)
    {
	goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno = MPIR_Testall_impl(count, array_of_requests, &disabled_anysource, array_of_statuses);
        goto fn_exit;
    }

    /* NOTE-O1: high-message-rate optimization.  For simple send and recv
     * operations and MPI_STATUSES_IGNORE we use a fastpath approach that strips
     * out as many unnecessary jumps and error handling as possible.
     *
     * Possible variation: permit request_ptrs[i]==NULL at the cost of an
     * additional branch inside the for-loop below. */
    if (optimize) {
        MPID_Waitall(count, request_ptrs, array_of_statuses);
        MPID_Progress_start(&progress_state);
        for (i = 0; i < count; ++i) {
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* must check and handle the error, can't guard with HAVE_ERROR_CHECKING, but it's
                 * OK for the error case to be slower */
                if (unlikely(mpi_errno)) {
                    /* --BEGIN ERROR HANDLING-- */
                    if (unlikely(MPIR_CVAR_ENABLE_FT &&
                                MPID_Request_is_anysource(request_ptrs[i]) &&
                                !MPIR_Request_is_complete(request_ptrs[i]) &&
                                !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                        MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");
                    }
                    MPID_Progress_end(&progress_state);
                    MPIR_ERR_POP(mpi_errno);
                    /* --END ERROR HANDLING-- */
                }
            }
            mpi_errno = request_complete_fastpath(&array_of_requests[i], request_ptrs[i]);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_Progress_reset();
        }

        MPID_Progress_end(&progress_state);

        goto fn_exit;
    }

    /* ------ "slow" code path below ------ */

    /* Grequest_waitall may run the progress engine - thus, we don't 
       invoke progress_start until after running Grequest_waitall */
    /* first, complete any generalized requests */
    if (n_greqs)
    {
        mpi_errno = MPIR_Grequest_waitall(count, request_ptrs);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
    MPID_Progress_start(&progress_state);

    for (i = 0; i < count; i++)
    {
        if (request_ptrs[i] == NULL)
        {
            if (!ignoring_statuses)
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            continue;
        }
        
        /* wait for ith request to complete */
        while (!MPIR_Request_is_complete(request_ptrs[i]))
        {
            /* generalized requests should already be finished */
            MPIR_Assert(request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST);
            
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                MPIR_ERR_POP(mpi_errno);
                /* --END ERROR HANDLING-- */
            } else if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptrs[i]) &&
                        !MPIR_Request_is_complete(request_ptrs[i]) &&
                        !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                /* Check for pending failures */
                MPID_Progress_end(&progress_state);
                MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                status_ptr = (ignoring_statuses) ? MPI_STATUS_IGNORE : &array_of_statuses[i];
                if (status_ptr != MPI_STATUS_IGNORE) status_ptr->MPI_ERROR = mpi_errno;
                proc_failure = TRUE;
                break;
            }
        }

        if (MPIR_Request_is_complete(request_ptrs[i])) {
            /* complete the request and check the status */
            status_ptr = (ignoring_statuses) ? MPI_STATUS_IGNORE : &array_of_statuses[i];
            rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
        }

        if (rc == MPI_SUCCESS)
        {
            request_ptrs[i] = NULL;
            if (!ignoring_statuses)
                status_ptr->MPI_ERROR = MPI_SUCCESS;
        }
        else
        {
            /* req completed with an error */
            MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");

            if (!proc_failure) {
                if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(rc))
                    proc_failure = TRUE;
            }

            if (!ignoring_statuses)
            {
                /* set the error code for this request */
                status_ptr->MPI_ERROR = rc;

                /* set the error codes for the rest of the uncompleted requests to PENDING */
                for (j = i+1; j < count; ++j)
                {
                    if (!ignoring_statuses)
                    {
                        if (request_ptrs[j] == NULL)
                        {
                            /* either the user specified MPI_REQUEST_NULL, or this is a completed greq */
                            array_of_statuses[j].MPI_ERROR = MPI_SUCCESS;
                        }
                        else
                        {
                            if (!proc_failure)
                                array_of_statuses[j].MPI_ERROR = MPI_ERR_PENDING;
                            else
                                array_of_statuses[j].MPI_ERROR = MPIX_ERR_PROC_FAILED_PENDING;
                        }
                    }
                }
            }
            break;
        }
    }
    MPID_Progress_end(&progress_state);
        
 fn_exit:
     if (count > MPIR_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIR_CHKLMEM_FREEALL();
    }

   return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Waitall_nopoll
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Waitall_nopoll(int count, MPI_Request array_of_requests[],
                      MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request * request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i, j;
    int n_completed;
    int active_flag;
    int rc;
    int n_greqs;
    int proc_failure = FALSE;
    int disabled_anysource = FALSE;
    const int ignoring_statuses = (array_of_statuses == MPI_STATUSES_IGNORE);
    int optimize = ignoring_statuses; /* see NOTE-O1 */
    MPIR_CHKLMEM_DECL(1);

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIR_CHKLMEM_MALLOC(request_ptrs, MPIR_Request **, count * sizeof(MPIR_Request *), mpi_errno, "request pointers");
    }

    n_greqs = 0;
    n_completed = 0;
    for (i = 0; i < count; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPIR_Request_valid_ptr( request_ptrs[i], mpi_errno );
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_ERR_CHKANDJUMP1((request_ptrs[i]->kind == MPIR_REQUEST_KIND__MPROBE),
                                         mpi_errno, MPI_ERR_ARG, "**msgnotreq", "**msgnotreq %d", i);
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif
            if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND)
            {
                optimize = FALSE;
            }

            if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST)
                ++n_greqs;

            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptrs[i]) &&
                        !MPIR_Request_is_complete(request_ptrs[i]) &&
                        !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                disabled_anysource = TRUE;
            }
	}
	else
	{
	    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
	    MPIR_Status_set_empty(status_ptr);
	    request_ptrs[i] = NULL;
	    n_completed += 1;
            optimize = FALSE;
	}
    }
    
    if (n_completed == count)
    {
	goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno = MPIR_Testall_impl(count, array_of_requests, &disabled_anysource, array_of_statuses);
        goto fn_exit;
    }

    /* NOTE-O1: high-message-rate optimization.  For simple send and recv
     * operations and MPI_STATUSES_IGNORE we use a fastpath approach that strips
     * out as many unnecessary jumps and error handling as possible.
     *
     * Possible variation: permit request_ptrs[i]==NULL at the cost of an
     * additional branch inside the for-loop below. */
    if (optimize) {
        for (i = 0; i < count; ++i) {
            while (!MPIR_Request_is_complete(request_ptrs[i]))
                ; /* busy wait */
            mpi_errno = request_complete_fastpath(&array_of_requests[i], request_ptrs[i]);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        goto fn_exit;
    }

    /* ------ "slow" code path below ------ */

    /* Grequest_waitall may run the progress engine - thus, we don't 
       invoke progress_start until after running Grequest_waitall */
    /* first, complete any generalized requests */
    if (n_greqs)
    {
        mpi_errno = MPIR_Grequest_waitall(count, request_ptrs);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    for (i = 0; i < count; i++)
    {
        if (request_ptrs[i] == NULL)
        {
            if (!ignoring_statuses)
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            continue;
        }
        /* wait for ith request to complete */
        while (!MPIR_Request_is_complete(request_ptrs[i]))
        {
            /* generalized requests should already be finished */
            MPIR_Assert(request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST);
            /* busy wait */
        }

        if (MPIR_Request_is_complete(request_ptrs[i])) {
            /* complete the request and check the status */
            status_ptr = (ignoring_statuses) ? MPI_STATUS_IGNORE : &array_of_statuses[i];
            rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
        }

        if (rc == MPI_SUCCESS)
        {
            request_ptrs[i] = NULL;
            if (!ignoring_statuses)
                status_ptr->MPI_ERROR = MPI_SUCCESS;
        }
        else
        {
            /* req completed with an error */
            MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");

            if (!proc_failure) {
                if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(rc))
                    proc_failure = TRUE;
            }

            if (!ignoring_statuses)
            {
                /* set the error code for this request */
                status_ptr->MPI_ERROR = rc;

                /* set the error codes for the rest of the uncompleted requests to PENDING */
                for (j = i+1; j < count; ++j)
                {
                    if (!ignoring_statuses)
                    {
                        if (request_ptrs[j] == NULL)
                        {
                            /* either the user specified MPI_REQUEST_NULL, or this is a completed greq */
                            array_of_statuses[j].MPI_ERROR = MPI_SUCCESS;
                        }
                        else
                        {
                            if (!proc_failure)
                                array_of_statuses[j].MPI_ERROR = MPI_ERR_PENDING;
                            else
                                array_of_statuses[j].MPI_ERROR = MPIX_ERR_PROC_FAILED_PENDING;
                        }
                    }
                }
            }
            break;
        }
    }

fn_exit:
     if (count > MPIR_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIR_CHKLMEM_FREEALL();
    }

   return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Waitall - Waits for all given MPI Requests to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of request handles (array of handles)

Output Parameters:
. array_of_statuses - array of status objects (array of Statuses).  May be
  'MPI_STATUSES_IGNORE'.

Notes:

If one or more of the requests completes with an error, 'MPI_ERR_IN_STATUS' is
returned.  An error value will be present is elements of 'array_of_status'
associated with the requests.  Likewise, the 'MPI_ERROR' field in the status
elements associated with requests that have successfully completed will be
'MPI_SUCCESS'.  Finally, those requests that have not completed will have a 
value of 'MPI_ERR_PENDING'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WAITALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPI_WAITALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int i;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);

	    if (count != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
	    
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
	    }

	    for (i = 0; i < count; i++) {
		MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */

#if !defined(MPIDI_CH4_MT_HANDOFF)
    mpi_errno = MPIR_Waitall_impl(count, array_of_requests, array_of_statuses);
#else
    mpi_errno = MPIR_Waitall_nopoll(count, array_of_requests, array_of_statuses);
#endif
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
 fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_WAITALL);
    MPID_THREAD_CS_EXIT(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_waitall",
				     "**mpi_waitall %d %p %p",
				     count, array_of_requests,
				     array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

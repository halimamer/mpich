/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Isend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Isend = PMPI_Isend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Isend  MPI_Isend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Isend as PMPI_Isend
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Isend(const void *buf, int count, int dest, int tag) __attribute__((weak,alias("PMPI_Isend")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Isend
#define MPI_Isend PMPI_Isend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Isend - Begins a nonblocking send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Output Parameters:
. request - communication request (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_EXHAUSTED

@*/
int MPI_Isend(const void *buf, int count, int dest, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ISEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(MPID_STATE_MPI_ISEND);

    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);

        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Isend_min(buf, count, dest, tag, MPIR_CONTEXT_INTRA_PT2PT);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* return the handle of the request to the user */
    /* MPIU_OBJ_HANDLE_PUBLISH is unnecessary for isend, lower-level access is
     * responsible for its own consistency, while upper-level field access is
     * controlled by the completion counter */

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_ISEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_isend",
	    "**mpi_isend %p %d %i %t", buf, count, dest, tag);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL /* we should not arrive here */, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

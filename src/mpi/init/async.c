/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"

#ifndef MPICH_MPI_FROM_PMPI

#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
static MPIR_Comm *progress_comm_ptr;
static MPID_Thread_id_t progress_thread_id;
static pthread_mutex_t progress_mutex;
static pthread_cond_t progress_cond;
static volatile int progress_thread_done = 0;

#if defined(__linux__)
//#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
//#endif
#include <pthread.h>
#include <sched.h>
static int verbose = 0;


static void set_odd_cpuset(void) __attribute__((unused));
static void set_even_cpuset(void) __attribute__((unused));

static int core_count(void){
    return (int) sysconf(_SC_NPROCESSORS_ONLN);
}

static void print_cpuset(const char* prefix) {
    int i, err;
    char s[100] = "";
    cpu_set_t my_cpuset;
    err = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
    assert(err==0);
    for(i=0; i<core_count(); i++) sprintf(s, "%s%d", s, CPU_ISSET(i, &my_cpuset));
    printf("%s %s\n", prefix, s); fflush(stdout);
}

static void set_tozero_cpuset(){
   int i, err;
   int num_cpus = 0;

   cpu_set_t my_cpuset;
   err = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
   print_cpuset("Initial async thread cpu set: ");

   for(i=0; i<core_count(); i++) {
       if (CPU_ISSET(i, &my_cpuset))
           num_cpus += 1;
       if (num_cpus > 1)
           CPU_CLR(i, &my_cpuset);
   }

   assert(num_cpus!=1);
   err = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
   assert(err==0);
   print_cpuset("New async thread cpu set:");
}

static void set_odd_cpuset() {
    int i, err;

    cpu_set_t my_cpuset;
    err = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
    if(verbose) print_cpuset("Initial async thread cpu set: ");

    for(i=0; i<core_count(); i+=2) CPU_CLR(i, &my_cpuset);

    err = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
    assert(err==0);
    if(verbose) print_cpuset("New async thread cpu set:");
}
static void set_even_cpuset() {
    int i, err;

    cpu_set_t my_cpuset;
    err = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
    if(verbose) print_cpuset("Initial main thread cpu set: ");

    for(i=1; i<core_count(); i+=2) CPU_CLR(i, &my_cpuset);

    err = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_cpuset);
    assert(err==0);
    if(verbose) print_cpuset("New main thread cpu set:");
}
#else
#define set_tozero_cpuset()
#define set_odd_cpuset()
#define set_even_cpuset()
#endif

/* We can use whatever tag we want; we use a different communicator
 * for communicating with the progress thread. */
#define WAKE_TAG 100

#undef FUNCNAME
#define FUNCNAME progress_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void progress_fn(void * data)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr = NULL;
    MPI_Request request;
    MPI_Status status;

    set_tozero_cpuset();

    /* Explicitly add CS_ENTER/EXIT since this thread is created from
     * within an internal function and will call NMPI functions
     * directly. */
    MPID_THREAD_CS_ENTER(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* FIXME: We assume that waiting on some request forces progress
     * on all requests. With fine-grained threads, will this still
     * work as expected? We can imagine an approach where a request on
     * a non-conflicting communicator would not touch the remaining
     * requests to avoid locking issues. Once the fine-grained threads
     * code is fully functional, we need to revisit this and, if
     * appropriate, either change what we do in this thread, or delete
     * this comment. */

    mpi_errno = MPID_Irecv(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait_impl(&request, &status);
    MPIR_Assert(!mpi_errno);

    /* Send a signal to the main thread saying we are done */
    mpi_errno = pthread_mutex_lock(&progress_mutex);
    MPIR_Assert(!mpi_errno);

    progress_thread_done = 1;

    mpi_errno = pthread_mutex_unlock(&progress_mutex);
    MPIR_Assert(!mpi_errno);

    mpi_errno = pthread_cond_signal(&progress_cond);
    MPIR_Assert(!mpi_errno);

    MPID_THREAD_CS_EXIT(EP_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return;
}

#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */

#undef FUNCNAME
#define FUNCNAME MPIR_Init_async_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Init_async_thread(void)
{
#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_self_ptr;
    int err = 0;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_INIT_ASYNC_THREAD);


    /* Dup comm world for the progress thread */
    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, &progress_comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    err =pthread_cond_init(&progress_cond, NULL);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**cond_create", "**cond_create %s", strerror(err));
    
    err = pthread_mutex_init(&progress_mutex, NULL);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**mutex_create", "**mutex_create %s", strerror(err));
    
    MPID_Thread_create((MPID_Thread_func_t) progress_fn, NULL, &progress_thread_id, &err);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**mutex_create", "**mutex_create %s", strerror(err));

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
#else
    return MPI_SUCCESS;
#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Finalize_async_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Finalize_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS;
#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
    MPIR_Request *request_ptr = NULL;
    MPI_Request request;
    MPI_Status status;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    mpi_errno = MPID_Isend(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait_impl(&request, &status);
    MPIR_Assert(!mpi_errno);

    /* XXX DJG why is this unlock/lock necessary?  Should we just YIELD here or later?  */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    mpi_errno = pthread_mutex_lock(&progress_mutex);
    MPIR_Assert(!mpi_errno);

    while (!progress_thread_done) {
        mpi_errno = pthread_cond_wait(&progress_cond, &progress_mutex);
        MPIR_Assert(!mpi_errno);
    }

    mpi_errno = pthread_mutex_unlock(&progress_mutex);
    MPIR_Assert(!mpi_errno);

    mpi_errno = MPIR_Comm_free_impl(progress_comm_ptr);
    MPIR_Assert(!mpi_errno);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    mpi_errno = pthread_cond_destroy(&progress_cond);
    MPIR_Assert(!mpi_errno);

    mpi_errno = pthread_mutex_destroy(&progress_mutex);
    MPIR_Assert(!mpi_errno);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */
    return mpi_errno;
}

#endif

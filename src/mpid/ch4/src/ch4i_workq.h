/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

#include "ch4i_workq_types.h"

/* For profiling */
extern double MPIDI_pt2pt_enqueue_time;
extern double MPIDI_pt2pt_progress_time;
extern double MPIDI_pt2pt_issue_pend_time;
extern double MPIDI_rma_enqueue_time;
extern double MPIDI_rma_progress_time;
extern double MPIDI_rma_issue_pend_time;
extern unsigned long MPIDI_nqueue_traversd;
extern unsigned long MPIDI_nqueue_nonempty;

static inline double get_wtime() {
    double d;
    MPID_Time_t t;
    MPID_Wtime(&t);
    MPID_Wtime_todouble(&t, &d);
    return d;
}

#if defined(MPIDI_WORKQ_PROFILE)
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START  double enqueue_t1 = get_wtime();
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP                          \
    do {                                                        \
        double enqueue_t2 = get_wtime();                        \
        MPIDI_pt2pt_enqueue_time += (enqueue_t2 - enqueue_t1);  \
    }while(0)
#define MPIDI_WORKQ_PROGRESS_START double progress_t1 = get_wtime();
#define MPIDI_WORKQ_PROGRESS_STOP                                 \
    do {                                                                \
        double progress_t2 = get_wtime();                               \
        MPIDI_pt2pt_progress_time += (progress_t2 - progress_t1);       \
    }while(0)
#define MPIDI_WORKQ_QUEUE_TRAVERSAL(workq_elemt)                        \
    do {                                                                \
        MPIDI_nqueue_traversd++;                                        \
        if (workq_elemt != NULL)  MPIDI_nqueue_nonempty++;              \
    }while(0)
#define MPIDI_WORKQ_ISSUE_START    double issue_t1 = get_wtime();
#define MPIDI_WORKQ_ISSUE_STOP                            \
    do {                                                        \
        double issue_t2 = get_wtime();                          \
        MPIDI_pt2pt_issue_pend_time += (issue_t2 - issue_t1);   \
    }while(0)

#define MPIDI_WORKQ_RMA_ENQUEUE_START    double enqueue_t1 = get_wtime();
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP                            \
    do {                                                        \
        double enqueue_t2 = get_wtime();                        \
        MPIDI_rma_enqueue_time += (enqueue_t2 - enqueue_t1);    \
    }while(0)
#else /*!defined(MPIDI_WORKQ_PROFILE)*/
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP
#define MPIDI_WORKQ_PROGRESS_START
#define MPIDI_WORKQ_PROGRESS_STOP
#define MPIDI_WORKQ_QUEUE_TRAVERSAL(workq_elemt)
#define MPIDI_WORKQ_ISSUE_START
#define MPIDI_WORKQ_ISSUE_STOP

#define MPIDI_WORKQ_RMA_ENQUEUE_START
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP
#endif

static inline void MPIDI_workq_pt2pt_enqueue_body(MPIDI_workq_op_t op,
                                                  const void *send_buf,
                                                  void *recv_buf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm *comm_ptr,
                                                  int context_offset,
                                                  int ep_idx,
                                                  MPI_Status *status,
                                                  MPIR_Request *request)
{
    int bucket_idx;
    MPIDI_workq_elemt_t* pt2pt_elemt = NULL;
    pt2pt_elemt = MPL_malloc(sizeof (*pt2pt_elemt));
    pt2pt_elemt->op       = op;
    pt2pt_elemt->send_buf = send_buf;
    pt2pt_elemt->recv_buf = recv_buf;
    pt2pt_elemt->count    = count;
    pt2pt_elemt->datatype = datatype;
    pt2pt_elemt->rank     = rank;
    pt2pt_elemt->tag      = tag;
    pt2pt_elemt->comm_ptr = comm_ptr;
    pt2pt_elemt->context_offset = context_offset;
    pt2pt_elemt->status  = status;
    pt2pt_elemt->request  = request;
    MPIDI_find_tag_bucket(comm_ptr, rank, tag, &bucket_idx);
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.ep_queues[ep_idx], pt2pt_elemt, bucket_idx);
}

static inline void MPIDI_workq_rma_enqueue_body(MPIDI_workq_op_t op,
                                                const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win *win_ptr,
                                                int ep_idx)
{
    int bucket_idx;
    MPIDI_workq_elemt_t* rma_elemt = NULL;
    rma_elemt = MPL_malloc(sizeof (*rma_elemt));
    rma_elemt->op               = op;
    rma_elemt->origin_addr      = origin_addr;
    rma_elemt->origin_count     = origin_count;
    rma_elemt->origin_datatype  = origin_datatype;
    rma_elemt->target_rank      = target_rank;
    rma_elemt->target_disp      = target_disp;
    rma_elemt->target_count     = target_count;
    rma_elemt->target_datatype  = target_datatype;
    rma_elemt->win_ptr          = win_ptr;
    MPIDI_find_rma_bucket(win_ptr, target_rank, &bucket_idx);
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.ep_queues[ep_idx], rma_elemt, bucket_idx);
}

#if !defined(WORKD_DEQ_RANGE)

static inline int MPIDI_workq_ep_progress_body(int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t* workq_elemt = NULL;
    MPIDI_workq_t *cur_workq = &MPIDI_CH4_Global.ep_queues[ep_idx];

    MPIDI_WORKQ_PROGRESS_START;

    MPIDI_workq_dequeue(cur_workq, (void**)&workq_elemt);
    MPIDI_WORKQ_QUEUE_TRAVERSAL(workq_elemt);
    while(workq_elemt != NULL) {
        MPIDI_WORKQ_ISSUE_START;
        switch(workq_elemt->op) {
        case SEND:
            mpi_errno = MPIDI_NM_mpi_send(workq_elemt->send_buf,
                                          workq_elemt->count,
                                          workq_elemt->datatype,
                                          workq_elemt->rank,
                                          workq_elemt->tag,
                                          workq_elemt->comm_ptr,
                                          workq_elemt->context_offset,
                                          &workq_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case ISEND:
            mpi_errno = MPIDI_NM_mpi_isend(workq_elemt->send_buf,
                                           workq_elemt->count,
                                           workq_elemt->datatype,
                                           workq_elemt->rank,
                                           workq_elemt->tag,
                                           workq_elemt->comm_ptr,
                                           workq_elemt->context_offset,
                                           &workq_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case ISSEND:
            mpi_errno = MPIDI_NM_mpi_issend(workq_elemt->send_buf,
                                           workq_elemt->count,
                                           workq_elemt->datatype,
                                           workq_elemt->rank,
                                           workq_elemt->tag,
                                           workq_elemt->comm_ptr,
                                           workq_elemt->context_offset,
                                           &workq_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case RECV:
            mpi_errno = MPIDI_NM_mpi_recv(workq_elemt->recv_buf,
                                           workq_elemt->count,
                                           workq_elemt->datatype,
                                           workq_elemt->rank,
                                           workq_elemt->tag,
                                           workq_elemt->comm_ptr,
                                           workq_elemt->context_offset,
                                           workq_elemt->status,
                                           &workq_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case IRECV:
            mpi_errno = MPIDI_NM_mpi_irecv(workq_elemt->recv_buf,
                                           workq_elemt->count,
                                           workq_elemt->datatype,
                                           workq_elemt->rank,
                                           workq_elemt->tag,
                                           workq_elemt->comm_ptr,
                                           workq_elemt->context_offset,
                                           &workq_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case PUT:
            mpi_errno = MPIDI_NM_mpi_put(workq_elemt->origin_addr,
                                         workq_elemt->origin_count,
                                         workq_elemt->origin_datatype,
                                         workq_elemt->target_rank,
                                         workq_elemt->target_disp,
                                         workq_elemt->target_count,
                                         workq_elemt->target_datatype,
                                         workq_elemt->win_ptr);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        }
        MPIDI_WORKQ_ISSUE_STOP;
        MPL_free(workq_elemt);
        MPIDI_workq_dequeue(cur_workq, (void**)&workq_elemt);
    }

    MPIDI_WORKQ_PROGRESS_STOP;

fn_fail:
    return mpi_errno;
}

#else

#include <omp.h>

#define WORKQ_BATCHSZ 16
#define WORKQ_PROGRESS_LEN 1
static inline int MPIDI_workq_ep_progress_body(int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    void* elemts[WORKQ_BATCHSZ] = {NULL};
    MPIDI_workq_elemt_t* workq_elemt;
    MPIDI_workq_t *cur_workq = &MPIDI_CH4_Global.ep_queues[ep_idx];
    int tid = omp_get_thread_num();
    int progress_partition = tid / WORKQ_PROGRESS_LEN;
    int start = progress_partition * WORKQ_PROGRESS_LEN;
    int stop = (progress_partition + 1) * WORKQ_PROGRESS_LEN;
    if (stop > MPIR_CVAR_CH4_WORKQ_NBUCKETS)
        stop = MPIR_CVAR_CH4_WORKQ_NBUCKETS;

    MPIDI_WORKQ_PROGRESS_START;

    while (1) {
        int out_count = 0, i;
        MPIDI_workq_dequeue_range(cur_workq, (void**)elemts, start, stop, WORKQ_BATCHSZ, &out_count);
        if(out_count > 0) {
            printf("[%d] dequeued %d elemts from [%d,%d[\n", tid, out_count, start, stop);
            for(i = 0; i < out_count; i++) {
                workq_elemt = (MPIDI_workq_elemt_t*) elemts[i];
                MPIDI_WORKQ_ISSUE_START;
                switch(workq_elemt->op) {
                case SEND:
                    mpi_errno = MPIDI_NM_mpi_send(workq_elemt->send_buf,
                                                  workq_elemt->count,
                                                  workq_elemt->datatype,
                                                  workq_elemt->rank,
                                                  workq_elemt->tag,
                                                  workq_elemt->comm_ptr,
                                                  workq_elemt->context_offset,
                                                  &workq_elemt->request);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                case ISEND:
                    mpi_errno = MPIDI_NM_mpi_isend(workq_elemt->send_buf,
                                                   workq_elemt->count,
                                                   workq_elemt->datatype,
                                                   workq_elemt->rank,
                                                   workq_elemt->tag,
                                                   workq_elemt->comm_ptr,
                                                   workq_elemt->context_offset,
                                                   &workq_elemt->request);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                case ISSEND:
                    mpi_errno = MPIDI_NM_mpi_issend(workq_elemt->send_buf,
                                                   workq_elemt->count,
                                                   workq_elemt->datatype,
                                                   workq_elemt->rank,
                                                   workq_elemt->tag,
                                                   workq_elemt->comm_ptr,
                                                   workq_elemt->context_offset,
                                                   &workq_elemt->request);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                case RECV:
                    mpi_errno = MPIDI_NM_mpi_recv(workq_elemt->recv_buf,
                                                   workq_elemt->count,
                                                   workq_elemt->datatype,
                                                   workq_elemt->rank,
                                                   workq_elemt->tag,
                                                   workq_elemt->comm_ptr,
                                                   workq_elemt->context_offset,
                                                   workq_elemt->status,
                                                   &workq_elemt->request);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                case IRECV:
                    mpi_errno = MPIDI_NM_mpi_irecv(workq_elemt->recv_buf,
                                                   workq_elemt->count,
                                                   workq_elemt->datatype,
                                                   workq_elemt->rank,
                                                   workq_elemt->tag,
                                                   workq_elemt->comm_ptr,
                                                   workq_elemt->context_offset,
                                                   &workq_elemt->request);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                case PUT:
                    mpi_errno = MPIDI_NM_mpi_put(workq_elemt->origin_addr,
                                                 workq_elemt->origin_count,
                                                 workq_elemt->origin_datatype,
                                                 workq_elemt->target_rank,
                                                 workq_elemt->target_disp,
                                                 workq_elemt->target_count,
                                                 workq_elemt->target_datatype,
                                                 workq_elemt->win_ptr);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    break;
                }
                MPIDI_WORKQ_ISSUE_STOP;
                MPL_free(workq_elemt);
            }
        } else {
            break;
        }
    }

    MPIDI_WORKQ_PROGRESS_STOP;

fn_fail:
    return mpi_errno;
}

#endif

static inline void MPIDI_workq_pt2pt_enqueue(MPIDI_workq_op_t op,
                                             const void *send_buf,
                                             void *recv_buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm *comm_ptr,
                                             int context_offset,
                                             int ep_idx,
                                             MPI_Status *status,
                                             MPIR_Request *request)
{
    MPIDI_WORKQ_PT2PT_ENQUEUE_START;
    MPIDI_workq_pt2pt_enqueue_body(op, send_buf, recv_buf, count, datatype,
                                   rank, tag, comm_ptr, context_offset, ep_idx, status, request);
    MPIDI_WORKQ_PT2PT_ENQUEUE_STOP;
}

static inline void MPIDI_workq_rma_enqueue(MPIDI_workq_op_t op,
                                           const void *origin_addr,
                                           int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank,
                                           MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype,
                                           MPIR_Win *win_ptr,
                                           int ep_idx)
{
    MPIDI_WORKQ_RMA_ENQUEUE_START;
    MPIDI_workq_rma_enqueue_body(op, origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype,
                                 win_ptr, ep_idx);
    MPIDI_WORKQ_RMA_ENQUEUE_STOP;
}

static inline int MPIDI_workq_ep_progress(int ep_idx)
{
    int mpi_errno;
    mpi_errno = MPIDI_workq_ep_progress_body(ep_idx);
    return mpi_errno;
}

static inline int MPIDI_workq_global_progress(int* made_progress)
{
    int mpi_errno = MPI_SUCCESS, ep_idx, cs_acq = 0;
    *made_progress = 1;
    for( ep_idx = 0; ep_idx < MPIDI_CH4_Global.n_netmod_eps; ep_idx++) {
        MPID_THREAD_CS_TRYENTER(EP, MPIDI_CH4_Global.ep_locks[ep_idx], cs_acq);
        if (cs_acq) {
            mpi_errno = MPIDI_workq_ep_progress(ep_idx);
            if(unlikely(mpi_errno != MPI_SUCCESS)) {
                MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);
                abort();
                break;
            }
            MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);
        }
    }
    return mpi_errno;
}

#define MPIDI_DISPATCH_PT2PT_RECV(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
    err = func(recv_buf, count, datatype, rank, tag, comm, context_offset, status, request);
#define MPIDI_DISPATCH_PT2PT_IRECV(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
    err = func(recv_buf, count, datatype, rank, tag, comm, context_offset, request);
#define MPIDI_DISPATCH_PT2PT_SEND(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
    err = func(send_buf, count, datatype, rank, tag, comm, context_offset, request);
#define MPIDI_DISPATCH_PT2PT_ISEND(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
    err = func(send_buf, count, datatype, rank, tag, comm, context_offset, request);
#define MPIDI_DISPATCH_PT2PT_ISSEND(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
    err = func(send_buf, count, datatype, rank, tag, comm, context_offset, request);

#define MPIDI_DISPATCH_RMA_PUT(func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err) \
    err = func(org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win);

#ifdef MPIDI_CH4_MT_DIRECT
#define MPIDI_DISPATCH_PT2PT(op, func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err)   \
do {                                                                                                        \
    int ep_idx;                                                                                             \
    MPIDI_find_tag_ep(comm, rank, tag, &ep_idx);                                                            \
    MPID_THREAD_CS_ENTER(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                            \
    MPIDI_DISPATCH_PT2PT_##op(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err)\
    MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                             \
} while (0)

#define MPIDI_DISPATCH_RMA(op, func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err)   \
do {                                                                                                        \
    int ep_idx;                                                                                             \
    MPIDI_find_rma_ep(win, trg_rank, &ep_idx);                                                              \
    MPID_THREAD_CS_ENTER(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                            \
    MPIDI_DISPATCH_RMA_##op(func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err)\
    MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                             \
} while (0)

#elif defined (MPIDI_CH4_MT_HANDOFF)
#define MPIDI_DISPATCH_PT2PT(op, func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
do {                                                                                                        \
    err = MPI_SUCCESS;                                                                                      \
    int ep_idx, bucket;                                                                                     \
    MPIDI_find_tag_bucket(comm, rank, tag, &bucket);                                                        \
    if (op == SEND || op == ISEND)                                                                          \
        *request = MPIR_Request_create_buck(MPIR_REQUEST_KIND__SEND, bucket);                               \
    else if (op == RECV || op == IRECV)                                                                     \
        *request = MPIR_Request_create_buck(MPIR_REQUEST_KIND__RECV, bucket);                               \
    MPIDI_find_tag_ep(comm, rank, tag, &ep_idx);                                                            \
    MPIDI_workq_pt2pt_enqueue(op, send_buf, recv_buf, count, datatype,                                      \
                              rank, tag, comm, context_offset, ep_idx, status, *request);                   \
} while (0)

#define MPIDI_DISPATCH_RMA(op, func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err) \
do {                                                                                                        \
    err = MPI_SUCCESS;                                                                                      \
    int ep_idx;                                                                                             \
    MPIDI_find_rma_ep(win, trg_rank, &ep_idx);                                                              \
    MPIDI_workq_rma_enqueue(op, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, ep_idx);\
} while (0)

#elif defined (MPIDI_CH4_MT_TRYLOCK)
#define MPIDI_DISPATCH_PT2PT(op, func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err) \
do {                                                                                                        \
    err = MPI_SUCCESS;                                                                                      \
    int ep_idx, cs_acq = 0, bucket;                                                                         \
    MPIDI_find_tag_bucket(comm, rank, tag, &bucket);                                                        \
    MPIDI_find_tag_ep(comm, rank, tag, &ep_idx);                                                            \
    MPID_THREAD_CS_TRYENTER(EP, MPIDI_CH4_Global.ep_locks[ep_idx], cs_acq);                                 \
    if(!cs_acq) {                                                                                           \
        if (op == ISEND || op == ISSEND || op == SEND)                                                      \
            *request = MPIR_Request_create_buck(MPIR_REQUEST_KIND__SEND, bucket);                           \
        else if (op == IRECV || op == RECV)                                                                 \
            *request = MPIR_Request_create_buck(MPIR_REQUEST_KIND__RECV, bucket);                           \
        MPIDI_workq_pt2pt_enqueue(op, send_buf, recv_buf, count, datatype,                                  \
                                  rank, tag, comm, context_offset, ep_idx, status, *request);               \
    } else {                                                                                                \
        MPIDI_DISPATCH_PT2PT_##op(func, send_buf, recv_buf, count, datatype, rank, tag, comm, context_offset, status, request, err);\
        MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                         \
    }                                                                                                       \
} while (0)

#define MPIDI_DISPATCH_RMA(op, func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err) \
do {                                                                                                        \
    err = MPI_SUCCESS;                                                                                      \
    int ep_idx, cs_acq = 0;                                                                                 \
    MPIDI_find_rma_ep(win,trg_rank, &ep_idx);                                                               \
    MPID_THREAD_CS_TRYENTER(EP, MPIDI_CH4_Global.ep_locks[ep_idx], cs_acq);                                 \
    if(!cs_acq) {                                                                                           \
        MPIDI_workq_rma_enqueue(op, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, ep_idx);\
    } else {                                                                                                \
        MPIDI_workq_ep_progress(ep_idx);                                                                    \
        MPIDI_DISPATCH_RMA_##op(func, org_addr, org_count, org_dt, trg_rank, trg_disp, trg_count, trg_dt, win, err);\
        MPID_THREAD_CS_EXIT(EP, MPIDI_CH4_Global.ep_locks[ep_idx]);                                         \
    }                                                                                                       \
} while (0)


#else
#error "Unknown thread safety model"
#endif

#endif /* CH4I_WORKQ_H_INCLUDED */

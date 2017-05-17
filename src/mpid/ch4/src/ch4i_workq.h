/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

#include "ch4i_workq_types.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_zm_msqueue_init(struct MPIDI_workq *q)
{
    zm_msqueue_init(&q->zm_msqueue);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_zm_msqueue_enqueue(struct MPIDI_workq *q,
    void *p)
{
    zm_msqueue_enqueue(&q->zm_msqueue, p);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_zm_msqueue_dequeue_bulk(struct MPIDI_workq *q,
                                                           void **p,
                                                           int n)
{
    zm_msqueue_dequeue(&q->zm_msqueue, p);
    if (*p == NULL)
        return 0;
    else
        return 1;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_zm_glqueue_init(struct MPIDI_workq *q)
{
    zm_glqueue_init(&q->zm_glqueue);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_zm_glqueue_enqueue(struct MPIDI_workq *q,
    void *p)
{
    zm_glqueue_enqueue(&q->zm_glqueue, p);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_zm_glqueue_dequeue_bulk(struct MPIDI_workq *q,
                                                           void **p,
                                                           int n)
{
    zm_glqueue_dequeue(&q->zm_glqueue, p);
    if (*p == NULL)
        return 0;
    else
        return 1;
}

#ifdef MPIDI_CH4_USE_WORKQ_RUNTIME
MPL_STATIC_INLINE_PREFIX int MPIDI_workq_init(MPIDI_workq_t *q)
{
    memset(q, 0, sizeof(*q));

    switch (MPIDI_CH4_WORKQ_TYPE) {
    case MPIDI_CH4_WORKQ_ZM_MSQUEUE:
        q->enqueue = MPIDI_zm_msqueue_enqueue;
        q->dequeue_bulk = MPIDI_zm_msqueue_dequeue_bulk;

        return zm_msqueue_init(&q->zm_msqueue);

    case MPIDI_CH4_WORKQ_ZM_GLQUEUE:
        q->enqueue = MPIDI_zm_glqueue_enqueue;
        q->dequeue_bulk = MPIDI_zm_glqueue_dequeue_bulk;

        return zm_glqueue_init(&q->zm_glqueue);

    default:
        MPIR_Assert(0);
        return -1;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_enqueue(struct MPIDI_workq *q, void *p)
{
    q->enqueue(q, p);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_dequeue_bulk(struct MPIDI_workq *q, void **pp, int n)
{
    return q->dequeue_bulk(q, pp, n);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_finalize(struct MPIDI_workq *q)
{
    if (q->finalize)
        q->finalize(q);
}
#else /* MPIDI_CH4_USE_WORKQ_RUNTIME */
#  if MPIDI_CH4_WORKQ_TYPE == MPIDI_CH4_WORKQ_ZM_MSQUEUE
#    define MPIDI_workq_init         MPIDI_zm_msqueue_init
#    define MPIDI_workq_enqueue      MPIDI_zm_msqueue_enqueue
#    define MPIDI_workq_dequeue_bulk MPIDI_zm_msqueue_dequeue_bulk
#    define MPIDI_workq_finalize(q)  do {} while (0)
#  elif MPIDI_CH4_WORKQ_TYPE == MPIDI_CH4_WORKQ_ZM_GLQUEUE
#    define MPIDI_workq_init         MPIDI_zm_glqueue_init
#    define MPIDI_workq_enqueue      MPIDI_zm_glqueue_enqueue
#    define MPIDI_workq_dequeue_bulk MPIDI_zm_glqueue_dequeue_bulk
#    define MPIDI_workq_finalize(q)  do {} while (0)
#  endif
#endif /* MPIDI_CH4_USE_WORKQ_RUNTIME */

#define MPIDI_REQUEST_KIND_SEND MPIR_REQUEST_KIND__SEND
#define MPIDI_REQUEST_KIND_ISEND MPIR_REQUEST_KIND__SEND
#define MPIDI_REQUEST_KIND_RECV MPIR_REQUEST_KIND__RECV
#define MPIDI_REQUEST_KIND_IRECV MPIR_REQUEST_KIND__RECV
#define MPIDI_REQUEST_KIND_PUT MPIR_REQUEST_KIND__RMA

#define MPIDI_EXTRACT_SEND_ARGS(elem)                             \
    (elem)->send_buf,                                             \
        (elem)->count,                                            \
        (elem)->datatype,                                         \
        (elem)->rank,                                             \
        (elem)->tag,                                              \
        (elem)->comm_ptr,                                         \
        (elem)->context_offset,                                   \
        (elem)->pt2pt_addr,                                       \
        &(elem)->request

#define MPIDI_EXTRACT_RECV_ARGS(elem)                             \
    (elem)->recv_buf,                                             \
        (elem)->count,                                            \
        (elem)->datatype,                                         \
        (elem)->rank,                                             \
        (elem)->tag,                                              \
        (elem)->comm_ptr,                                         \
        (elem)->context_offset,                                   \
        (elem)->pt2pt_addr,                                       \
        (elem)->status,                                           \
        &(elem)->request

#define MPIDI_EXTRACT_IRECV_ARGS(elem)                             \
    (elem)->recv_buf,                                             \
        (elem)->count,                                            \
        (elem)->datatype,                                         \
        (elem)->rank,                                             \
        (elem)->tag,                                              \
        (elem)->comm_ptr,                                         \
        (elem)->context_offset,                                   \
        (elem)->pt2pt_addr,                                       \
        &(elem)->request

#define MPIDI_EXTRACT_PUTGET_ARGS(elem)         \
    (elem)->origin_addr,                        \
        (elem)->origin_count,                   \
        (elem)->origin_datatype,                \
        (elem)->target_rank,                    \
        (elem)->target_disp,                    \
        (elem)->target_count,                   \
        (elem)->target_datatype,                                \
        (elem)->win_ptr,                                        \
        (elem)->rma_addr

#define MPIDI_INVOKE_DIRECT_SEND(args...) MPIDI_NM_mpi_send(args)
#define MPIDI_INVOKE_DIRECT_ISEND(args...) MPIDI_NM_mpi_isend(args)
#define MPIDI_INVOKE_DIRECT_RECV(args...) MPIDI_NM_mpi_recv(args)
#define MPIDI_INVOKE_DIRECT_IRECV(args...) MPIDI_NM_mpi_irecv(args)
#define MPIDI_INVOKE_DIRECT_PUT(args...) MPIDI_NM_mpi_put(args)

#define MPIDI_INVOKE_DEFERRED_SEND(elem) MPIDI_NM_mpi_send(MPIDI_EXTRACT_SEND_ARGS(elem))
#define MPIDI_INVOKE_DEFERRED_ISEND(elem) MPIDI_NM_mpi_isend(MPIDI_EXTRACT_SEND_ARGS(elem))
#define MPIDI_INVOKE_DEFERRED_RECV(elem) MPIDI_NM_mpi_recv(MPIDI_EXTRACT_RECV_ARGS(elem))
#define MPIDI_INVOKE_DEFERRED_IRECV(elem) MPIDI_NM_mpi_irecv(MPIDI_EXTRACT_IRECV_ARGS(elem))
#define MPIDI_INVOKE_DEFERRED_PUT(elem) MPIDI_NM_mpi_put(MPIDI_EXTRACT_PUTGET_ARGS(elem))

#define MPIDI_INVOKE_DEFERRED(op, elem)                         \
    case op: {                                                  \
        mpi_errno = MPIDI_INVOKE_DEFERRED_##op(elem);           \
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;         \
        break;                                                  \
    }

/* For profiling */
extern double MPIDI_pt2pt_enqueue_time;
extern double MPIDI_pt2pt_progress_time;
extern double MPIDI_pt2pt_issue_pend_time;
extern double MPIDI_rma_enqueue_time;
extern double MPIDI_rma_progress_time;
extern double MPIDI_rma_issue_pend_time;

#if defined(MPIDI_WORKQ_PROFILE)
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START  double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP                          \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_pt2pt_enqueue_time += (enqueue_t2 - enqueue_t1);  \
    }while(0)
#define MPIDI_WORKQ_PROGRESS_START double progress_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PROGRESS_STOP                                 \
    do {                                                                \
        double progress_t2 = MPI_Wtime();                               \
        MPIDI_pt2pt_progress_time += (progress_t2 - progress_t1);       \
    }while(0)
#define MPIDI_WORKQ_ISSUE_START    double issue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_ISSUE_STOP                            \
    do {                                                        \
        double issue_t2 = MPI_Wtime();                          \
        MPIDI_pt2pt_issue_pend_time += (issue_t2 - issue_t1);   \
    }while(0)

#define MPIDI_WORKQ_RMA_ENQUEUE_START    double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP                            \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_rma_enqueue_time += (enqueue_t2 - enqueue_t1);    \
    }while(0)
#else /*!defined(MPIDI_WORKQ_PROFILE)*/
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP
#define MPIDI_WORKQ_PROGRESS_START
#define MPIDI_WORKQ_PROGRESS_STOP
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
                                                  MPIDI_av_entry_t *addr,
                                                  int vni_idx,
                                                  MPI_Status *status,
                                                  MPIR_Request *request)
{
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
    pt2pt_elemt->pt2pt_addr = addr;
    pt2pt_elemt->status   = status;
    pt2pt_elemt->request  = request;

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_workq_enqueue(&comm_ptr->dev.work_queues[vni_idx].pend_ops, pt2pt_elemt);
    else
        MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueues.pvni[vni_idx], pt2pt_elemt);
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
                                                MPIDI_av_entry_t *addr,
                                                int vni_idx)
{
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
    rma_elemt->rma_addr         = addr;

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_workq_enqueue(&win_ptr->dev.work_queues[vni_idx].pend_ops, rma_elemt);
    else
        MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueues.pvni[vni_idx], rma_elemt);
}

static inline int MPIDI_workq_dispatch(MPIDI_workq_elemt_t* workq_elemt)
{
    int mpi_errno = MPI_SUCCESS;

    switch(workq_elemt->op) {
        MPIDI_INVOKE_DEFERRED(SEND, workq_elemt);
        MPIDI_INVOKE_DEFERRED(ISEND, workq_elemt);
        MPIDI_INVOKE_DEFERRED(RECV, workq_elemt);
        MPIDI_INVOKE_DEFERRED(IRECV, workq_elemt);
        MPIDI_INVOKE_DEFERRED(PUT, workq_elemt);
    }

fn_fail:
    return mpi_errno;
}

#define MPIDI_WORKQ_DEQUEUE_SIZE 128

static inline int MPIDI_workq_vni_progress_pobj(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t* workq_elemts[MPIDI_WORKQ_DEQUEUE_SIZE];
    MPIDI_workq_list_t *cur_workq;

    MPIR_Assert(MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    MPIDI_WORKQ_PROGRESS_START;
    MPL_DL_FOREACH(MPIDI_CH4_Global.workqueues.pobj[vni_idx], cur_workq) {
        int n;
        do {
            int i;
            n = MPIDI_workq_dequeue_bulk(&cur_workq->pend_ops,
                                         (void**)workq_elemts, MPIDI_WORKQ_DEQUEUE_SIZE);
            for (i = 0; i < n; i++) {
                MPIDI_WORKQ_ISSUE_START;
                mpi_errno = MPIDI_workq_dispatch(workq_elemts[i]);
                if (mpi_errno != MPI_SUCCESS) {
                    int j;
                    for (j = i; j < n; j++)
                        MPL_free(workq_elemts[j]);
                    goto fn_fail;
                }
                MPIDI_WORKQ_ISSUE_STOP;
                MPL_free(workq_elemts[i]);
            }
        } while (n > 0);
    }
    MPIDI_WORKQ_PROGRESS_STOP;
fn_fail:
    return mpi_errno;
}

static inline int MPIDI_workq_vni_progress_pvni(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t* workq_elemts[MPIDI_WORKQ_DEQUEUE_SIZE];
    int n;

    MPIR_Assert(!MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    MPIDI_WORKQ_PROGRESS_START;

    do {
        int i;
        n = MPIDI_workq_dequeue_bulk(&MPIDI_CH4_Global.workqueues.pvni[vni_idx],
                                     (void**)workq_elemts, MPIDI_WORKQ_DEQUEUE_SIZE);
        for (i = 0; i < n; i++) {
            MPIDI_WORKQ_ISSUE_START;
            mpi_errno = MPIDI_workq_dispatch(workq_elemts[i]);
            if (mpi_errno != MPI_SUCCESS) {
                    int j;
                    for (j = i; j < n; j++)
                        MPL_free(workq_elemts[j]);
                goto fn_fail;
            }
            MPIDI_WORKQ_ISSUE_STOP;
            MPL_free(workq_elemts[i]);
        }
    } while (n > 0);
    MPIDI_WORKQ_PROGRESS_STOP;
fn_fail:
    return mpi_errno;
}

static inline void MPIDI_workq_pt2pt_enqueue(MPIDI_workq_op_t op,
                                             const void *send_buf,
                                             void *recv_buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm *comm_ptr,
                                             int context_offset,
                                             MPIDI_av_entry_t *addr,
                                             int vni_idx,
                                             MPI_Status *status,
                                             MPIR_Request *request)
{
    MPIDI_WORKQ_PT2PT_ENQUEUE_START;
    MPIDI_workq_pt2pt_enqueue_body(op, send_buf, recv_buf, count, datatype,
                                   rank, tag, comm_ptr, context_offset, addr, vni_idx, status, request);
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
                                           MPIDI_av_entry_t *addr,
                                           int vni_idx)
{
    MPIDI_WORKQ_RMA_ENQUEUE_START;
    MPIDI_workq_rma_enqueue_body(op, origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype,
                                 win_ptr, addr, vni_idx);
    MPIDI_WORKQ_RMA_ENQUEUE_STOP;
}

static inline int MPIDI_workq_vni_progress(int vni_idx)
{
    int mpi_errno;

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES) {
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        mpi_errno = MPIDI_workq_vni_progress_pobj(vni_idx);
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
    }
    else {
        /* Per-VNI workqueue does not require VNI lock, since
           the queue is lock free */
        mpi_errno = MPIDI_workq_vni_progress_pvni(vni_idx);
    }

    return mpi_errno;
}

static inline int MPIDI_workq_global_progress(int* made_progress)
{
    int mpi_errno, vni_idx;
    *made_progress = 1;
    for (vni_idx = 0; vni_idx < MPIDI_CH4_Global.n_netmod_vnis; vni_idx++) {
        mpi_errno = MPIDI_workq_vni_progress(vni_idx);
        if(unlikely(mpi_errno != MPI_SUCCESS))
            break;
    }
    return mpi_errno;
}

#define MPIDI_ENQUEUE_SEND(opval, vni_idx, buf, count, datatype, rank, tag, comm, context_offset, addr, request) \
    MPIDI_workq_pt2pt_enqueue(opval, buf, NULL /* recv_buf */, count, datatype, \
                              rank, tag, comm, context_offset, addr, vni_idx, \
                              NULL /* status */, *request)
#define MPIDI_ENQUEUE_ISEND MPIDI_ENQUEUE_SEND
#define MPIDI_ENQUEUE_RECV(opval, vni_idx, buf, count, datatype, rank, tag, comm, context_offset, addr, status, request) \
    MPIDI_workq_pt2pt_enqueue(opval, NULL /* send_buf */, buf, count, datatype, \
                              rank, tag, comm, context_offset, addr, vni_idx, \
                              status, *request)

#define MPIDI_ENQUEUE_IRECV(opval, vni_idx, buf, count, datatype, rank, tag, comm, context_offset, addr, request) \
    MPIDI_workq_pt2pt_enqueue(opval, NULL /* send_buf */, buf, count, datatype, \
                              rank, tag, comm, context_offset, addr, vni_idx, \
                              NULL /* status */, *request)
#define MPIDI_ENQUEUE_PUT(opval, vni_idx, origin_addr, origin_count, origin_datatype, target_rank, \
                          target_disp, target_count, target_datatype, win, addr) \
    MPIDI_workq_rma_enqueue(vni_idx, origin_addr, origin_count, origin_datatype, target_rank, \
                            target_disp, target_count, target_datatype, win, addr)
/*
  MPIDI_DISPATCH - dispatching macro for each MPI functions
  args must include an argument called `request`, of type `MPIR_Request **`
*/
#define MPIDI_DISPATCH(op, mpi_errno, args...)                          \
    do {                                                                \
        int vni_idx_, cs_acq = 0;                                       \
        MPIDI_find_tag_vni(comm, rank, tag, &vni_idx_);                 \
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_TRYLOCK) {               \
            MPID_THREAD_CS_TRYENTER(VNI, MPIDI_CH4_Global.vni_locks[vni_idx_], cs_acq); \
            if(!cs_acq) {                                               \
                *(request) = MPIR_Request_create(MPIDI_REQUEST_KIND_##op); \
                MPIDI_ENQUEUE_##op(op, vni_idx_, args);                 \
                (mpi_errno) = MPI_SUCCESS;                              \
            } else {                                                    \
                mpi_errno = MPIDI_INVOKE_DIRECT_##op(args);             \
                MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[vni_idx_]); \
            }                                                           \
        } else {                                                        \
            /* Direct && Handoff */                                     \
            if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF) {           \
                *(request) = MPIR_Request_create(MPIDI_REQUEST_KIND_##op); \
            }                                                           \
            if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF && MPIR_ThreadInfo.isThreaded) { \
                MPIDI_ENQUEUE_##op(op, vni_idx_, args);                 \
                (mpi_errno) = MPI_SUCCESS;                              \
            } else {                                                    \
                MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[vni_idx_]); \
                mpi_errno = MPIDI_INVOKE_DIRECT_##op(args);             \
                MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[vni_idx_]); \
            }                                                           \
        }                                                               \
    } while (0)

#endif /* CH4I_WORKQ_H_INCLUDED */

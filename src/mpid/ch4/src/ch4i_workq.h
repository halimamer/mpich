/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

#include "ch4i_workq_types.h"
#include <utlist.h>

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC];
extern MPIR_Object_alloc_t MPIDI_workq_elemt_mem;

/* Forward declarations of the routines that can be pushed to a work-queue */

MPL_STATIC_INLINE_PREFIX int MPIDI_send_unsafe(const void *, int, MPI_Datatype, int, int,
                                               MPIR_Comm *, int, MPIDI_av_entry_t *,
                                               MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_isend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_ssend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_issend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                 MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                 MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_recv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                               MPIR_Comm *, int, MPIDI_av_entry_t *, MPI_Status *,
                                               MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_irecv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_imrecv_unsafe(void *, MPI_Aint, MPI_Datatype, MPIR_Request *,
                                                 MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_put_unsafe(const void *, int, MPI_Datatype, int, MPI_Aint, int,
                                              MPI_Datatype, MPIR_Win *);
MPL_STATIC_INLINE_PREFIX int MPIDI_get_unsafe(void *, int, MPI_Datatype, int, MPI_Aint, int,
                                              MPI_Datatype, MPIR_Win *);
MPL_STATIC_INLINE_PREFIX struct MPIDI_workq_elemt *MPIDI_workq_elemt_create(void)
{
    return MPIR_Handle_obj_alloc(&MPIDI_workq_elemt_mem);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_elemt_free(struct MPIDI_workq_elemt *elemt)
{
    MPIR_Handle_obj_free(&MPIDI_workq_elemt_mem, elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_pt2pt_enqueue(MPIDI_workq_op_t op,
                                                        const void *send_buf,
                                                        void *recv_buf,
                                                        MPI_Aint count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm_ptr,
                                                        int context_offset,
                                                        MPIDI_av_entry_t * addr,
                                                        MPI_Status * status,
                                                        MPIR_Request * request,
                                                        int *flag,
                                                        MPIR_Request ** message,
                                                        OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *pt2pt_elemt;

    MPIR_Assert(request != NULL);

    MPIR_Request_add_ref(request);
    pt2pt_elemt = &request->dev.ch4.command;
    pt2pt_elemt->op = op;
    pt2pt_elemt->processed = processed;

    switch (op) {
        case SEND:
        case ISEND:
        case SSEND:
        case ISSEND:
        case RSEND:
        case IRSEND:
            {
                struct MPIDI_workq_send *sendop = &pt2pt_elemt->params.pt2pt.send;
                sendop->send_buf = send_buf;
                sendop->count = count;
                sendop->datatype = datatype;
                sendop->rank = rank;
                sendop->tag = tag;
                sendop->comm_ptr = comm_ptr;
                sendop->context_offset = context_offset;
                sendop->addr = addr;
                sendop->request = request;
                break;
            }
        case RECV:
            {
                struct MPIDI_workq_recv *recvop = &pt2pt_elemt->params.pt2pt.recv;
                recvop->recv_buf = recv_buf;
                recvop->count = count;
                recvop->datatype = datatype;
                recvop->rank = rank;
                recvop->tag = tag;
                recvop->comm_ptr = comm_ptr;
                recvop->context_offset = context_offset;
                recvop->addr = addr;
                recvop->status = status;
                recvop->request = request;
                break;
            }
        case IRECV:
            {
                struct MPIDI_workq_irecv *irecvop = &pt2pt_elemt->params.pt2pt.irecv;
                irecvop->recv_buf = recv_buf;
                irecvop->count = count;
                irecvop->datatype = datatype;
                irecvop->rank = rank;
                irecvop->tag = tag;
                irecvop->comm_ptr = comm_ptr;
                irecvop->context_offset = context_offset;
                irecvop->addr = addr;
                irecvop->request = request;
                break;
            }
        case IPROBE:
            {
                struct MPIDI_workq_iprobe *iprobeop = &pt2pt_elemt->params.pt2pt.iprobe;
                iprobeop->count = count;
                iprobeop->datatype = datatype;
                iprobeop->rank = rank;
                iprobeop->tag = tag;
                iprobeop->comm_ptr = comm_ptr;
                iprobeop->context_offset = context_offset;
                iprobeop->addr = addr;
                iprobeop->status = status;
                iprobeop->request = request;
                iprobeop->flag = flag;
                break;
            }
        case IMPROBE:
            {
                struct MPIDI_workq_improbe *improbeop = &pt2pt_elemt->params.pt2pt.improbe;
                improbeop->count = count;
                improbeop->datatype = datatype;
                improbeop->rank = rank;
                improbeop->tag = tag;
                improbeop->comm_ptr = comm_ptr;
                improbeop->context_offset = context_offset;
                improbeop->addr = addr;
                improbeop->status = status;
                improbeop->request = request;
                improbeop->flag = flag;
                improbeop->message = message;
                break;
            }
        case IMRECV:
            {
                struct MPIDI_workq_imrecv *imrecvop = &pt2pt_elemt->params.pt2pt.imrecv;
                imrecvop->buf = recv_buf;
                imrecvop->count = count;
                imrecvop->datatype = datatype;
                imrecvop->message = message;
                imrecvop->request = request;
                break;
            }
        default:
            MPIR_Assert(0);
    }

    MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueue, pt2pt_elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_rma_enqueue(MPIDI_workq_op_t op,
                                                      const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      const void *compare_addr,
                                                      void *result_addr,
                                                      int result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op acc_op,
                                                      MPIR_Win * win_ptr,
                                                      MPIDI_av_entry_t * addr,
                                                      OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *rma_elemt = NULL;
    rma_elemt = MPIDI_workq_elemt_create();
    rma_elemt->op = op;
    rma_elemt->processed = processed;

    switch (op) {
        case PUT:
            {
                struct MPIDI_workq_put *putop = &rma_elemt->params.rma.put;
                putop->origin_addr = origin_addr;
                putop->origin_count = origin_count;
                putop->origin_datatype = origin_datatype;
                putop->target_rank = target_rank;
                putop->target_disp = target_disp;
                putop->target_count = target_count;
                putop->target_datatype = target_datatype;
                putop->win_ptr = win_ptr;
                putop->addr = addr;
                break;
            }
        case GET:
            {
                struct MPIDI_workq_get *getop = &rma_elemt->params.rma.get;
                getop->origin_addr = result_addr;
                getop->origin_count = result_count;
                getop->origin_datatype = result_datatype;
                getop->target_rank = target_rank;
                getop->target_disp = target_disp;
                getop->target_count = target_count;
                getop->target_datatype = target_datatype;
                getop->win_ptr = win_ptr;
                getop->addr = addr;
                break;
            }
        default:
            MPIR_Assert(0);
    }
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueue, rma_elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_release_pt2pt_elemt(MPIDI_workq_elemt_t * workq_elemt)
{
    MPIR_Request *req;
    req = MPL_container_of(workq_elemt, MPIR_Request, dev.ch4.command);
    MPIR_Request_free(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_dispatch(MPIDI_workq_elemt_t * workq_elemt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;

    MPIR_Assert(workq_elemt != NULL);

    switch (workq_elemt->op) {
        case SEND:
            {
                struct MPIDI_workq_send *sendop = &workq_elemt->params.pt2pt.send;
                req = sendop->request;
                MPIDI_send_unsafe(sendop->send_buf, sendop->count,
                                  sendop->datatype, sendop->rank,
                                  sendop->tag, sendop->comm_ptr,
                                  sendop->context_offset, sendop->addr, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case ISEND:
            {
                struct MPIDI_workq_send *isendop = &workq_elemt->params.pt2pt.send;
                req = isendop->request;
                MPIDI_isend_unsafe(isendop->send_buf, isendop->count,
                                   isendop->datatype, isendop->rank,
                                   isendop->tag, isendop->comm_ptr,
                                   isendop->context_offset, isendop->addr, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case SSEND:
            {
                struct MPIDI_workq_send *ssendop = &workq_elemt->params.pt2pt.send;
                req = ssendop->request;
                MPIDI_ssend_unsafe(ssendop->send_buf, ssendop->count,
                                   ssendop->datatype, ssendop->rank,
                                   ssendop->tag, ssendop->comm_ptr,
                                   ssendop->context_offset, ssendop->addr, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case ISSEND:
            {
                struct MPIDI_workq_send *issendop = &workq_elemt->params.pt2pt.send;
                req = issendop->request;
                MPIDI_issend_unsafe(issendop->send_buf, issendop->count,
                                    issendop->datatype, issendop->rank,
                                    issendop->tag, issendop->comm_ptr,
                                    issendop->context_offset, issendop->addr, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case RECV:
            {
                struct MPIDI_workq_recv *recvop = &workq_elemt->params.pt2pt.recv;
                req = recvop->request;
                MPIDI_recv_unsafe(recvop->recv_buf, recvop->count,
                                  recvop->datatype, recvop->rank,
                                  recvop->tag, recvop->comm_ptr,
                                  recvop->context_offset, recvop->addr, recvop->status, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case IRECV:
            {
                struct MPIDI_workq_irecv *irecvop = &workq_elemt->params.pt2pt.irecv;
                req = irecvop->request;
                MPIDI_irecv_unsafe(irecvop->recv_buf, irecvop->count,
                                   irecvop->datatype, irecvop->rank,
                                   irecvop->tag, irecvop->comm_ptr,
                                   irecvop->context_offset, irecvop->addr, &req);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case IMRECV:
            {
                struct MPIDI_workq_imrecv *imrecvop = &workq_elemt->params.pt2pt.imrecv;
                req = imrecvop->request;
                MPIDI_imrecv_unsafe(imrecvop->buf, imrecvop->count,
                                    imrecvop->datatype, *imrecvop->message, &req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
                if (!MPIDI_CH4I_REQUEST(*imrecvop->message, is_local))
#endif
                    MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case PUT:
            {
                struct MPIDI_workq_put *putop = &workq_elemt->params.rma.put;
                MPIDI_put_unsafe(putop->origin_addr, putop->origin_count,
                                 putop->origin_datatype, putop->target_rank,
                                 putop->target_disp, putop->target_count,
                                 putop->target_datatype, putop->win_ptr);
                MPIDI_workq_elemt_free(workq_elemt);
                break;
            }
        case GET:
            {
                struct MPIDI_workq_get *getop = &workq_elemt->params.rma.get;
                MPIDI_get_unsafe(getop->origin_addr, getop->origin_count,
                                 getop->origin_datatype, getop->target_rank,
                                 getop->target_disp, getop->target_count,
                                 getop->target_datatype, getop->win_ptr);
                MPIDI_workq_elemt_free(workq_elemt);
                break;
            }
        default:
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
    }

  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_unsafe(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t *workq_elemt = NULL;

    MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueue, (void **) &workq_elemt);
    while (workq_elemt != NULL) {
        mpi_errno = MPIDI_workq_dispatch(workq_elemt);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueue, (void **) &workq_elemt);
  } fn_fail:return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);

    mpi_errno = MPIDI_workq_vni_progress_unsafe();

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
  fn_fail:
    return mpi_errno;
}

#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
#define MPIDI_workq_pt2pt_enqueue(...)
#define MPIDI_workq_rma_enqueue(...)

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_unsafe(void)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress(void)
{
    return MPI_SUCCESS;
}
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

#endif /* CH4I_WORKQ_H_INCLUDED */

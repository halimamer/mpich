/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4I_WORKQ_TYPES_H_INCLUDED
#define CH4I_WORKQ_TYPES_H_INCLUDED

/*
  Multi-threading models
*/
enum {
    MPIDI_CH4_MT_DIRECT,
    MPIDI_CH4_MT_HANDOFF,
    MPIDI_CH4_MT_TRYLOCK,

    MPIDI_CH4_NUM_MT_MODELS,
};

/* For now any thread safety model that is not "direct" requires
   work queues. These queues might be used for different reasons,
   thus a new macro to capture that. */
#if !defined(MPIDI_CH4_USE_MT_DIRECT)
#define MPIDI_CH4_USE_WORK_QUEUES
#endif

/*
  Work queue implementations
  We can't use enum here because we want to use these values in #if macros
*/
#define MPIDI_CH4_WORKQ_ZM_MSQUEUE 0
#define MPIDI_CH4_WORKQ_ZM_GLQUEUE 1
#define MPIDI_CH4_NUM_WORKQ_TYPES  2

static const char *MPIDI_CH4_mt_model_names[MPIDI_CH4_NUM_MT_MODELS] = {
    "direct",
    "handoff",
    "trylock",
};

enum MPIDI_workq_op { SEND, ISEND, SSEND, ISSEND, RSEND, IRSEND, RECV, IRECV, PUT, IPROBE,
     IMPROBE
};


#if defined(MPIDI_CH4_USE_WORK_QUEUES)

#include <queue/zm_msqueue.h>
#include <queue/zm_glqueue.h>

#define MPIDI_CH4_WORKQ_EXTERNAL_DATA_SIZE 768

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct MPIDI_workq {
        /* TODO: we can cut off unused members in case of compile time build */
        union {
            zm_msqueue_t zm_msqueue;
            zm_glqueue_t zm_glqueue;
            char external_data[MPIDI_CH4_WORKQ_EXTERNAL_DATA_SIZE];
        };

#ifdef MPIDI_CH4_USE_WORKQ_RUNTIME
        void (*enqueue) (struct MPIDI_workq * q, void *p);
        int (*dequeue_bulk) (struct MPIDI_workq * q, void **p, int n);
        void (*finalize) (struct MPIDI_workq * q);
#endif
    } MPIDI_workq_t;

    static const char *MPIDI_workq_types[] = {
        "zm_msqueue",
        "zm_glqueue",
    };

    typedef enum MPIDI_workq_op MPIDI_workq_op_t;

    typedef struct MPIDI_workq_elemt MPIDI_workq_elemt_t;
    typedef struct MPIDI_workq_list MPIDI_workq_list_t;

    typedef struct MPIDI_av_entry MPIDI_av_entry_t;

#ifndef NO_WORKQ_ELEMENT_DECLARATION

    struct MPIDI_workq_elemt {
        volatile struct MPIDI_workq_elemt *next;
        MPIDI_workq_op_t op;
        union {
            /* Point-to-Point */
            struct {
                const void *send_buf;
                void *recv_buf;
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *pt2pt_addr;
                MPI_Status *status;
                MPIR_Request *request;
                int *flag;      /* needed for the probe routines */
                MPIR_Request **message; /* used for mprobe */
                OPA_int_t *processed;   /* set to true by the progress thread when
                                         * this work item is done */
            };
            /* RMA */
            struct {
                const void *origin_addr;
                int origin_count;
                MPI_Datatype origin_datatype;
                int target_rank;
                MPI_Aint target_disp;
                int target_count;
                MPI_Datatype target_datatype;
                MPIR_Win *win_ptr;
                MPIDI_av_entry_t *rma_addr;
            };
        };
    };

    struct MPIDI_workq_list {
        MPIDI_workq_t pend_ops;
        MPIDI_workq_list_t *next, *prev;
    };

#endif /* NO_WORKQ_ELEMENT_DECLARATION */

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif /* MPIDI_CH4_USE_WORK_QUEUES */

#endif /* CH4I_WORKQ_TYPES_H_INCLUDED */

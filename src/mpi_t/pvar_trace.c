/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if defined (MPIR_PVAR_SAMPLE_TRACING)

#define MPIR_CVAR_TRACE_BUFFER_LENGTH 1000000
#define MPIR_CVAR_PVAR_TRACE_INTERVAL_DEFAULT 1000000
FILE *PVAR_TRACE_fd;

timer_data_t        PVAR_TRACE_timer;
long long           PVAR_TRACE_interval;
unsigned            PVAR_TRACE_index;

double             *PVAR_TRACE_timestamps;
unsigned long long *PVAR_TRACE_recv_issued;
unsigned           *PVAR_TRACE_posted_recvq_length;
unsigned           *PVAR_TRACE_unexpected_recvq_length;
unsigned long long *PVAR_TRACE_posted_recvq_match_attempts;
unsigned long long *PVAR_TRACE_unexpected_recvq_match_attempts;
unsigned long long *PVAR_TRACE_req_created;
unsigned long long *PVAR_TRACE_req_completed;
unsigned long long *PVAR_TRACE_req_freed;
FILE               *PVAR_TRACE_fd;

static void PVAR_TRACE_dump_trace() {
        unsigned i;
        for( i=0; i< PVAR_TRACE_index; i++) {
            fprintf(PVAR_TRACE_fd, "%.6f \t ", PVAR_TRACE_timestamps[i]);
            if(ENABLE_PVAR_RECVQ) {
                fprintf(PVAR_TRACE_fd, "%llu \t ", PVAR_TRACE_recv_issued[i]);
                fprintf(PVAR_TRACE_fd, "%u \t ",   PVAR_TRACE_posted_recvq_length[i]);
                fprintf(PVAR_TRACE_fd, "%u \t ",   PVAR_TRACE_unexpected_recvq_length[i]);
                fprintf(PVAR_TRACE_fd, "%llu \t ", PVAR_TRACE_posted_recvq_match_attempts[i]);
                fprintf(PVAR_TRACE_fd, "%llu \t ", PVAR_TRACE_unexpected_recvq_match_attempts[i]);
            }
            if(ENABLE_PVAR_REQUEST) {
                fprintf(PVAR_TRACE_fd, "%llu \t ", PVAR_TRACE_req_created[i]);
                fprintf(PVAR_TRACE_fd, "%llu \t ", PVAR_TRACE_req_completed[i]);
                fprintf(PVAR_TRACE_fd, "%llu \n",  PVAR_TRACE_req_freed[i]);
            }
        }
        memset(PVAR_TRACE_timestamps,            0.0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(double));
        if(ENABLE_PVAR_RECVQ) {
            memset(PVAR_TRACE_recv_issued,             0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long));
            memset(PVAR_TRACE_posted_recvq_length,     0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned));
            memset(PVAR_TRACE_unexpected_recvq_length, 0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned));
            memset(PVAR_TRACE_posted_recvq_match_attempts,     0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long long));
            memset(PVAR_TRACE_unexpected_recvq_match_attempts, 0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long long));
        }
        if(ENABLE_PVAR_REQUEST) {
            memset(PVAR_TRACE_req_created,             0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long));
            memset(PVAR_TRACE_req_completed,           0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long));
            memset(PVAR_TRACE_req_freed,               0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(unsigned long));
        }
        PVAR_TRACE_index = 0;
}

void PVAR_TRACE_timer_handler(int sig, siginfo_t *si, void *uc) {

    int i;
    double timestamp = PMPI_Wtime();

    if(PVAR_TRACE_index + 1 < MPIR_CVAR_TRACE_BUFFER_LENGTH) {
        PVAR_TRACE_timestamps[PVAR_TRACE_index]                            = timestamp;
        if(ENABLE_PVAR_RECVQ) {
            PVAR_TRACE_recv_issued[PVAR_TRACE_index]                           = MPIR_T_get_recv_issued();
            PVAR_TRACE_posted_recvq_length[PVAR_TRACE_index]                   = MPIR_T_get_posted_recvq_length();
            PVAR_TRACE_unexpected_recvq_length[PVAR_TRACE_index]               = MPIR_T_get_unexpected_recvq_length();
            PVAR_TRACE_posted_recvq_match_attempts[PVAR_TRACE_index]           = MPIR_T_get_posted_recvq_match_attempts();
            PVAR_TRACE_unexpected_recvq_match_attempts[PVAR_TRACE_index]       = MPIR_T_get_unexpected_recvq_match_attempts();
        }
        if(ENABLE_PVAR_REQUEST) {
            PVAR_TRACE_req_created[PVAR_TRACE_index]                           = MPIR_T_get_req_created();
            PVAR_TRACE_req_completed[PVAR_TRACE_index]                         = MPIR_T_get_req_complet();
            PVAR_TRACE_req_freed[PVAR_TRACE_index]                             = MPIR_T_get_req_freed();
        }
        PVAR_TRACE_index++;
    }
    else {
        /* Dump current trace and reset everything */
        PVAR_TRACE_dump_trace();
    }

}

void MPIR_T_init_trace() {
    char *s;
    s = getenv("MPIR_CVAR_PVAR_TRACE_INTERVAL");
    if(s) PVAR_TRACE_interval = atoll(s);
    else  PVAR_TRACE_interval = MPIR_CVAR_PVAR_TRACE_INTERVAL_DEFAULT;
    PVAR_TRACE_timestamps     = (double*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(double));
    if(ENABLE_PVAR_RECVQ) {
        PVAR_TRACE_recv_issued   = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
        PVAR_TRACE_posted_recvq_length     = (unsigned*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned));
        PVAR_TRACE_unexpected_recvq_length = (unsigned*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned));
        PVAR_TRACE_posted_recvq_match_attempts     = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
        PVAR_TRACE_unexpected_recvq_match_attempts = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
    }
    if(ENABLE_PVAR_REQUEST) {
        PVAR_TRACE_req_created   = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
        PVAR_TRACE_req_completed = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
        PVAR_TRACE_req_freed     = (unsigned long long*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(unsigned long long));
    }
    PVAR_TRACE_index = 0;
    int my_rank = MPIR_Process.comm_world->rank;
    int my_pid = getpid();
    char filename[30];
    sprintf(filename, "%d_%d.trace", my_rank, my_pid);
    PVAR_TRACE_fd = fopen(filename, "w");
    fprintf(PVAR_TRACE_fd, "Timestamp \t Recv_Issued \t PostedQ_Length \t UnexpectedQ_Length \t PostedQ_Match_Attempts \t UnexpectedQ_Match_Attempts \t Created_Requests \t Completed_Requests \t Freed_Requests\n");
    start_sampling(&PVAR_TRACE_timer, PVAR_TRACE_interval, PVAR_TRACE_timer_handler);
}

void PVAR_TRACE_finalize() {
    PVAR_TRACE_dump_trace();
    free(PVAR_TRACE_timestamps);
    if(ENABLE_PVAR_RECVQ) {
        free(PVAR_TRACE_recv_issued);
        free(PVAR_TRACE_posted_recvq_length);
        free(PVAR_TRACE_unexpected_recvq_length);
        free(PVAR_TRACE_posted_recvq_match_attempts);
        free(PVAR_TRACE_unexpected_recvq_match_attempts);
    }
    if(ENABLE_PVAR_REQUEST) {
        free(PVAR_TRACE_req_created);
        free(PVAR_TRACE_req_completed);
        free(PVAR_TRACE_req_freed);
    }
    fclose(PVAR_TRACE_fd);
}

void MPIR_T_stop_trace() {
      stop_sampling(&PVAR_TRACE_timer, PVAR_TRACE_finalize);
}

#else

void MPIR_T_init_trace() {
    return;
}

void MPIR_T_stop_trace() {
    return;
}

#endif


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
double              PVAR_TRACE_time_prev;

struct PVAR_TRACE_entry {
    double             timestamp;
#if ENABLE_PVAR_RECVQ
    unsigned long long recv_issued;
    unsigned           posted_recvq_length;
    unsigned           unexpected_recvq_length;
    unsigned long long posted_recvq_match_attempts;
    unsigned long long unexpected_recvq_match_attempts;
#endif
#if ENABLE_PVAR_REQUEST
    unsigned long long req_created;
    unsigned long long req_completed;
    unsigned long long req_freed;
#endif
};

struct PVAR_TRACE_entry old_entry = {0};
struct PVAR_TRACE_entry cur_entry;

struct PVAR_TRACE_entry *PVAR_TRACE_buffer;
FILE               *PVAR_TRACE_fd;

static void PVAR_TRACE_dump_trace() {
        unsigned i;
        for( i=0; i< PVAR_TRACE_index; i++) {
            fprintf(PVAR_TRACE_fd, "%.3f", 1e3*PVAR_TRACE_buffer[i].timestamp);
#if ENABLE_PVAR_RECVQ
            fprintf(PVAR_TRACE_fd, ",%llu", PVAR_TRACE_buffer[i].recv_issued);
            fprintf(PVAR_TRACE_fd, ",%u",   PVAR_TRACE_buffer[i].posted_recvq_length);
            fprintf(PVAR_TRACE_fd, ",%u",   PVAR_TRACE_buffer[i].unexpected_recvq_length);
            fprintf(PVAR_TRACE_fd, ",%llu", PVAR_TRACE_buffer[i].posted_recvq_match_attempts);
            fprintf(PVAR_TRACE_fd, ",%llu", PVAR_TRACE_buffer[i].unexpected_recvq_match_attempts);
#endif
#if ENABLE_PVAR_REQUEST
            fprintf(PVAR_TRACE_fd, ",%llu", PVAR_TRACE_buffer[i].req_created);
            fprintf(PVAR_TRACE_fd, ",%llu", PVAR_TRACE_buffer[i].req_completed);
            fprintf(PVAR_TRACE_fd, ",%llu",  PVAR_TRACE_buffer[i].req_freed);
#endif
            fprintf(PVAR_TRACE_fd, "\n");
        }
        memset(PVAR_TRACE_buffer,            0.0, MPIR_CVAR_TRACE_BUFFER_LENGTH*sizeof(struct PVAR_TRACE_entry));
        PVAR_TRACE_index = 0;
}

void PVAR_TRACE_timer_handler(int sig, siginfo_t *si, void *uc) {

    double timestamp = PMPI_Wtime();
    double duration = timestamp - PVAR_TRACE_time_prev;
    PVAR_TRACE_time_prev = timestamp;

    if(PVAR_TRACE_index + 1 < MPIR_CVAR_TRACE_BUFFER_LENGTH) {
        PVAR_TRACE_buffer[PVAR_TRACE_index].timestamp = duration;
#if ENABLE_PVAR_RECVQ
        cur_entry.recv_issued                           = MPIR_T_get_recv_issued();
        cur_entry.posted_recvq_length                   = MPIR_T_get_posted_recvq_length();
        cur_entry.unexpected_recvq_length               = MPIR_T_get_unexpected_recvq_length();
        cur_entry.posted_recvq_match_attempts           = MPIR_T_get_posted_recvq_match_attempts();
        cur_entry.unexpected_recvq_match_attempts       = MPIR_T_get_unexpected_recvq_match_attempts();
        PVAR_TRACE_buffer[PVAR_TRACE_index].recv_issued                     = cur_entry.recv_issued                     - old_entry.recv_issued;
        PVAR_TRACE_buffer[PVAR_TRACE_index].posted_recvq_length             = cur_entry.posted_recvq_length             - old_entry.posted_recvq_length;
        PVAR_TRACE_buffer[PVAR_TRACE_index].unexpected_recvq_length         = cur_entry.unexpected_recvq_length         - old_entry.unexpected_recvq_length;
        PVAR_TRACE_buffer[PVAR_TRACE_index].posted_recvq_match_attempts     = cur_entry.posted_recvq_match_attempts     - old_entry.posted_recvq_match_attempts;
        PVAR_TRACE_buffer[PVAR_TRACE_index].unexpected_recvq_match_attempts = cur_entry.unexpected_recvq_match_attempts - old_entry.unexpected_recvq_match_attempts;

#endif
#if ENABLE_PVAR_REQUEST
        cur_entry.req_created                           = MPIR_T_get_req_created();
        cur_entry.req_completed                         = MPIR_T_get_req_complet();
        cur_entry.req_freed                             = MPIR_T_get_req_freed();
        PVAR_TRACE_buffer[PVAR_TRACE_index].req_created   = cur_entry.req_created   - old_entry.req_created;
        PVAR_TRACE_buffer[PVAR_TRACE_index].req_completed = cur_entry.req_completed - old_entry.req_completed;
        PVAR_TRACE_buffer[PVAR_TRACE_index].req_freed     = cur_entry.req_freed     - old_entry.req_freed;
#endif
        memcpy(&old_entry, &cur_entry, sizeof(struct PVAR_TRACE_entry));
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
    PVAR_TRACE_buffer     = (struct PVAR_TRACE_entry*) calloc(MPIR_CVAR_TRACE_BUFFER_LENGTH, sizeof(struct PVAR_TRACE_entry));
    PVAR_TRACE_index = 0;
    int my_rank = MPIR_Process.comm_world->rank;
    int my_pid = getpid();
    char filename[30];
    sprintf(filename, "%d_%d.csv", my_rank, my_pid);
    PVAR_TRACE_fd = fopen(filename, "w");
    fprintf(PVAR_TRACE_fd, "time");
    if(ENABLE_PVAR_RECVQ)
        fprintf(PVAR_TRACE_fd, ",recv_issued,postedq_len,unexpq_len,postedq_match_atmpts,unexpq_match_atmpts");
    if(ENABLE_PVAR_REQUEST)
        fprintf(PVAR_TRACE_fd, ",created_reqs,completed_reqs,freed_reqs");
    fprintf(PVAR_TRACE_fd, "\n");
    PVAR_TRACE_time_prev = PMPI_Wtime() ;
    start_sampling(&PVAR_TRACE_timer, PVAR_TRACE_interval, PVAR_TRACE_timer_handler);
}

void PVAR_TRACE_finalize() {
    PVAR_TRACE_dump_trace();
    free(PVAR_TRACE_buffer);
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


//
// Created by Bolun Li on 9/21/21.
//

#ifndef _THREAD_DATA_H
#define _THREAD_DATA_H

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include "profiler_support.h"

namespace ThreadData {

typedef struct thread_data_t {
    pid_t tid;
    bool inside_agent;
    sigjmp_buf jbuf;
    void *perf_state;
    void *context_state;
#ifndef COUNT_OVERHEAD
    void *output_state;
    void *output_state_trace;
    void *output_state_alloc;
    void *output_state_cachemiss;
#endif
    void *ctxt_sample_state[4]; // only have 4 debug registers
#ifdef PRINT_PMU_INS
    void *pmu_ins_output_stream;
#endif
} thread_data_t;


void thread_data_init();

thread_data_t *thread_data_alloc();

thread_data_t *thread_data_get();

void thread_data_dealloc(std::string clientName);

void thread_data_shutdown();

}

#define TD_GET(field) ThreadData::thread_data_get()->field

#endif //_THREAD_DATA_H

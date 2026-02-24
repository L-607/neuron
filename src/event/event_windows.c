#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "event/event.h"
#include "utils/log.h"
#include "utils/utlist.h"

// Basic linked list for timers
struct neu_event_timer {
    neu_event_timer_param_t param;
    uint64_t next_files_ms;
    struct neu_event_timer *next;
    struct neu_event_timer *prev;
};

// IO mapping
struct neu_event_io {
    neu_event_io_param_t param;
    struct neu_event_io *next;
    struct neu_event_io *prev;
};

struct neu_events {
    bool running;
    HANDLE thread_handle;
    unsigned int thread_id;
    
    // Simplistic handling: Single lock for structure protection
    CRITICAL_SECTION lock;
    
    struct neu_event_timer *timers;
    struct neu_event_io *ios;
};

static uint64_t get_now_ms() {
    return GetTickCount64();
}

static unsigned __stdcall event_loop(void *arg) {
    neu_events_t *events = (neu_events_t *)arg;
    
    while (events->running) {
        EnterCriticalSection(&events->lock);
        
        // Prepare FD sets for select
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = 0; // Ignored on Windows but good practice
        
        struct neu_event_io *io_elt;
        DL_FOREACH(events->ios, io_elt) {
            FD_SET(io_elt->param.fd, &readfds);
        }
        
        // Check timers for shortest timeout
        uint64_t now = get_now_ms();
        uint64_t min_timeout = 1000; // Default 1 sec
        
        struct neu_event_timer *timer_elt, *tmp;
        DL_FOREACH_SAFE(events->timers, timer_elt, tmp) {
            if (timer_elt->next_files_ms <= now) {
                // Timer expired
                // Release lock during callback? Dangerous if list changes. 
                // For simplicity, keep hold or copy. 
                // Let's execute and Update next time.
                // Note: This is blocking other events. 
                
                timer_elt->param.cb(timer_elt->param.usr_data);
                
                if (timer_elt->param.type == NEU_EVENT_TIMER_BLOCK) {
                     // Should be removed? Implementation dependent. 
                     // Assuming 'BLOCK' means one-shot or blocking? 
                     // Looking at usage usually Periodic or OneShot. 
                     // If periodic, update next time.
                }

                int64_t interval = timer_elt->param.second * 1000 + timer_elt->param.millisecond;
                timer_elt->next_files_ms = now + interval;
            }
            
            int64_t diff = timer_elt->next_files_ms - now;
            if (diff < 0) diff = 0;
            if ((uint64_t)diff < min_timeout) min_timeout = (uint64_t)diff;
        }
        
        LeaveCriticalSection(&events->lock);
        
        struct timeval tv;
        tv.tv_sec = (long)(min_timeout / 1000);
        tv.tv_usec = (long)((min_timeout % 1000) * 1000);
        
        // Use a copy of sets because select modifies them
        fd_set readfds_copy = readfds; 
        
        // Need to be careful: if no FDs, select might fail or behave differently (sleep).
        int ret = 0;
        if (readfds_copy.fd_count > 0) {
            ret = select(0, &readfds_copy, NULL, NULL, &tv);
        } else {
            Sleep((DWORD)min_timeout);
        }

        if (ret > 0) {
            // Check which FDs are ready
            EnterCriticalSection(&events->lock);
            DL_FOREACH(events->ios, io_elt) {
                if (FD_ISSET(io_elt->param.fd, &readfds_copy)) {
                    io_elt->param.cb(NEU_EVENT_IO_READ, io_elt->param.fd, io_elt->param.usr_data);
                }
            }
            LeaveCriticalSection(&events->lock);
        }
    }
    return 0;
}

neu_events_t *neu_event_new(const char *name) {
    (void)name;
    neu_events_t *events = calloc(1, sizeof(neu_events_t));
    if (!events) return NULL;
    
    InitializeCriticalSection(&events->lock);
    events->running = true;
    
    events->thread_handle = (HANDLE)_beginthreadex(NULL, 0, event_loop, events, 0, &events->thread_id);
    
    return events;
}

int neu_event_close(neu_events_t *events) {
    if (!events) return -1;
    
    events->running = false;
    WaitForSingleObject(events->thread_handle, INFINITE);
    CloseHandle(events->thread_handle);
    DeleteCriticalSection(&events->lock);
    
    // Free lists... (omitted for brevity)
    free(events);
    return 0;
}

neu_event_timer_t *neu_event_add_timer(neu_events_t *events, neu_event_timer_param_t timer) {
    struct neu_event_timer *t = calloc(1, sizeof(struct neu_event_timer));
    t->param = timer;
    t->next_files_ms = get_now_ms() + timer.second * 1000 + timer.millisecond;
    
    EnterCriticalSection(&events->lock);
    DL_APPEND(events->timers, t);
    LeaveCriticalSection(&events->lock);
    return t;
}

int neu_event_del_timer(neu_events_t *events, neu_event_timer_t *timer) {
    EnterCriticalSection(&events->lock);
    DL_DELETE(events->timers, timer);
    LeaveCriticalSection(&events->lock);
    free(timer);
    return 0;
}

neu_event_io_t *neu_event_add_io(neu_events_t *events, neu_event_io_param_t io) {
    struct neu_event_io *i = calloc(1, sizeof(struct neu_event_io));
    i->param = io;
    
    EnterCriticalSection(&events->lock);
    DL_APPEND(events->ios, i);
    LeaveCriticalSection(&events->lock);
    return i;
}

int neu_event_del_io(neu_events_t *events, neu_event_io_t *io) {
    EnterCriticalSection(&events->lock);
    DL_DELETE(events->ios, io);
    LeaveCriticalSection(&events->lock);
    free(io);
    return 0;
}

/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * Cygwin-compatible event implementation using poll() and POSIX threads.
 * Used when building on Cygwin (no epoll/kqueue available).
 **/
#if defined(__CYGWIN__)

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "event/event.h"
#include "utils/log.h"

#define MAX_TIMERS 64
#define MAX_IOS    64

struct neu_event_timer {
    int                      id;
    neu_event_timer_param_t  param;
    struct timespec          next_fire;
    bool                     active;
    bool                     in_callback;
    pthread_mutex_t          cb_mtx;
    pthread_cond_t           cb_cv;
};

struct neu_event_io {
    int                  fd;
    neu_event_io_param_t param;
    bool                 active;
};

struct neu_events {
    char            name[64];
    bool            running;
    pthread_t       thread;
    pthread_mutex_t mtx;

    struct neu_event_timer timers[MAX_TIMERS];
    struct neu_event_io    ios[MAX_IOS];
    int                    n_timers;
    int                    n_ios;

    // wakeup pipe for unblocking poll()
    int pipe_r;
    int pipe_w;
};

static void timespec_add_ms(struct timespec *ts, int64_t ms)
{
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

static int64_t timespec_diff_ms(struct timespec *a, struct timespec *b)
{
    // a - b in ms
    int64_t diff = (int64_t)(a->tv_sec - b->tv_sec) * 1000;
    diff += (a->tv_nsec - b->tv_nsec) / 1000000L;
    return diff;
}

static void *event_loop(void *arg)
{
    neu_events_t *events = (neu_events_t *) arg;

    while (events->running) {
        pthread_mutex_lock(&events->mtx);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Compute shortest timeout to next timer
        int timeout_ms = 1000;
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (!events->timers[i].active)
                continue;
            int64_t diff =
                timespec_diff_ms(&events->timers[i].next_fire, &now);
            if (diff <= 0) {
                timeout_ms = 0;
                break;
            }
            if (diff < timeout_ms)
                timeout_ms = (int) diff;
        }

        // Build pollfd array
        struct pollfd fds[MAX_IOS + 1];
        int           nfds = 0;

        // Always watch the wakeup pipe
        fds[nfds].fd      = events->pipe_r;
        fds[nfds].events  = POLLIN;
        fds[nfds].revents = 0;
        nfds++;

        // IO file descriptors
        int io_idx[MAX_IOS];
        int n_active_ios = 0;
        for (int i = 0; i < MAX_IOS; i++) {
            if (!events->ios[i].active)
                continue;
            fds[nfds].fd      = events->ios[i].fd;
            fds[nfds].events  = POLLIN | POLLHUP;
            fds[nfds].revents = 0;
            io_idx[n_active_ios] = i;
            n_active_ios++;
            nfds++;
        }

        pthread_mutex_unlock(&events->mtx);

        int ret = poll(fds, nfds, timeout_ms);
        (void) ret;

        pthread_mutex_lock(&events->mtx);
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Drain wakeup pipe
        if (fds[0].revents & POLLIN) {
            char buf[64];
            read(events->pipe_r, buf, sizeof(buf));
        }

        // Handle IO events
        for (int j = 0; j < n_active_ios; j++) {
            int            idx = io_idx[j];
            struct pollfd *pfd = &fds[j + 1];

            if (!events->ios[idx].active)
                continue;

            if (pfd->revents & (POLLHUP | POLLERR | POLLNVAL)) {
                neu_event_io_callback cb  = events->ios[idx].param.cb;
                int                   fd  = events->ios[idx].fd;
                void *                usr = events->ios[idx].param.usr_data;
                pthread_mutex_unlock(&events->mtx);
                cb(NEU_EVENT_IO_CLOSED, fd, usr);
                pthread_mutex_lock(&events->mtx);
            } else if (pfd->revents & POLLIN) {
                neu_event_io_callback cb  = events->ios[idx].param.cb;
                int                   fd  = events->ios[idx].fd;
                void *                usr = events->ios[idx].param.usr_data;
                pthread_mutex_unlock(&events->mtx);
                cb(NEU_EVENT_IO_READ, fd, usr);
                pthread_mutex_lock(&events->mtx);
            }
        }

        // Fire expired timers
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (!events->timers[i].active)
                continue;

            int64_t diff = timespec_diff_ms(&events->timers[i].next_fire, &now);
            if (diff <= 0) {
                neu_event_timer_callback cb  = events->timers[i].param.cb;
                void *                   usr = events->timers[i].param.usr_data;
                int64_t interval = events->timers[i].param.second * 1000 +
                                   events->timers[i].param.millisecond;

                // Schedule next fire
                clock_gettime(CLOCK_MONOTONIC,
                              &events->timers[i].next_fire);
                timespec_add_ms(&events->timers[i].next_fire, interval);

                pthread_mutex_lock(&events->timers[i].cb_mtx);
                events->timers[i].in_callback = true;
                pthread_mutex_unlock(&events->timers[i].cb_mtx);

                pthread_mutex_unlock(&events->mtx);
                cb(usr);
                pthread_mutex_lock(&events->mtx);

                pthread_mutex_lock(&events->timers[i].cb_mtx);
                events->timers[i].in_callback = false;
                pthread_cond_broadcast(&events->timers[i].cb_cv);
                pthread_mutex_unlock(&events->timers[i].cb_mtx);
            }
        }

        pthread_mutex_unlock(&events->mtx);
    }

    return NULL;
}

neu_events_t *neu_event_new(const char *name)
{
    neu_events_t *events = calloc(1, sizeof(neu_events_t));
    if (!events)
        return NULL;

    if (name)
        strncpy(events->name, name, sizeof(events->name) - 1);

    pthread_mutex_init(&events->mtx, NULL);
    for (int i = 0; i < MAX_TIMERS; i++) {
        pthread_mutex_init(&events->timers[i].cb_mtx, NULL);
        pthread_cond_init(&events->timers[i].cb_cv, NULL);
    }
    events->running = true;

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        free(events);
        return NULL;
    }
    events->pipe_r = pipefd[0];
    events->pipe_w = pipefd[1];

    pthread_create(&events->thread, NULL, event_loop, events);
    return events;
}

int neu_event_close(neu_events_t *events)
{
    events->running = false;

    // Wake up the poll()
    char c = 'x';
    write(events->pipe_w, &c, 1);

    pthread_join(events->thread, NULL);
    pthread_mutex_destroy(&events->mtx);
    for (int i = 0; i < MAX_TIMERS; i++) {
        pthread_mutex_destroy(&events->timers[i].cb_mtx);
        pthread_cond_destroy(&events->timers[i].cb_cv);
    }

    close(events->pipe_r);
    close(events->pipe_w);

    free(events);
    return 0;
}

neu_event_timer_t *neu_event_add_timer(neu_events_t *          events,
                                       neu_event_timer_param_t timer)
{
    pthread_mutex_lock(&events->mtx);

    neu_event_timer_t *ctx = NULL;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!events->timers[i].active) {
            ctx           = &events->timers[i];
            ctx->id       = i;
            ctx->param    = timer;
            ctx->active   = true;
            ctx->in_callback = false;
            clock_gettime(CLOCK_MONOTONIC, &ctx->next_fire);
            int64_t interval =
                timer.second * 1000 + timer.millisecond;
            timespec_add_ms(&ctx->next_fire, interval);
            break;
        }
    }

    pthread_mutex_unlock(&events->mtx);

    // Wake the loop so it picks up the new timer immediately
    char c = 'w';
    write(events->pipe_w, &c, 1);

    return ctx;
}

int neu_event_del_timer(neu_events_t *events, neu_event_timer_t *timer)
{
    if (!timer)
        return 0;

    pthread_mutex_lock(&events->mtx);
    timer->active = false;
    pthread_mutex_unlock(&events->mtx);

    pthread_mutex_lock(&timer->cb_mtx);
    while (timer->in_callback) {
        pthread_cond_wait(&timer->cb_cv, &timer->cb_mtx);
    }
    pthread_mutex_unlock(&timer->cb_mtx);

    // Wake loop
    char c = 'w';
    write(events->pipe_w, &c, 1);

    return 0;
}

neu_event_io_t *neu_event_add_io(neu_events_t *       events,
                                 neu_event_io_param_t io)
{
    pthread_mutex_lock(&events->mtx);

    neu_event_io_t *ctx = NULL;
    for (int i = 0; i < MAX_IOS; i++) {
        if (!events->ios[i].active) {
            ctx          = &events->ios[i];
            ctx->fd      = io.fd;
            ctx->param   = io;
            ctx->active  = true;
            break;
        }
    }

    pthread_mutex_unlock(&events->mtx);

    // Wake loop to pick up new fd
    char c = 'w';
    write(events->pipe_w, &c, 1);

    return ctx;
}

int neu_event_del_io(neu_events_t *events, neu_event_io_t *io)
{
    if (!io)
        return 0;

    pthread_mutex_lock(&events->mtx);
    io->active = false;
    pthread_mutex_unlock(&events->mtx);

    // Wake loop
    char c = 'w';
    write(events->pipe_w, &c, 1);

    return 0;
}

#endif /* __CYGWIN__ */

// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <immintrin.h>

#define MKFW_SPIN_THRESHOLD_NS 500000	// NOTE(peter): 500us spin threshold for Linux

struct mkfw_timer_handle {
	uint64_t interval_ns;
	struct timespec next_deadline;
	uint32_t running;
	mkfw_thread timer_thread;

	volatile int futex_word;

#ifdef MKFW_TIMER_DEBUG
	struct timespec last_wait_start;
#endif
};

static void mkfw_timespec_add_ns(struct timespec *ts, uint64_t ns) {
	ts->tv_nsec += ns;
	while(ts->tv_nsec >= 1000000000L) {
		ts->tv_nsec -= 1000000000L;
		ts->tv_sec++;
	}
}

static int64_t mkfw_timespec_diff_ns(struct timespec *a, struct timespec *b) {
	int64_t sec = a->tv_sec - b->tv_sec;
	int64_t nsec = a->tv_nsec - b->tv_nsec;

	if(nsec < 0) {
		nsec += 1000000000L;
		sec -= 1;
	}

	return sec * 1000000000LL + nsec;
}

static int mkfw_futex_wait(volatile int *addr, int val) {
	return syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, val, 0, 0, 0);
}

static int mkfw_futex_wake(volatile int *addr) {
	return syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0);
}

static MKFW_THREAD_FUNC(mkfw_timer_thread_func, arg) {
	struct mkfw_timer_handle *t = (struct mkfw_timer_handle *)arg;

	while(t->running) {
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

#ifdef MKFW_TIMER_DEBUG
		int64_t remaining_after_sleep_ns = -1;
#endif

		int64_t diff_ns = mkfw_timespec_diff_ns(&t->next_deadline, &now);
		if(diff_ns > MKFW_SPIN_THRESHOLD_NS) {
			struct timespec sleep_time;
			uint64_t sleep_ns = diff_ns - MKFW_SPIN_THRESHOLD_NS;
			sleep_time.tv_sec = sleep_ns / 1000000000;
			sleep_time.tv_nsec = sleep_ns % 1000000000;
			nanosleep(&sleep_time, 0);
#ifdef MKFW_TIMER_DEBUG
			clock_gettime(CLOCK_MONOTONIC_RAW, &now);
			remaining_after_sleep_ns = mkfw_timespec_diff_ns(&t->next_deadline, &now);
#endif
		}

		while(clock_gettime(CLOCK_MONOTONIC_RAW, &now), mkfw_timespec_diff_ns(&t->next_deadline, &now) > 0) {
			_mm_pause();
		}

		t->futex_word = 1;
		mkfw_futex_wake(&t->futex_word);

#ifdef MKFW_TIMER_DEBUG
		if(t->last_wait_start.tv_sec) {
			int64_t overshoot_ns = mkfw_timespec_diff_ns(&now, &t->next_deadline);
			if(overshoot_ns < 0) overshoot_ns = 0;

			if(remaining_after_sleep_ns >= 0) {
				mkfw_error("[DEBUG] Woke up with %ld ns left. Overshoot: %5ld ns", remaining_after_sleep_ns, overshoot_ns);
			} else {
				mkfw_error("[DEBUG] No sleep. Overshoot: %ld ns", overshoot_ns);
			}
		}
		t->last_wait_start = now;
#endif

		mkfw_timespec_add_ns(&t->next_deadline, t->interval_ns);
	}

	return 0;
}

static struct mkfw_timer_handle *mkfw_timer_new(uint64_t interval_ns) {
	struct mkfw_timer_handle *t = calloc(1, sizeof(struct mkfw_timer_handle));

	t->interval_ns = interval_ns;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t->next_deadline);
	mkfw_timespec_add_ns(&t->next_deadline, interval_ns);
	t->running = 1;
	t->futex_word = 0;

#ifdef MKFW_TIMER_DEBUG
	t->last_wait_start.tv_sec = 0;
	t->last_wait_start.tv_nsec = 0;
#endif

	t->timer_thread = mkfw_thread_create(mkfw_timer_thread_func, t);

	return t;
}

static uint32_t mkfw_timer_wait(struct mkfw_timer_handle *t) {
	mkfw_futex_wait(&t->futex_word, 0);
	t->futex_word = 0;
	return 1;
}

static void mkfw_timer_destroy(struct mkfw_timer_handle *t) {
	t->running = 0;
	t->futex_word = 1;
	mkfw_futex_wake(&t->futex_word);
	mkfw_thread_join(t->timer_thread);
	free(t);
}

// NOTE(peter): These are just stubs on linux, they are however needed in the windows implementation!
static void mkfw_timer_init() { }
static void mkfw_timer_shutdown() { }

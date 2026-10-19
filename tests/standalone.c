// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#include "../mkfw_audio.h"
#include "../mkfw_timer.h"
#include "../mkfw_thread.h"
#include <string.h>

static uint32_t worker_result;

// [=]===^=[ worker ]===========================================================================[=]
static MKFW_THREAD_FUNC(worker, arg) {
	uint32_t *result = arg;

	if(mkfw_get_last_error() || mkfw_get_last_error_code()) {
		*result = 1;
		return 0;

	}

	mkfw_record_error("worker", -37);

	if(strcmp(mkfw_get_last_error(), "worker") || mkfw_get_last_error_code() != -37) {
		*result = 2;

	}

	return 0;

}

// [=]===^=[ main ]==============================================================================[=]
int main(void) {
	mkfw_record_error("caller", -19);
	mkfw_thread thread = mkfw_thread_create(worker, &worker_result);

	if(thread) {
		mkfw_thread_join(thread);

		if(worker_result || strcmp(mkfw_get_last_error(), "caller") || mkfw_get_last_error_code() != -19) {
			return 1;

		}

		mkfw_clear_last_error();

		if(mkfw_get_last_error() || mkfw_get_last_error_code()) {
			return 2;

		}

		mkfw_timer_init();
		struct mkfw_timer_handle *timer = mkfw_timer_create(1000000);

		if(timer) {
			mkfw_timer_set_spin(timer, 0);
			uint32_t first = mkfw_timer_wait(timer);
			mkfw_timer_set_interval(timer, 2000000);
			uint32_t second = mkfw_timer_wait(timer);
			mkfw_timer_destroy(timer);
			mkfw_timer_shutdown();

			if(!first || !second) {
				return 3;

			}

			return 0;

		}

		mkfw_timer_shutdown();
	}

	return 4;

}

// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef HANDLE mkfw_thread;
#define MKFW_THREAD_FUNC(name, arg) DWORD WINAPI name(LPVOID arg)

// [=]===^=[ mkfw_thread_create ]================================================================[=]
static inline mkfw_thread mkfw_thread_create(LPTHREAD_START_ROUTINE func, void *arg) {
	return CreateThread(0, 0, func, arg, 0, 0);

}

// [=]===^=[ mkfw_thread_join ]==================================================================[=]
static inline void mkfw_thread_join(mkfw_thread thread) {
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);

}
#elif defined(__linux__)
#include <pthread.h>

typedef pthread_t mkfw_thread;
#define MKFW_THREAD_FUNC(name, arg) void *name(void *arg)

// [=]===^=[ mkfw_thread_create ]================================================================[=]
static inline mkfw_thread mkfw_thread_create(void *(*func)(void *), void *arg) {
	pthread_t thread;

	if(pthread_create(&thread, 0, func, arg)) {
		return 0;

	}

	return thread;

}

// [=]===^=[ mkfw_thread_join ]==================================================================[=]
static inline void mkfw_thread_join(mkfw_thread thread) {
	pthread_join(thread, 0);

}
#endif

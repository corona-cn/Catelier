#include "Thread.hpp"

#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>
#include <sched.h>
#include <cstring>

#endif

namespace Catelier::src::foundation::primitive {
    namespace {
        typedef struct ThreadContext {
            void (*entry)(void*);
            void* userData;
        } ThreadContext;

        #ifdef _WIN32

        auto asNative(const Thread* self) -> HANDLE* {
            return (HANDLE*) self->storage;
        }

        auto WINAPI threadEntryBridge(void* parameter) -> DWORD {
            auto* const context = (ThreadContext*) parameter;

            void (*const entry)(void*) = context->entry;
            void* const userData = context->userData;

            free(context);

            entry(userData);

            return 0;
        }

        #else

        auto asNative(const Thread* self) -> pthread_t* {
            return (pthread_t*) self->storage;
        }

        auto threadEntryBridge(void* parameter) -> void* {
            auto* const context = (ThreadContext*) parameter;

            void (*const entry)(void*) = context->entry;
            void* const userData = context->userData;

            free(context);

            entry(userData);

            return nullptr;
        }

        #endif
    }

    auto Thread_init(Thread* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        memset(self->storage, 0, sizeof(self->storage));
        self->valid = true;
        self->started = false;

        return true;
    }
    auto Thread_destroy(Thread* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        if (self->started) {
            #ifdef _WIN32

            CloseHandle(*asNative(self));

            #else

            pthread_detach(*asNative(self));

            #endif

            self->started = false;
        }

        self->valid = false;

        #ifndef NDEBUG

        memset(self->storage, 0xDE, sizeof(self->storage));

        #endif

        return true;
    }

    auto Thread_currentId() -> u64 {
        #ifdef _WIN32

        return (u64) GetCurrentThreadId();

        #else

        const pthread_t self = pthread_self();
        u64 id = 0;
        memcpy(&id, &self, sizeof(pthread_t) < sizeof(u64) ? sizeof(pthread_t) : sizeof(u64));
        return id;

        #endif
    }

    auto Thread_start(Thread* self, void (*const entry)(void*), void* const userData) -> bool {
        if (!self || !self->valid || self->started || !entry) {
            return false;
        }

        auto* const context = (ThreadContext*) malloc(sizeof(ThreadContext));
        if (!context) {
            return false;
        }

        context->entry = entry;
        context->userData = userData;

        #ifdef _WIN32

        const HANDLE handle = CreateThread(nullptr, 0, threadEntryBridge, context, 0, nullptr);
        if (!handle) {
            free(context);
            return false;
        }

        *asNative(self) = handle;

        #else

        pthread_t thread;
        if (pthread_create(&thread, nullptr, threadEntryBridge, context) != 0) {
            free(context);
            return false;
        }

        *asNative(self) = thread;

        #endif

        self->started = true;

        return true;
    }

    auto Thread_join(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        #ifdef _WIN32

        WaitForSingleObject(*asNative(self), INFINITE);
        CloseHandle(*asNative(self));

        #else

        pthread_join(*asNative(self), nullptr);

        #endif

        self->started = false;

        return true;
    }

    auto Thread_detach(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        #ifdef _WIN32

        CloseHandle(*asNative(self));

        #else

        pthread_detach(*asNative(self));

        #endif

        self->started = false;

        return true;
    }

    auto Thread_yield() -> void {
        #ifdef _WIN32

        SwitchToThread();

        #else

        sched_yield();

        #endif
    }
}
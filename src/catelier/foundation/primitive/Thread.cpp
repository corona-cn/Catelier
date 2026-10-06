#include "Thread.hpp"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <sched.h>
#include <cstring>
#endif

namespace Catelier::src::foundation::primitive {
    #ifdef _WIN32
    namespace {
        typedef struct ThreadContext {
            Thread_EntryFunc entry;
            void* userData;
        } ThreadContext;

        auto asNative(const Thread* self) -> HANDLE* {
            return (HANDLE*) self->storage;
        }

        auto __stdcall threadEntryBridge(void* parameter) -> unsigned {
            auto* const context = (ThreadContext*) parameter;

            const Thread_EntryFunc entry = context->entry;
            void* const userData = context->userData;

            free(context);

            entry(userData);

            return 0;
        }
    }

    auto Thread_init(Thread* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        *(asNative(self)) = nullptr;
        self->valid = true;
        self->started = false;

        return true;
    }
    auto Thread_destroy(Thread* self) -> bool {
        if (!self || !self->valid || self->started) {
            return false;
        }

        *(asNative(self)) = nullptr;
        self->valid = false;

        return true;
    }

    auto Thread_start(Thread* self, const Thread_EntryFunc entry, void* userData) -> bool {
        if (!self || !self->valid || self->started || !entry) {
            return false;
        }

        auto* const context = (ThreadContext*) malloc(sizeof(ThreadContext));
        if (!context) {
            return false;
        }

        context->entry = entry;
        context->userData = userData;

        const addr handle = _beginthreadex(nullptr, 0, threadEntryBridge, context, 0, nullptr);
        if (handle == 0) {
            free(context);
            return false;
        }

        *(asNative(self)) = (HANDLE) handle;
        self->started = true;

        return true;
    }
    auto Thread_join(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        const HANDLE handle = *(asNative(self));
        WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);

        *(asNative(self)) = nullptr;
        self->started = false;

        return true;
    }
    auto Thread_detach(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        CloseHandle(*(asNative(self)));

        *(asNative(self)) = nullptr;
        self->started = false;

        return true;
    }

    auto Thread_currentId() -> u64 {
        return (u64) GetCurrentThreadId();
    }
    auto Thread_yield() -> void {
        SwitchToThread();
    }
    #else
    namespace {
        typedef struct ThreadContext {
            Thread_EntryFunc entry;
            void* userData;
        } ThreadContext;

        auto asNative(const Thread* self) -> pthread_t* {
            return (pthread_t*) self->storage;
        }

        auto threadEntryBridge(void* parameter) -> void* {
            auto* const context = (ThreadContext*) parameter;

            const Thread_EntryFunc entry = context->entry;
            void* const userData = context->userData;

            free(context);

            entry(userData);

            return nullptr;
        }
    }

    auto Thread_init(Thread* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        memset(asNative(self), 0, sizeof(pthread_t));
        self->valid = true;
        self->started = false;

        return true;
    }
    auto Thread_destroy(Thread* self) -> bool {
        if (!self || !self->valid || self->started) {
            return false;
        }

        memset(asNative(self), 0, sizeof(pthread_t));
        self->valid = false;

        return true;
    }

    auto Thread_start(Thread* self, const Thread_EntryFunc entry, void* userData) -> bool {
        if (!self || !self->valid || self->started || !entry) {
            return false;
        }

        auto* const context = (ThreadContext*) malloc(sizeof(ThreadContext));
        if (!context) {
            return false;
        }

        context->entry = entry;
        context->userData = userData;

        if (pthread_create(asNative(self), nullptr, threadEntryBridge, context) != 0) {
            free(context);
            return false;
        }

        self->started = true;

        return true;
    }
    auto Thread_join(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        pthread_join(*asNative(self), nullptr);

        self->started = false;

        return true;
    }
    auto Thread_detach(Thread* self) -> bool {
        if (!self || !self->valid || !self->started) {
            return false;
        }

        pthread_detach(*asNative(self));

        self->started = false;

        return true;
    }

    auto Thread_currentId() -> u64 {
        return (u64) pthread_self();
    }
    auto Thread_yield() -> void {
        sched_yield();
    }
    #endif
}
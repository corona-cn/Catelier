#include "ThreadKey.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <cstdlib>

namespace Catelier::src::foundation::primitive {
    #ifdef _WIN32
    namespace {
        typedef struct WindowsFlsValue {
            u32 keyIndex;
            void* userValue;
        } WindowsFlsValue;

        constexpr u32 THREAD_KEY_MAX_INDEX = 2048;

        void (*userCallbacks[THREAD_KEY_MAX_INDEX])(void*) = {};

        CRITICAL_SECTION registryLock;
        INIT_ONCE registryOnce = INIT_ONCE_STATIC_INIT;

        auto CALLBACK initRegistryOnce(PINIT_ONCE, PVOID, PVOID*) -> BOOL {
            InitializeCriticalSection(&registryLock);
            return TRUE;
        }

        auto ensureRegistryInitialized() -> void {
            InitOnceExecuteOnce(&registryOnce, initRegistryOnce, nullptr, nullptr);
        }

        auto NTAPI flsCallbackBridge(const PVOID lpFlsData) -> void {
            auto* const wrapper = (WindowsFlsValue*) lpFlsData;
            if (!wrapper) {
                return;
            }

            const u32 keyIndex  = wrapper->keyIndex;
            void* const userValue = wrapper->userValue;

            void (*userCallback)(void*) = nullptr;
            EnterCriticalSection(&registryLock);
            if (keyIndex < THREAD_KEY_MAX_INDEX) {
                userCallback = userCallbacks[keyIndex];
            }
            LeaveCriticalSection(&registryLock);

            if (userCallback) {
                userCallback(userValue);
            }

            free(wrapper);
        }
    }
    #endif

    #ifdef _WIN32
    auto ThreadKey_init(ThreadKey* self, void (*destructor)(void* value)) -> bool {
        if (!self) {
            return false;
        }

        self->handle = 0;
        self->valid  = false;

        ensureRegistryInitialized();

        const DWORD index = FlsAlloc(flsCallbackBridge);
        if (index == FLS_OUT_OF_INDEXES) {
            return false;
        }

        if ((u32) index >= THREAD_KEY_MAX_INDEX) {
            FlsFree(index);
            return false;
        }

        EnterCriticalSection(&registryLock);
        userCallbacks[index] = destructor;
        LeaveCriticalSection(&registryLock);

        self->handle = (u32) index;
        self->valid = true;

        return true;
    }
    auto ThreadKey_destroy(ThreadKey* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        const DWORD index = (DWORD) self->handle;

        EnterCriticalSection(&registryLock);
        if ((u32) index < THREAD_KEY_MAX_INDEX) {
            userCallbacks[index] = nullptr;
        }
        LeaveCriticalSection(&registryLock);

        FlsFree(index);

        self->handle = 0;
        self->valid = false;

        return true;
    }

    auto ThreadKey_get(const ThreadKey* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const wrapper = (WindowsFlsValue*) FlsGetValue((DWORD) self->handle);
        if (!wrapper) {
            return nullptr;
        }

        return wrapper->userValue;
    }
    auto ThreadKey_set(const ThreadKey* self, void* value) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        const DWORD index = (DWORD) self->handle;

        auto* wrapper = (WindowsFlsValue*) FlsGetValue(index);

        if (value == nullptr) {
            if (wrapper) {
                FlsSetValue(index, nullptr);
                free(wrapper);
            }
            return true;
        }

        if (wrapper) {
            wrapper->userValue = value;
            return true;
        }

        wrapper = (WindowsFlsValue*) malloc(sizeof(WindowsFlsValue));
        if (!wrapper) {
            return false;
        }

        wrapper->keyIndex = (u32) index;
        wrapper->userValue = value;

        if (!FlsSetValue(index, wrapper)) {
            free(wrapper);
            return false;
        }

        return true;
    }
    #else
    auto ThreadKey_init(ThreadKey* self, void (*destructor)(void* value)) -> bool {
        if (!self) {
            return false;
        }

        self->handle = 0;
        self->valid = false;

        pthread_key_t key;
        if (pthread_key_create(&key, destructor) != 0) {
            return false;
        }

        self->handle = (u32) key;
        self->valid = true;

        return true;
    }
    auto ThreadKey_destroy(ThreadKey* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        pthread_key_delete((pthread_key_t) self->handle);

        self->handle = 0;
        self->valid = false;

        return true;
    }

    auto ThreadKey_get(const ThreadKey* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        return pthread_getspecific((pthread_key_t) self->handle);
    }
    auto ThreadKey_set(const ThreadKey* self, void* value) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_setspecific((pthread_key_t) self->handle, value) == 0;
    }
    #endif
}
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
            u64 keyGeneration;
            void* userValue;
        } WindowsFlsValue;

        constexpr u32 THREAD_KEY_MAX_INDEX = 2048;

        void (*userCallbacks[THREAD_KEY_MAX_INDEX])(void*) = {};
        u64 keyGenerations[THREAD_KEY_MAX_INDEX] = {};
        u64 keyGenerationCounter = 0;

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

            const u32 keyIndex = wrapper->keyIndex;
            const u64 keyGeneration = wrapper->keyGeneration;
            void* const userValue = wrapper->userValue;

            void (*userCallback)(void*) = nullptr;
            EnterCriticalSection(&registryLock);
            if (keyIndex < THREAD_KEY_MAX_INDEX && keyGenerations[keyIndex] == keyGeneration) {
                userCallback = userCallbacks[keyIndex];
            }
            LeaveCriticalSection(&registryLock);

            if (userCallback) {
                userCallback(userValue);
            }

            free(wrapper);
        }
    }

    auto ThreadKey_init(ThreadKey* self, void (*destructor)(void* value)) -> bool {
        if (!self || self->valid) {
            return false;
        }

        self->handle = 0;
        self->generation = 0;
        self->valid = false;

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
        keyGenerationCounter++;
        const u64 generation = keyGenerationCounter;
        userCallbacks[index] = destructor;
        keyGenerations[index] = generation;
        LeaveCriticalSection(&registryLock);

        self->handle = (u32) index;
        self->generation = generation;
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
            keyGenerations[index] = 0;
        }
        LeaveCriticalSection(&registryLock);

        FlsFree(index);

        self->handle = 0;
        self->generation = 0;
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

        if (wrapper->keyGeneration != self->generation) {
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

        if (wrapper && wrapper->keyGeneration != self->generation) {
            FlsSetValue(index, nullptr);
            free(wrapper);
            wrapper = nullptr;
        }

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
        wrapper->keyGeneration = self->generation;
        wrapper->userValue = value;

        if (!FlsSetValue(index, wrapper)) {
            free(wrapper);
            return false;
        }

        return true;
    }
    #else
    auto ThreadKey_init(ThreadKey* self, void (*destructor)(void* value)) -> bool {
        if (!self || self->valid) {
            return false;
        }

        self->handle = 0;
        self->generation = 0;
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
        self->generation = 0;
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
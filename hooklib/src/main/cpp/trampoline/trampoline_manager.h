//
// Created by swift on 2019/1/20.
//

#ifndef SANDHOOK_TRAMPOLINE_MANAGER_H
#define SANDHOOK_TRAMPOLINE_MANAGER_H

#include "map"
#include "list"
#include "trampoline.cpp"
#include "../utils/lock.h"
#include <sys/mman.h>
#include "../casts/art/art.h"
#include <unistd.h>

namespace SandHook {

    #define MMAP_PAGE_SIZE sysconf(_SC_PAGESIZE)
    #define EXE_BLOCK_SIZE MMAP_PAGE_SIZE

    using namespace art;


    class HookTrampoline {
    public:

        HookTrampoline() = default;

        Trampoline* replacement = nullptr;
        Trampoline* inlineJump = nullptr;
        Trampoline* inlineSecondory = nullptr;
        Trampoline* callOrigin = nullptr;
        Code originCode = nullptr;
    };

    class TrampolineManager {
    public:
        TrampolineManager() = default;

        void init(int sdk, Size quickCompileOffset) {
            this->quickCompileOffset = quickCompileOffset;
            SDK_INT = sdk;
        }

        Code allocExecuteSpace(Size size);

        HookTrampoline* installReplacementTrampoline(mirror::ArtMethod* originMethod, mirror::ArtMethod* hookMethod, mirror::ArtMethod* backupMethod);
        HookTrampoline* installInlineTrampoline(mirror::ArtMethod* originMethod, mirror::ArtMethod* hookMethod, mirror::ArtMethod* backupMethod,
                                                bool isNative);

        uint32_t sizeOfEntryCode(mirror::ArtMethod* method);

        HookTrampoline* getHookTrampoline(mirror::ArtMethod* method) {
            return trampolines[method];
        }

        bool memUnprotect(Size addr, Size len) {
            long pagesize = sysconf(_SC_PAGESIZE);
            unsigned alignment = (unsigned)((unsigned long long)addr % pagesize);
            int i = mprotect((void *) (addr - alignment), (size_t) (alignment + len),
                             PROT_READ | PROT_WRITE | PROT_EXEC);
            if (i == -1) {
                return false;
            }
            return true;
        }

        Code getEntryCode(void* method) {
            Code entryCode = *reinterpret_cast<Code*>((Size)method + quickCompileOffset);
            return entryCode;
        }

        static bool isThumbCode(Size codeAddr) {
            return (codeAddr & 0x1) == 0x1;
        }

        static void checkThumbCode(Trampoline* trampoline, Code code) {
            #if defined(__arm__)
            trampoline->setThumb(isThumbCode(reinterpret_cast<Size>(code)));
            #endif
        }

        static Code getThumbCodeAddress(Code code) {
            Size addr = reinterpret_cast<Size>(code) & (~0x00000001);
            return reinterpret_cast<Code>(addr);
        }


    private:

        Size quickCompileOffset;
        int SDK_INT = 0;
        std::map<mirror::ArtMethod*,HookTrampoline*> trampolines;
        std::list<Code> executeSpaceList = std::list<Code>();
        std::mutex allocSpaceLock;
        std::mutex installLock;
        Size executePageOffset = 0;
    };

}

#endif //SANDHOOK_TRAMPOLINE_MANAGER_H

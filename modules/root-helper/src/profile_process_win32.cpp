#include "gg_profile_process.h"
#include "gg_raii.h"

#include <cstdint>
#include <cstring>
#include <tlhelp32.h>
#include <vector>

namespace {

constexpr DWORD kSdkTextLimit = 8000000u;

uintptr_t process_image_base(DWORD pid) {
    gg::FileHandle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)};
    if (!snapshot)
        return 0;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof entry;
    if (Module32FirstW(snapshot.get(), &entry))
        return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
    return 0;
}

} // namespace

gg_detection gg_detect_process_sdk(uint32_t pid) {
    gg_detection result{GG_PROFILE_UNKNOWN, 0};
    uintptr_t base;
    if (!pid || !(base = process_image_base(pid)))
        return result;
    gg::NullHandle process{OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)};
    wchar_t image_path[MAX_PATH];
    DWORD path_length = MAX_PATH;
    if (!process || !QueryFullProcessImageNameW(process.get(), 0, image_path, &path_length))
        return result;
    // The packer may erase the mapped PE header. Use the original file for section layout.
    gg::FileHandle image_file{CreateFileW(image_path, GENERIC_READ,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          nullptr, OPEN_EXISTING, 0, nullptr)};
    BYTE header[4096];
    DWORD header_length = 0;
    if (!image_file ||
        !ReadFile(image_file.get(), header, sizeof header, &header_length, nullptr) ||
        header_length != sizeof header)
        return result;
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(header);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < static_cast<LONG>(sizeof *dos) ||
        dos->e_lfanew > static_cast<LONG>(sizeof header - sizeof(IMAGE_NT_HEADERS32)))
        return result;
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS32 *>(header + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->FileHeader.NumberOfSections > 24)
        return result;
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    if (reinterpret_cast<BYTE *>(sections + nt->FileHeader.NumberOfSections) >
        header + sizeof header)
        return result;
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (std::memcmp(sections[i].Name, ".text\0", 6))
            continue;
        const DWORD size = sections[i].Misc.VirtualSize;
        if (size < 96 || size > kSdkTextLimit ||
            sections[i].VirtualAddress > nt->OptionalHeader.SizeOfImage ||
            size > nt->OptionalHeader.SizeOfImage - sections[i].VirtualAddress)
            return result;
        std::vector<BYTE> text(size);
        unsigned pages = 0;
        for (DWORD offset = 0; offset < size; offset += 4096u) {
            const DWORD part = size - offset > 4096u ? 4096u : size - offset;
            SIZE_T got = 0;
            if (ReadProcessMemory(
                    process.get(),
                    reinterpret_cast<LPCVOID>(base + sections[i].VirtualAddress + offset),
                    text.data() + offset, part, &got) &&
                got == part)
                ++pages;
        }
        // A partially readable section cannot rule out another matching wrapper.
        if (pages == (size + 4095u) / 4096u)
            result = gg_detect_sdk(text.data(), size);
        break;
    }
    return result;
}

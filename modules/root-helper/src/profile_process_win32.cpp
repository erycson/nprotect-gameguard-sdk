#include "gg_profile_process.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <string.h>

#define GG_SDK_TEXT_LIMIT 8000000u

static uintptr_t process_image_base(DWORD pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32W entry;
    uintptr_t base = 0;
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    memset(&entry, 0, sizeof entry);
    entry.dwSize = sizeof entry;
    if (Module32FirstW(snapshot, &entry))
        base = (uintptr_t)entry.modBaseAddr;
    CloseHandle(snapshot);
    return base;
}

gg_detection gg_detect_process_sdk(uint32_t pid) {
    gg_detection result = {GG_PROFILE_UNKNOWN, 0};
    BYTE header[4096], *text = NULL;
    SIZE_T got;
    HANDLE process = NULL, image_file = INVALID_HANDLE_VALUE;
    wchar_t image_path[MAX_PATH];
    DWORD path_length = MAX_PATH, header_length = 0;
    uintptr_t base;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS32 *nt;
    IMAGE_SECTION_HEADER *sections;
    unsigned i;
    if (!pid || !(base = process_image_base(pid)))
        return result;
    process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process || !QueryFullProcessImageNameW(process, 0, image_path, &path_length))
        goto done;
    /* The packer may erase the mapped PE header. Use the original file for section layout. */
    image_file = CreateFileW(image_path, GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                             OPEN_EXISTING, 0, NULL);
    if (image_file == INVALID_HANDLE_VALUE ||
        !ReadFile(image_file, header, sizeof header, &header_length, NULL) ||
        header_length != sizeof header)
        goto done;
    dos = (IMAGE_DOS_HEADER *)header;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < (LONG)sizeof *dos ||
        dos->e_lfanew > (LONG)(sizeof header - sizeof *nt))
        goto done;
    nt = (IMAGE_NT_HEADERS32 *)(header + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->FileHeader.NumberOfSections > 24)
        goto done;
    sections = IMAGE_FIRST_SECTION(nt);
    if ((BYTE *)(sections + nt->FileHeader.NumberOfSections) > header + sizeof header)
        goto done;
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        DWORD size = sections[i].Misc.VirtualSize, offset;
        unsigned pages = 0;
        if (memcmp(sections[i].Name, ".text\0", 6))
            continue;
        if (size < 96 || size > GG_SDK_TEXT_LIMIT ||
            sections[i].VirtualAddress > nt->OptionalHeader.SizeOfImage ||
            size > nt->OptionalHeader.SizeOfImage - sections[i].VirtualAddress)
            goto done;
        text = (BYTE *)calloc(1, size);
        if (!text)
            goto done;
        for (offset = 0; offset < size; offset += 4096u) {
            DWORD part = size - offset > 4096u ? 4096u : size - offset;
            if (ReadProcessMemory(process, (LPCVOID)(base + sections[i].VirtualAddress + offset),
                                  text + offset, part, &got) &&
                got == part)
                ++pages;
        }
        /* A partially readable section cannot rule out another matching wrapper. */
        if (pages == (size + 4095u) / 4096u)
            result = gg_detect_sdk(text, size);
        break;
    }
done:
    free(text);
    if (image_file != INVALID_HANDLE_VALUE)
        CloseHandle(image_file);
    if (process)
        CloseHandle(process);
    return result;
}

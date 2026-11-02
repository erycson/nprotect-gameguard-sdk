/* Compatibility for the exact signed v584 GameMon, not a general exception filter.
   Four pinned NULL call sites resume at the real ntdll exports; all other
   exceptions follow the original handlers. No validation result is replaced. */
typedef struct {
    BYTE b[2048];
    int n;
    BOOL overflow;
} CODE;
static void emit_byte(CODE *c, BYTE x) {
    if (c->n >= (int)sizeof c->b) {
        c->overflow = TRUE;
        return;
    }
    c->b[c->n++] = x;
}
static void emit_bytes(CODE *c, const BYTE *p, int n) {
    if (n < 0 || n > (int)sizeof c->b - c->n) {
        c->overflow = TRUE;
        return;
    }
    memcpy(c->b + c->n, p, (size_t)n);
    c->n += n;
}
static void emit_dword(CODE *c, DWORD x) {
    emit_bytes(c, (const BYTE *)&x, 4);
}
static int emit_conditional_jump(CODE *c, BYTE op) {
    emit_byte(c, 15);
    emit_byte(c, op);
    int p = c->n;
    emit_dword(c, 0);
    return p;
}
static void patch_branch(CODE *c, int p, int target) {
    DWORD x;
    if (p < 0 || p > c->n - 4 || target < 0 || target > c->n) {
        c->overflow = TRUE;
        return;
    }
    x = (DWORD)(target - p - 4);
    memcpy(c->b + p, &x, 4);
}
static int emit_compare(CODE *c, int reg, int offset, DWORD value, BYTE op) {
    emit_byte(c, 0x81);
    if (offset) {
        emit_byte(c, (BYTE)(0x78 + reg));
        emit_byte(c, (BYTE)offset);
    } else
        emit_byte(c, (BYTE)(0x38 + reg));
    emit_dword(c, value);
    return emit_conditional_jump(c, op);
}
static DWORD remote_ntdll_base(DWORD pid) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32W m = {.dwSize = sizeof(m)};
    DWORD b = 0;
    if (s == INVALID_HANDLE_VALUE)
        return 0;
    if (Module32FirstW(s, &m))
        do {
            if (!_wcsicmp(m.szModule, L"ntdll.dll")) {
                b = (DWORD)(uintptr_t)m.modBaseAddr;
                break;
            }
        } while (Module32NextW(s, &m));
    CloseHandle(s);
    return b;
}
static DWORD ntfn(DWORD base, const char *name) {
    HMODULE h = GetModuleHandleW(L"ntdll.dll");
    FARPROC p = GetProcAddress(h, name);
    return p ? base + (DWORD)((BYTE *)p - (BYTE *)h) : 0;
}
static BOOL build_exception_handler(CODE *out, DWORD base, const DWORD apis[4]) {
    CODE c = {0};
    int fails[64], nf = 0;
    BYTE pre[] = {0x9c, 0x60, 0x8b, 0x44, 0x24, 0x28, 0x8b, 0x30};
    emit_bytes(&c, pre, sizeof(pre));
    fails[nf++] = emit_compare(&c, 6, 0, 0xc0000005, 0x85);
    BYTE context[] = {0x8b, 0x58, 0x04, 0x81, 0xbb};
    emit_bytes(&c, context, sizeof(context));
    emit_dword(&c, 184);
    emit_dword(&c, 0);
    fails[nf++] = emit_conditional_jump(&c, 0x85);
    BYTE sp[] = {0x8b, 0x93};
    emit_bytes(&c, sp, 2);
    emit_dword(&c, 196);
    DWORD returns[] = {0x71eced, 0x71ed04, 0x71ed88, 0x71ed9f};
    BOOL valid = TRUE;
    for (int i = 0; i < 4; i++) {
        DWORD api = apis[i];
        if (!api) {
            valid = FALSE;
            break;
        }
        int next = emit_compare(&c, 2, 0, returns[i], 0x85);
        if (i == 0) {
            fails[nf++] = emit_compare(&c, 2, 8, 0x1f03ff, 0x85);
            BYTE obj[] = {0x8b, 0x4a, 0x0c, 0x85, 0xc9};
            emit_bytes(&c, obj, sizeof(obj));
            fails[nf++] = emit_conditional_jump(&c, 0x84);
            fails[nf++] = emit_compare(&c, 1, 0, 24, 0x85);
            BYTE cid[] = {0x8b, 0x4a, 0x10, 0x85, 0xc9};
            emit_bytes(&c, cid, sizeof(cid));
            fails[nf++] = emit_conditional_jump(&c, 0x84);
            fails[nf++] = emit_compare(&c, 1, 0, 0, 0x85);
            fails[nf++] = emit_compare(&c, 1, 4, 0, 0x84);
        }
        if (i == 1) {
            fails[nf++] = emit_compare(&c, 2, 8, 9, 0x85);
            fails[nf++] = emit_compare(&c, 2, 16, 4, 0x85);
            fails[nf++] = emit_compare(&c, 2, 20, 0, 0x85);
        }
        if (i == 2) {
            fails[nf++] = emit_compare(&c, 2, 8, 17, 0x85);
            fails[nf++] = emit_compare(&c, 2, 12, 0, 0x85);
            fails[nf++] = emit_compare(&c, 2, 16, 0, 0x85);
        }
        if (i == 3)
            fails[nf++] = emit_compare(&c, 2, 4, 0, 0x84);
        BYTE set[] = {0xc7, 0x83};
        emit_bytes(&c, set, 2);
        emit_dword(&c, 184);
        emit_dword(&c, api);
        BYTE inc[] = {0xff, 0x05};
        emit_bytes(&c, inc, 2);
        emit_dword(&c, base + 4096 + i * 4);
        BYTE done[] = {0x61, 0x9d, 0xb8, 0xff, 0xff, 0xff, 0xff, 0xc2, 4, 0};
        emit_bytes(&c, done, sizeof(done));
        patch_branch(&c, next, c.n);
    }
    int end = c.n;
    BYTE pass[] = {0x61, 0x9d, 0x31, 0xc0, 0xc2, 4, 0};
    emit_bytes(&c, pass, sizeof(pass));
    for (int i = 0; i < nf; i++)
        patch_branch(&c, fails[i], end);
    *out = c;
    return valid && !c.overflow;
}
static BOOL install_monitor_handler(DWORD pid, const wchar_t *expected) {
    DWORD nt = remote_ntdll_base(pid);
    if (!nt)
        return FALSE;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ |
                               PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | SYNCHRONIZE,
                           FALSE, pid);
    if (!h)
        return FALSE;
    wchar_t actual[MAX_PATH];
    DWORD n = MAX_PATH;
    if (!QueryFullProcessImageNameW(h, 0, actual, &n) || _wcsicmp(actual, expected)) {
        CloseHandle(h);
        return FALSE;
    }
    DWORD base = (DWORD)(uintptr_t)VirtualAllocEx(h, 0, 8192, MEM_COMMIT | MEM_RESERVE,
                                                  PAGE_EXECUTE_READWRITE);
    if (!base) {
        CloseHandle(h);
        return FALSE;
    }
    DWORD apis[4] = {ntfn(nt, "NtOpenThread"), ntfn(nt, "NtQueryInformationThread"),
                     ntfn(nt, "NtSetInformationThread"), ntfn(nt, "NtClose")};
    CODE c;
    BOOL valid = build_exception_handler(&c, base, apis);
    CODE start = {0};
    emit_byte(&start, 0x68);
    emit_dword(&start, base);
    BYTE one[] = {0x6a, 1, 0xb8};
    emit_bytes(&start, one, 3);
    DWORD add = ntfn(nt, "RtlAddVectoredExceptionHandler");
    emit_dword(&start, add);
    BYTE call[] = {0xff, 0xd0, 0xa3};
    emit_bytes(&start, call, 3);
    emit_dword(&start, base + 4128);
    BYTE ret[] = {0xc2, 4, 0};
    emit_bytes(&start, ret, 3);
    SIZE_T wrote;
    BOOL ok = valid && !start.overflow && add &&
              WriteProcessMemory(h, (void *)(uintptr_t)base, c.b, c.n, &wrote) &&
              WriteProcessMemory(h, (void *)(uintptr_t)(base + 2048), start.b, start.n, &wrote);
    HANDLE thread = 0;
    BOOL running = FALSE;
    if (ok) {
        FlushInstructionCache(h, 0, 0);
        thread =
            CreateRemoteThread(h, 0, 0, (LPTHREAD_START_ROUTINE)(uintptr_t)(base + 2048), 0, 0, 0);
        ok = thread != 0;
    }
    if (thread) {
        running = WaitForSingleObject(thread, 3000) == WAIT_TIMEOUT;
        DWORD installed = 0;
        SIZE_T got;
        ok = !running &&
             ReadProcessMemory(h, (void *)(uintptr_t)(base + 4128), &installed, 4, &got) &&
             got == 4 && installed;
        CloseHandle(thread);
    }
    if (!ok && !running)
        VirtualFreeEx(h, (void *)(uintptr_t)base, 0, MEM_RELEASE);
    CloseHandle(h);
    return ok;
}

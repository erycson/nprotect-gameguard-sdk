#ifndef GG_RAII_H
#define GG_RAII_H

#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <windows.h>
#include <wincrypt.h>

namespace gg {

// Move-only RAII owner for a Win32-style resource described by Traits.
template <class Traits> class Unique {
  public:
    using value_type = typename Traits::value_type;

    Unique() = default;
    explicit Unique(value_type handle) : handle_(handle) {}
    Unique(Unique &&other) noexcept : handle_(other.release()) {}
    Unique &operator=(Unique &&other) noexcept {
        if (this != &other) {
            reset();
            handle_ = other.release();
        }
        return *this;
    }
    Unique(const Unique &) = delete;
    Unique &operator=(const Unique &) = delete;
    ~Unique() {
        reset();
    }

    value_type get() const {
        return handle_;
    }
    explicit operator bool() const {
        return handle_ != Traits::invalid();
    }

    // Releases any owned handle and returns a pointer for an out-parameter API.
    value_type *receive() {
        reset();
        return &handle_;
    }
    value_type release() {
        value_type released = handle_;
        handle_ = Traits::invalid();
        return released;
    }
    void reset() {
        if (handle_ != Traits::invalid()) {
            Traits::close(handle_);
            handle_ = Traits::invalid();
        }
    }

  private:
    value_type handle_ = Traits::invalid();
};

namespace detail {

struct NullHandleTraits {
    using value_type = HANDLE;
    static HANDLE invalid() {
        return nullptr;
    }
    static void close(HANDLE h) {
        CloseHandle(h);
    }
};
struct FileHandleTraits {
    using value_type = HANDLE;
    static HANDLE invalid() {
        return INVALID_HANDLE_VALUE;
    }
    static void close(HANDLE h) {
        CloseHandle(h);
    }
};
struct CryptProviderTraits {
    using value_type = HCRYPTPROV;
    static HCRYPTPROV invalid() {
        return 0;
    }
    static void close(HCRYPTPROV h) {
        CryptReleaseContext(h, 0);
    }
};
struct CryptKeyTraits {
    using value_type = HCRYPTKEY;
    static HCRYPTKEY invalid() {
        return 0;
    }
    static void close(HCRYPTKEY h) {
        CryptDestroyKey(h);
    }
};
struct CryptHashTraits {
    using value_type = HCRYPTHASH;
    static HCRYPTHASH invalid() {
        return 0;
    }
    static void close(HCRYPTHASH h) {
        CryptDestroyHash(h);
    }
};
struct RegKeyTraits {
    using value_type = HKEY;
    static HKEY invalid() {
        return nullptr;
    }
    static void close(HKEY h) {
        RegCloseKey(h);
    }
};
struct CFileTraits {
    using value_type = std::FILE *;
    static std::FILE *invalid() {
        return nullptr;
    }
    static void close(std::FILE *f) {
        std::fclose(f);
    }
};

} // namespace detail

using NullHandle = Unique<detail::NullHandleTraits>; // OpenProcess, CreateEvent, CreateThread
using FileHandle = Unique<detail::FileHandleTraits>; // CreateFile*
using CryptProvider = Unique<detail::CryptProviderTraits>;
using CryptKey = Unique<detail::CryptKeyTraits>;
using CryptHash = Unique<detail::CryptHashTraits>;
using RegKey = Unique<detail::RegKeyTraits>;
using CFile = Unique<detail::CFileTraits>;

} // namespace gg

#endif

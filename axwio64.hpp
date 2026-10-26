#pragma once

#include <windows.h>
#include <cstdint>

constexpr DWORD ioctl_map_phys = 0x80102040;
constexpr DWORD ioctl_unmap_phys = 0x80102044;
constexpr DWORD ioctl_read_port = 0x80102050;
constexpr DWORD ioctl_write_port = 0x80102054;

struct phys_mem_buf_t {
    LARGE_INTEGER mapping_size;
    LARGE_INTEGER physical_address;
    LARGE_INTEGER section_handle;
    LARGE_INTEGER virtual_address;
    PVOID         section_object;
};
static_assert(sizeof(phys_mem_buf_t) == 40);

#pragma pack(push, 1)
struct port_buf_t {
    uint16_t port;
    uint32_t value;
    uint8_t  size;
};
#pragma pack(pop)
static_assert(sizeof(port_buf_t) == 7);

class axwio64_t {
public:
    axwio64_t() {
        handle_ = ::CreateFileW(
            L"\\\\.\\AxWIO",
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    }

    ~axwio64_t() {
        if (handle_ != INVALID_HANDLE_VALUE)
            ::CloseHandle(handle_);
    }

    [[nodiscard]] bool is_open() const noexcept {
        return handle_ != INVALID_HANDLE_VALUE;
    }

    bool map_physical(phys_mem_buf_t& buf) const {
        DWORD bytes = 0;
        return ::DeviceIoControl(
            handle_, ioctl_map_phys,
            &buf, sizeof(buf),
            &buf, sizeof(buf),
            &bytes, nullptr) != FALSE;
    }

    bool unmap_physical(const phys_mem_buf_t& buf) const {
        DWORD bytes = 0;
        return ::DeviceIoControl(
            handle_, ioctl_unmap_phys,
            const_cast<phys_mem_buf_t*>(&buf), sizeof(buf),
            nullptr, 0,
            &bytes, nullptr) != FALSE;
    }

    bool read_port(uint16_t port, uint8_t size, uint32_t& out) const {
        port_buf_t in = { port, 0, size };
        uint32_t   val = 0;
        DWORD      bytes = 0;
        bool ok = ::DeviceIoControl(
            handle_, ioctl_read_port,
            &in, sizeof(in),
            &val, sizeof(val),
            &bytes, nullptr) != FALSE;
        if (ok) out = val;
        return ok;
    }

    bool write_port(uint16_t port, uint32_t value, uint8_t size) const {
        port_buf_t buf = { port, value, size };
        DWORD      bytes = 0;
        return ::DeviceIoControl(
            handle_, ioctl_write_port,
            &buf, sizeof(buf),
            nullptr, 0,
            &bytes, nullptr) != FALSE;
    }

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};
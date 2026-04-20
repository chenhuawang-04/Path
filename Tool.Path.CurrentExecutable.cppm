module;

#include <filesystem>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <unistd.h>
#elif defined(__linux__)
#  include <unistd.h>
#  include <cerrno>
#endif

export module Tool.Path.CurrentExecutable;

export namespace Tool::Path {

struct PathQueryResult final {
    std::filesystem::path path{};
    std::error_code error{};

    [[nodiscard]] explicit operator bool() const noexcept {
        return !error && !path.empty();
    }
};

[[nodiscard]] const std::filesystem::path& executable_path() noexcept;
[[nodiscard]] const std::filesystem::path& executable_directory() noexcept;
[[nodiscard]] const std::error_code& executable_path_error() noexcept;

[[nodiscard]] bool try_get_executable_path(std::filesystem::path& out,
                                           std::error_code& ec) noexcept;
[[nodiscard]] bool try_get_executable_directory(std::filesystem::path& out,
                                                std::error_code& ec) noexcept;

[[nodiscard]] PathQueryResult get_executable_path() noexcept;
[[nodiscard]] PathQueryResult get_executable_directory() noexcept;

} // namespace Tool::Path

namespace Tool::Path::detail {

struct Cache final {
    std::filesystem::path exe_path{};
    std::filesystem::path exe_dir{};
    std::error_code ec{};
};

[[nodiscard]] std::error_code query_executable_path(std::filesystem::path& out) noexcept {
#if defined(_WIN32)
    constexpr std::size_t kInitial = 260;
    constexpr std::size_t kMax = 32768;

    std::wstring buffer(kInitial, L'\0');

    while (true) {
        const auto cap = static_cast<DWORD>(buffer.size());
        const DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(), cap);

        if (len == 0) {
            return {static_cast<int>(::GetLastError()), std::system_category()};
        }

        if (len < cap) {
            buffer.resize(static_cast<std::size_t>(len));
            out = std::filesystem::path(std::move(buffer));
            return {};
        }

        if (buffer.size() >= kMax) {
            return std::make_error_code(std::errc::filename_too_long);
        }

        const auto next_size = (buffer.size() < kMax / 2) ? buffer.size() * 2 : kMax;
        buffer.resize(next_size);
    }

#elif defined(__linux__)
    constexpr std::size_t kInitial = 256;
    constexpr std::size_t kMax = 1 << 20; // 1 MiB hard cap.

    std::vector<char> buffer(kInitial);

    while (true) {
        const ssize_t n = ::readlink("/proc/self/exe", buffer.data(), buffer.size());

        if (n < 0) {
            return {errno, std::generic_category()};
        }

        if (static_cast<std::size_t>(n) < buffer.size()) {
            out = std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(n)));
            return {};
        }

        if (buffer.size() >= kMax) {
            return std::make_error_code(std::errc::filename_too_long);
        }

        const auto next_size = (buffer.size() < kMax / 2) ? buffer.size() * 2 : kMax;
        buffer.resize(next_size);
    }

#elif defined(__APPLE__)
    uint32_t size = 1024;
    std::string buffer(size, '\0');

    while (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        buffer.resize(size);
    }

    std::filesystem::path p(buffer.c_str());

    // Apple 文档说明该路径可能包含符号链接；这里做轻量 absolute 归一化，
    // 不做昂贵的 canonical 解析，兼顾性能与可用性。
    std::error_code abs_ec;
    auto abs = std::filesystem::absolute(p, abs_ec);
    if (!abs_ec) {
        p = std::move(abs);
    }

    out = std::move(p);
    return {};
#else
    (void)out;
    return std::make_error_code(std::errc::not_supported);
#endif
}

[[nodiscard]] Cache build_cache() noexcept {
    Cache c{};

    try {
        c.ec = query_executable_path(c.exe_path);
        if (c.ec) {
            return c;
        }

        c.exe_dir = c.exe_path.parent_path();
        if (c.exe_dir.empty()) {
            c.ec = std::make_error_code(std::errc::no_such_file_or_directory);
        }
    } catch (const std::bad_alloc&) {
        c.ec = std::make_error_code(std::errc::not_enough_memory);
        c.exe_path.clear();
        c.exe_dir.clear();
    } catch (...) {
        c.ec = std::make_error_code(std::errc::io_error);
        c.exe_path.clear();
        c.exe_dir.clear();
    }

    return c;
}

[[nodiscard]] const Cache& cache() noexcept {
    static const Cache c = build_cache();
    return c;
}

} // namespace Tool::Path::detail

namespace Tool::Path {

const std::filesystem::path& executable_path() noexcept {
    return detail::cache().exe_path;
}

const std::filesystem::path& executable_directory() noexcept {
    return detail::cache().exe_dir;
}

const std::error_code& executable_path_error() noexcept {
    return detail::cache().ec;
}

bool try_get_executable_path(std::filesystem::path& out, std::error_code& ec) noexcept {
    const auto& c = detail::cache();
    ec = c.ec;
    if (ec) {
        out.clear();
        return false;
    }

    out = c.exe_path;
    return true;
}

bool try_get_executable_directory(std::filesystem::path& out, std::error_code& ec) noexcept {
    const auto& c = detail::cache();
    ec = c.ec;
    if (ec) {
        out.clear();
        return false;
    }

    out = c.exe_dir;
    return true;
}

PathQueryResult get_executable_path() noexcept {
    const auto& c = detail::cache();
    return PathQueryResult{c.exe_path, c.ec};
}

PathQueryResult get_executable_directory() noexcept {
    const auto& c = detail::cache();
    return PathQueryResult{c.exe_dir, c.ec};
}

} // namespace Tool::Path


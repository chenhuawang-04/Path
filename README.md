# Tool.Path.CurrentExecutable

一个基于 **C++23 标准模块（cppm）** 的跨平台路径工具，命名空间为 `Tool::Path`，用于**高效、安全地获取当前可执行文件路径及所在目录**。

## 特性

- **跨平台**：Windows / Linux / macOS
- **高性能**：首次查询后静态缓存，后续调用零系统调用开销
- **安全可靠**：
  - API 全部 `noexcept`
  - 提供 `std::error_code` 错误返回
  - 避免异常向外传播
  - 平台路径长度自适应扩容，含上限保护
- **现代 C++ 接口**：标准模块 + `std::filesystem::path`

## 模块与命名空间

- 模块名：`Tool.Path.CurrentExecutable`
- 命名空间：`Tool::Path`
- 源文件：`Tool.Path.CurrentExecutable.cppm`

## 导出 API

```cpp
namespace Tool::Path {

struct PathQueryResult final {
    std::filesystem::path path;
    std::error_code error;
    explicit operator bool() const noexcept;
};

const std::filesystem::path& executable_path() noexcept;
const std::filesystem::path& executable_directory() noexcept;
const std::error_code& executable_path_error() noexcept;

bool try_get_executable_path(std::filesystem::path& out,
                             std::error_code& ec) noexcept;
bool try_get_executable_directory(std::filesystem::path& out,
                                  std::error_code& ec) noexcept;

PathQueryResult get_executable_path() noexcept;
PathQueryResult get_executable_directory() noexcept;

}
```

## 使用示例

```cpp
import Tool.Path.CurrentExecutable;
#include <iostream>

int main() {
    if (const auto& ec = Tool::Path::executable_path_error(); ec) {
        std::cerr << "query failed: " << ec.value() << " " << ec.message() << "\n";
        return 1;
    }

    std::cout << "exe: " << Tool::Path::executable_path().string() << "\n";
    std::cout << "dir: " << Tool::Path::executable_directory().string() << "\n";
    return 0;
}
```

## Clang 编译（C++23）

> 以下命令已在 Windows + clang++ 环境验证。

### 1) 编译模块接口

```powershell
clang++ -std=c++23 -c Tool.Path.CurrentExecutable.cppm -fmodule-output -o Tool.Path.CurrentExecutable.obj
```

会生成：

- `Tool.Path.CurrentExecutable.obj`
- `Tool.Path.CurrentExecutable.pcm`

### 2) 编译业务代码并链接

```powershell
clang++ -std=c++23 app.cpp Tool.Path.CurrentExecutable.obj -fprebuilt-module-path=. -o app.exe
```

## 平台实现说明

- **Windows**：`GetModuleFileNameW`
- **Linux**：`readlink("/proc/self/exe")`
- **macOS**：`_NSGetExecutablePath`（并进行轻量 `absolute` 归一化）

## 设计细节

- 使用函数内 `static const Cache` 完成一次性初始化与线程安全缓存。
- 首次调用后，后续 API 仅返回缓存结果，适合高频访问场景。
- 对极端长路径采用动态扩容与上限保护，避免无限增长。

## 注意事项

- Linux 依赖 `/proc/self/exe`（容器/特殊运行环境需确保可用）。
- macOS 返回路径可能存在符号链接语义差异；本实现默认优先性能，不做 `canonical`。

---
如需我继续补充：
- CMake 示例（含 Clang 模块构建）
- 单元测试（路径有效性、错误路径模拟）
- 兼容 `std::expected` 风格 API

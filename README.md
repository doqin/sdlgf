# sdlgf

Đây là một dự án dành cho học môn Nhập môn lập trình game - SE102: một framework game 2D bằng C++ chạy trên SDL3 (được port từ framework DirectX 9 cũ, xem `dx9gf`).

# Cách chạy demo game (CMake)
Dự án đã được port sang SDL3 + CMake (xem `cmake/` và `CMakeLists.txt`). Không cần cài DirectX SDK: SDL3, SDL_ttf, SDL_image và tmxlite được tải và build từ source ở lần cấu hình đầu tiên (cần `git` và kết nối mạng).

Windows:
```
cmake -S . -B build -A x64
cmake --build build --config Release
build/bin/Release/demo.exe
```

Linux (xem danh sách thư viện cần cài trong job `linux` của `.github/workflows/portable-build.yml`):
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build/bin/Release/demo
```

Tuỳ chọn: `-DSDLGF_TESTING=ON` (vào thẳng `DebugScene`), `-DSDLGF_COPY_ASSETS=OFF`, `-DSDLGF_BUILD_TESTS=OFF`.

Chạy test: `ctest --test-dir build -C Release --output-on-failure`

Phát hành bản build: chạy workflow `Portable build` bằng `workflow_dispatch` và nhập `version` (ví dụ `0.1.0`, không có chữ `v`); workflow sẽ build, test, đóng gói, tạo tag `v0.1.0` và publish GitHub Release kèm hai file: `sdlgf-demo.zip` (Windows x64) và `sdlgf-demo-linux-x86_64.tar.gz` (Linux x64, giải nén rồi chạy `./demo`). Release chỉ được tạo khi cả job Windows lẫn Linux đều thành công. Chỉ chạy được từ nhánh `master`, và tag `v<version>` đó phải chưa tồn tại. Push một tag `v*` cũng publish release như cũ.

Cửa sổ, vòng lặp, input, đồ hoạ (SDL_Renderer), chữ (SDL_ttf) và âm thanh (SDL3 audio) đều chạy trên SDL3; file âm thanh đọc được dạng `.wav` hoặc `.ogg` (Ogg Vorbis, giải mã bằng SDL_mixer, tải và build từ source cùng các thư viện khác); không còn Direct3D 9 / DirectInput / XAudio2. CI build và chạy test trên Windows và Linux.

Các file Visual Studio cũ (`.vcxproj`, `.slnx`) đã được xoá; dùng CMake (Visual Studio mở được thư mục CMake trực tiếp).

# Phát triển trong VSCodium (CMake Tools + clangd)

Đã cấu hình sẵn trong repo: `CMakePresets.json` và `.vscode/{settings,launch,tasks,extensions}.json`.

- Extension: `ms-vscode.cmake-tools`, `llvm-vs-code-extensions.vscode-clangd`, và `webfreak.debug` (Native Debug — dùng gdb; VSCodium không cài được `ms-vscode.cpptools` từ Open VSX nên không dùng được `cppdbg` / nút debug của CMake Tools).
- Mở thư mục là CMake Tools tự configure theo preset `linux` (Ninja, Release, build vào `build/` — đúng như README, executable ở `build/bin/Release/demo`). Muốn debug có debug info thì chọn preset `linux-debug` (`build/debug`, `-g`, không bật `SDLGF_TESTING`; thêm `-DSDLGF_TESTING=ON` nếu muốn vào thẳng DebugScene, executable ở `build/debug/bin/Debug/demo`); preset này build các thư viện phụ thêm một lần nữa và tốn thêm ~900 MB trong `build/debug`.
- `CMakeLists.txt` bật `CMAKE_EXPORT_COMPILE_COMMANDS`; mỗi lần configure, CMake Tools chép `compile_commands.json` của preset đang chọn ra thư mục gốc repo (`cmake.copyCompileCommands`, file này bị git bỏ qua) và clangd đọc file đó. Đổi preset (`linux` ↔ `linux-debug`) thì clangd đổi theo: `_DEBUG` (bật các khối `#ifdef _DEBUG`), `NDEBUG`, `SDLGF_TESTING`... Với Debug trên Linux, `core/CMakeLists.txt` tự định nghĩa `_DEBUG` (MSVC thì tự có). clangd dùng PCH (`cmake_pch.hxx`) do CMake sinh, nên preset đó phải được build ít nhất một lần (chỉ configure thì clangd báo thiếu file PCH).
- Task: `cmake: build` (Ctrl+Shift+B, targets `all`), `run: demo`, `test: ctest (build/)`; F5 chạy các cấu hình trong `launch.json`.

# Thành viên làm đồ án
24520763 - Nguyễn Tuấn Khang

23521334 - Phạm Ngọc Trúc Quỳnh

25520200 - Trần Tô Việt Chí

24521166 - Trần Thị Hoài Ngọc

24520743 - Lê Nguyễn Hữu Khang

Youtube Walkthrough: https://youtu.be/pcNXUmLuEN8

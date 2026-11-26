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
build/bin/demo
```

Tuỳ chọn: `-DSDLGF_TESTING=ON` (vào thẳng `DebugScene`), `-DSDLGF_COPY_ASSETS=OFF`, `-DSDLGF_BUILD_TESTS=OFF`.

Chạy test: `ctest --test-dir build -C Release --output-on-failure`

Phát hành bản build: chạy workflow `Portable build` bằng `workflow_dispatch` và nhập `version` (ví dụ `0.1.0`, không có chữ `v`); workflow sẽ build, test, đóng gói, tạo tag `v0.1.0` và publish GitHub Release kèm hai file: `sdlgf-demo.zip` (Windows x64) và `sdlgf-demo-linux-x86_64.tar.gz` (Linux x64, giải nén rồi chạy `./demo`). Release chỉ được tạo khi cả job Windows lẫn Linux đều thành công. Chỉ chạy được từ nhánh `master`, và tag `v<version>` đó phải chưa tồn tại. Push một tag `v*` cũng publish release như cũ.

Cửa sổ, vòng lặp, input, đồ hoạ (SDL_Renderer), chữ (SDL_ttf) và âm thanh (SDL3 audio) đều chạy trên SDL3; file âm thanh đọc được dạng `.wav` hoặc `.ogg` (Ogg Vorbis, giải mã bằng SDL_mixer, tải và build từ source cùng các thư viện khác); không còn Direct3D 9 / DirectInput / XAudio2. CI build và chạy test trên Windows và Linux.

Các file Visual Studio cũ (`.vcxproj`, `.slnx`) đã được xoá; dùng CMake (Visual Studio mở được thư mục CMake trực tiếp).

# Thành viên làm đồ án
24520763 - Nguyễn Tuấn Khang

23521334 - Phạm Ngọc Trúc Quỳnh

25520200 - Trần Tô Việt Chí

24521166 - Trần Thị Hoài Ngọc

24520743 - Lê Nguyễn Hữu Khang

Youtube Walkthrough: https://youtu.be/pcNXUmLuEN8

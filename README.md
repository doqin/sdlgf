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

Cửa sổ, vòng lặp, input, đồ hoạ (SDL_Renderer), chữ (SDL_ttf) và âm thanh (SDL3 audio) đều chạy trên SDL3; không còn Direct3D 9 / DirectInput / XAudio2. CI build và chạy test trên Windows và Linux. Trên Linux mới chỉ build và chạy test tự động (âm thanh dùng driver "dummy"); chưa chạy thử demo bằng cửa sổ và loa thật.

Các file `.vcxproj`/`.slnx` cũ sẽ bị xoá ở bước dọn dẹp cuối của quá trình port.

# Thành viên làm đồ án
24520763 - Nguyễn Tuấn Khang

23521334 - Phạm Ngọc Trúc Quỳnh

25520200 - Trần Tô Việt Chí

24521166 - Trần Thị Hoài Ngọc

24520743 - Lê Nguyễn Hữu Khang

Youtube Walkthrough: https://youtu.be/pcNXUmLuEN8

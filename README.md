# sdlgf

Đây là một dự án dành cho học môn Nhập môn lập trình game - SE102: một framework game 2D bằng C++ chạy trên SDL3 (được port từ framework DirectX 9 cũ, xem `dx9gf`).

# Cách chạy demo game (CMake)
Nhánh này đang được port sang SDL3 + CMake (xem `cmake/` và `CMakeLists.txt`). Không cần cài DirectX SDK nữa: D3DX9 được tải tự động từ NuGet, SDL3 và tmxlite được build từ source.

```
cmake -S . -B build -A x64
cmake --build build --config Release
build/bin/Release/demo.exe
```

Tuỳ chọn: `-DSDLGF_TESTING=ON` (vào thẳng `DebugScene`), `-DSDLGF_COPY_ASSETS=OFF`, `-DSDLGF_BUILD_TESTS=OFF`.

Chạy test: `ctest --test-dir build -C Release --output-on-failure`

Cửa sổ, vòng lặp và input đã chạy trên SDL3 (không còn Win32/DirectInput). Phần đồ hoạ (Direct3D 9) và âm thanh (XAudio2) vẫn đang được port ở các bước sau.

Các file `.vcxproj`/`.slnx` cũ sẽ bị xoá ở bước dọn dẹp cuối của quá trình port.

# Thành viên làm đồ án
24520763 - Nguyễn Tuấn Khang

23521334 - Phạm Ngọc Trúc Quỳnh

25520200 - Trần Tô Việt Chí

24521166 - Trần Thị Hoài Ngọc

24520743 - Lê Nguyễn Hữu Khang

Youtube Walkthrough: https://youtu.be/pcNXUmLuEN8

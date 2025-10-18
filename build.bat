@echo off
echo 构建 ImGui 中文字体项目...

if not exist "build" mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
if errorlevel 1 (
    echo CMake 配置失败!
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo 构建失败!
    pause
    exit /b 1
)

echo 构建完成！可执行文件位于: build\Release\ImGuiProject.exe
pause 
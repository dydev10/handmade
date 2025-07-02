@echo off

mkdir build
pushd build

cl -Zi ..\src\win32_handmade.cpp  /link user32.lib Gdi32.lib

popd
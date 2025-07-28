@echo off

mkdir build
pushd build

cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FC -Zi ..\src\win32_handmade.cpp  /link user32.lib Gdi32.lib

popd
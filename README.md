Tool to easily create hardlinks on Windows

# Use cases
- Create a Windows explorer directory context menu entry by creating the registry keys `HKEY_CLASSES_ROOT\Directory\Background\shell\paste_hardlink\command`
  - set value of `paste_hardlink` to the text to be shown in the context menu
  - set value of `command` to the executable path with corresponding options
- Create a hotkey with [AutoHotKey](https://www.autohotkey.com/), e.g.:
  ```
  #IfWinActive ahk_exe explorer.exe
  ^h::
  cmnd = "HardLinker.exe" --auto
  RunWait, %cmnd%
  return
  #IfWinActive
  ```

# How to use
For help, run:
```
HardLinker.exe --help
```
Currently only supports creating hardlinks with:
```
HardLinker.exe --auto
```
This creates hardlinks to all files on the clipboard inside the currently focused Windows explorer window.


# Install / Compile
Compile the code with CMake to create the executable in `./bin/` in the repository root.
Requires an installation of [Qt5](https://www.qt.io/) to compile.
Compile by running the following in the repository root[^1]:
```
mkdir build
cd build
cmake ..
make
make install
```

[^1]: tested under [MSYS2](https://www.msys2.org/) with `mingw-w64-x86_64-gcc 11.2.0-5`

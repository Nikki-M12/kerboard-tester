# Keyboard Tester

![](https://github.com/Nikki-M12/kerboard-tester/blob/main/key_tester.png)

## Description

It is a keyboard tester that graphically shows which key is pressed, has a simple window that turns green when a key is pressed and shows the name of the detected key

## Requirements

### Windows
- Windows 7 or higher
- C++ compiler (Visual Studio, MinGW, etc.)

### Linux
- GTK+ 3.0
- C++ compiler (g++, clang, etc.)

#### Ubuntu/Debian:
```bash
sudo apt-get install build-essential g++ libgtk-3-dev
```
### Fedora/RHEL:
```bash
sudo dnf install gcc-c++ gtk3-devel
```
### Arch Linux:
```bash
sudo pacman -S base-devel gtk3
```
# Compilation
## Windows (MinGW-w64)
```bash
g++ -std=c++11 key_tester.cpp -lgdi32 -luser32 -o key_tester.exe
```
## Linux
```bash
g++ -std=c++11 key_tester.cpp `pkg-config --cflags --libs gtk+-3.0` -o key_tester
```
# Usage
- Compile the code
- Run the program
- Test the keyboard

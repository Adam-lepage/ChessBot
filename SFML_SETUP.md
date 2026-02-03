# SFML Installation Guide for Windows with MSYS2

## Option 1: Using MSYS2 Package Manager (Recommended)

1. Open MSYS2 MinGW 64-bit terminal (not PowerShell)
2. Run the following command:
   ```
   pacman -S mingw-w64-x86_64-sfml
   ```
3. Type `y` and press Enter to confirm installation

## Option 2: Manual Installation

1. Download SFML 2.5.1 for Windows from: https://www.sfml-dev.org/download.php
2. Extract to a known location (e.g., `C:\SFML-2.5.1`)
3. Update your Makefile with the SFML path:
   ```makefile
   SFML_DIR = C:\SFML-2.5.1
   CPPFLAGS = -I$(SFML_DIR)\include -Iinclude
   LDFLAGS = -L$(SFML_DIR)\lib -lsfml-graphics -lsfml-window -lsfml-system
   ```

## Verifying Installation

After installation, test if SFML is available:
```powershell
pkg-config --cflags --libs sfml-graphics
```

If this returns paths instead of an error, SFML is properly installed.

## Building the Project

Once SFML is installed, simply run:
```powershell
make
```

Or to build and run:
```powershell
make run
```

## Troubleshooting

If you get "SFML not found" errors:
1. Ensure you're using MSYS2's g++ compiler (not a different MinGW installation)
2. Verify SFML installation by running: `pkg-config --list-all | grep sfml`
3. If using manual installation, verify paths in the Makefile match your SFML directory

## Using WSL Alternative

If you have WSL (Windows Subsystem for Linux) installed, you can use the Linux terminal in VS Code:
1. Switch to the WSL terminal
2. Install SFML: `sudo apt-get install libsfml-dev`
3. Run: `make`

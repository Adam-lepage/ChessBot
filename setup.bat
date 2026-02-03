@echo off
REM Setup script for Chess Game - Installs dependencies and builds

echo Installing SFML in WSL...
wsl -d Ubuntu -- bash -ic "sudo apt-get update && sudo apt-get install -y libsfml-dev make build-essential"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SFML installation complete!
    echo.
    echo Building Chess Game in WSL...
    wsl -d Ubuntu -- bash -ic "cd /mnt/c/CodingProjects/ChessGame && make clean && make"
) else (
    echo.
    echo Installation failed. Please install SFML manually.
    echo See SFML_SETUP.md for instructions.
)

pause

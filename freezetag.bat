@echo off
:: --- CONFIGURATION ---
:: Point this to where your main Steam application lives (C: drive)
set "STEAM_EXE=C:\Program Files (x86)\Steam\steam.exe"

:: Point this to where the specific mod folder is (E: drive)
set "MOD_PATH=E:\Program Files (x86)\Steam\steamapps\common\Quake 2\rerelease\freezetag"

:: --- CHECKS ---
if not exist "%MOD_PATH%" (
    echo.
    echo ERROR: Mod folder not found at:
    echo "%MOD_PATH%"
    echo.
    echo Please check that you copied the 'freezetag' folder correctly.
    pause
    exit /b
)

:: --- LAUNCH ---
:: We use Steam to launch the game (AppID 2320) to avoid the "Application Load Error"
:: Steam automatically handles finding the game on the E: drive.
echo Launching Quake 2 Freeze Tag...

start "" "%STEAM_EXE%" -applaunch 2320 +set game freezetag +exec freeze.cfg +map q2dm1 +set deathmatch 1 +set teamplay 1 +set developer 1 +set cheats 1 +set bot_numBots 7

exit
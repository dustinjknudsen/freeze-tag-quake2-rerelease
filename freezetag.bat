@echo off
set Q2DIR=C:\Program Files (x86)\Steam\steamapps\common\Quake 2
set "MODDIR=%Q2DIR%\rerelease\freezetag"

if not exist "%MODDIR%" (
	echo "%MODDIR% not found"
	pause
	exit /b
)

start "" /D "%Q2DIR%" quake2.exe +set game freezetag +exec freeze.cfg +map ndctf0 +set deathmatch 1 +set teamplay 1 +set developer 1 +set cheats 1 +set bot_numBots 20

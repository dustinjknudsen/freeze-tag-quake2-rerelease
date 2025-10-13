## Install

Copy these files to C:\Program Files (x86)\Steam\steamapps\common\Quake 2\rerelease\freezetag

## Play

Run C:\Program Files (x86)\Steam\steamapps\common\Quake 2\rerelease\freezetag\freezetag.bat, or when running Quake 2, type "game freezetag"

Go to Multiplayer > Local/Online > 
	Change Settings > 
		Game Mode > Team Deathmatch
		Automatic Teams > Off
		Bot Settings > Minimum Players > 8

## How To Play

Freeze Tag is a game modification for Quake 2 that changes how Team Deathmatch is played.

In Freeze Tag, when you kill someone, they are frozen and cannot respawn. They're waiting for someone to come rescue them. You can rescue a team member by standing next to their body for three seconds. You will see the ice flicker and hear a steam noise. They should break and they're now free to respawn.

If you can freeze the entire enemy team, they will all break and be free, however your team will have scored a Team Point.

Freeze Tag comes with a nice off-hand grapple you can use if you bind to it. Here is how I bound mouse2 to the hook:

alias +hook "hook action;wait;hook shrink"
alias -hook "hook stop"
bind mouse2 +hook

The grapple can pull frozen bodies to you! Great if you want to grab your friend who's being guarded by the enemy.

Also, bots know how to unfreeze teammates so look out!

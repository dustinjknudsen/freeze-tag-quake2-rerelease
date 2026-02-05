# Freeze Tag for Quake 2 Rerelease (Enhanced Edition)

![Freeze Tag Logo](https://darrellbircsak.com/wp-content/uploads/2016/03/getfreeze.gif) [![View Freeze Tag on ModDB](https://button.moddb.com/rating/medium/mods/68289.png)](https://www.moddb.com/mods/freeze-tag)

A modernized Freeze Tag mod for the 2023 Quake II Rerelease (KEX Engine), based on [Darrell Bircsak's original Freeze Tag](https://github.com/dbircsak/freeze-tag-quake2-rerelease).

---

## What Is Freeze Tag

Shoot the enemy team. When they die, they "freeze" and cannot respawn. Kill everyone on the team to score a point and everyone will "thaw" and be released to spawn in again. If you find your frozen teammate, stand by them for three seconds to "thaw" them so they can respawn again. Bonus strategy: there's a grappling hook that can pull you around as well as pull people to you.

[![YouTube](https://img.youtube.com/vi/GMu3_jjiyr4/hqdefault.jpg)](https://www.youtube.com/watch?v=GMu3_jjiyr4)

---

## Features Added in This Fork

- **Ghost Entity Chase Camera** - Frozen players can now spectate their frozen body in third-person using a proxy entity system (fixes the original chase cam issues)
- **4-Team Support** - Expanded beyond 2 teams for larger, more chaotic matches
- **Enhanced Scoreboard** - Custom HUD displays for freeze tag stats
- **Grappling Hook Physics Improvements** - Refined hook mechanics
- **Configuration File** - New `freeze.ini` for easy customization
- **Build System Updates** - Added fmt library and jsoncpp for modern C++ development

---

## Installation

1. Download the latest release
2. Extract to `%USERPROFILE%\Saved Games\Nightdive Studios\Quake II\freezetag`
3. Launch with `+set game freezetag` or type `game freezetag` in console

---

## Building from Source

Requires Visual Studio 2019 or 2022.

1. Clone this repository
2. Open `src/game.vcxproj` in Visual Studio
3. Build in Release mode
4. Copy the compiled DLL to your mod folder

---

## Credits

- **Darrell Bircsak (Doolittle)** - Original Freeze Tag creator and KEX port
- **Dustin Knudsen** - Enhanced edition features (chase cam fix, 4-team support, scoreboard)
- **Perecli Manole** - Grapple hook code
- **id Software / Nightdive Studios** - Quake II Rerelease

---

## Links

- [Original Freeze Tag History](https://darrellbircsak.com/2016/03/25/freeze-tag-reminisced/)
- [Darrell's KEX Port](https://github.com/dbircsak/freeze-tag-quake2-rerelease)
- [Quake II Rerelease Announcement](https://bethesda.net/en/article/6NIyBxapXOurTKtF4aPiF4/enhancing-quake-ii)

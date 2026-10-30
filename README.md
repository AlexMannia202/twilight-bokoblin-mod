# Twilight Bokoblin Mod — experimental guard prototype

Targets **DuskLight v2.0.3**.

## Current experimental changes

- Shortens the Bokoblin's existing close-in/find wind-up.
- Refreshes the vanilla follow-up window so its original AI can chain attacks when Link remains close and in front.
- Adds an occasional short **visual guard pose experiment**. During a randomly triggered guard window, the mod changes the existing rotation applied to the Bokoblin's weapon-bearing joint (`0x0C`). This is a first-pass angle test, not a confirmed polished block animation.
- Does not intentionally change health, damage, or attack hitboxes.

## What needs in-game testing

The source confirms that joint `0x0C` is the joint whose transform receives the vanilla weapon-ground-angle adjustment, and the separate weapon model is attached to the Bokoblin model. The experimental guard uses that same joint path. The chosen rotation (`0x1800`) is a starting point; the visible direction and amount must be checked in DuskLight. If the weapon twists the wrong way or clips through the body, the angle/sign needs tuning. This is **not yet verified in-game** and should be treated as an experimental build.

The guard is visual only in this iteration: it does not grant invulnerability or cancel Link's hit detection. It is meant to let us inspect the weapon pose first, as requested.

## Build

Requirements: CMake 3.26+, a C++20 toolchain, Git, and an internet connection for the first configure (DuskLight is fetched by CMake).

```sh
cmake -B build
cmake --build build --parallel
```

The expected output is `build/mods/twilight_bokoblin_mod.dusk`.

Install on Windows by copying the `.dusk` file to:

`%APPDATA%\TwilitRealm\Dusklight\mods`

Then enable it in DuskLight's mod manager and reload.

## Status

This archive contains source code, **not a compiled `.dusk` bundle**. It has not yet been compiled or tested in-game. Keep a backup of your saves and report what the weapon looks like during the guard pose so the angle can be adjusted.

# Diesel3LightingFix

Fixes excessively dark lighting in PAYDAY 2's native D3D11 renderer after the Diesel 3.0 update.

## What this fixes

After the Diesel 3.0 update, some PAYDAY 2 levels can appear much darker than intended when using the native Direct3D 11 renderer.

The issue was reproduced on maps including:

- No Mercy
- Ukrainian Prisoner

This plugin fixes the affected D3D11 texture resource state so that lighting is rendered normally again.

## Requirements

- PAYDAY 2
- 64-bit SuperBLT

This plugin is intended for the native D3D11 renderer.

## Installation

1. Install 64-bit SuperBLT.
2. Download the release of this mod.
3. Extract the `Diesel3LightingFix` folder into:

   `PAYDAY 2/mods/`

The final folder structure should look like this:

    PAYDAY 2/
    └── mods/
        └── Diesel3LightingFix/
            ├── mod.txt
            ├── main.lua
            └── Diesel3LightingFix.dll

4. Launch PAYDAY 2 normally.

## How it works

Diesel 3.0 can create small D3D11 texture resources with a `MinLOD` value that leaves no valid mip level available for sampling.

This plugin hooks the relevant D3D11 resource functions and:

- prevents the problematic `MinLOD` state from being applied to affected textures;
- repairs already affected textures before they are sampled by the pixel shader.

The fix is intentionally limited to the texture pattern that was observed during debugging.

## Tested

Confirmed working with:

- Native Direct3D 11
- No Mercy
- Ukrainian Prisoner

The fix was developed and tested on an NVIDIA RTX 3080.

## Troubleshooting

If the lighting is still incorrect:

1. Make sure 64-bit SuperBLT is installed and working.
2. Make sure `Diesel3LightingFix.dll` is inside the mod folder.
3. Check the SuperBLT log for:

    Diesel3LightingFix: hooks installed
    [Diesel3LightingFix] Native plugin loaded

If the game crashes or the fix does not work, include the SuperBLT log when reporting the issue.

## License

GPL-2.0

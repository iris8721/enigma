# enigma

Interactive Enigma machine simulator written in C++20 with raylib. Single source file (`enigma.cpp`, ~1100 lines).

Simulates three machine variants with historically accurate wiring:

- **Enigma I** — 3 rotors chosen from I–V, reflectors UKW-A/B/C
- **M3 Navy** — 3 rotors chosen from I–VIII (VI–VIII have dual notches), reflectors UKW-A/B/C
- **M4 U-boat** — 4 rotors: thin Beta/Gamma rotor plus 3 from I–VIII, thin reflectors UKW-B/C

Rotor wirings, notch positions, ring settings (Ringstellung), plugboard (Steckerbrett, up to 13 pairs), and the double-stepping anomaly are all implemented faithfully. The M4's leftmost thin rotor never steps, matching the real machine.

## Features

- **Animated signal path** — each keypress traces the signal step-by-step through plugboard → rotors (right to left) → reflector → rotors (left to right) → plugboard → lamp, with an adjustable speed slider
- Full machine UI: lampboard, keyboard, rotor windows, plugboard with color-coded pairs, and a configuration panel
- Live message in/out display; backspace deletes a character *and* restores the rotor positions to before that keypress

## Controls

| Input | Action |
|---|---|
| `A`–`Z` | Encode a letter (triggers the path animation) |
| `Backspace` | Delete last character, rewind rotor positions |
| Click plugboard letter, then another | Connect a plug pair |
| Click a plugged letter twice | Remove its pair |
| Rotor dropdowns | Change rotor order (duplicates rejected) |
| `+` / `-` under positions / ring settings | Adjust rotor position and Ringstellung |
| Machine / reflector buttons | Switch Enigma I / M3 / M4 and reflector |
| Speed slider | Animation speed (0.1x–2.0x) |
| `clear plugs` / `reset all` | Reset plugboard / reset positions and message |

## Build

Visual Studio project (`enigma.slnx` / `enigma.vcxproj`), C++20, toolset v145. Requires raylib headers and libs — the project assumes raylib is on the default include/library path (e.g. via vcpkg or `Microsoft.Cpp.user.props`); otherwise add its include/lib directories to the project and link `raylib.lib`.

`Roboto-Medium.ttf` must sit next to the executable at runtime (falls back to the default raylib font if missing).

Build unverified on this machine — no Windows/MSVC toolchain available.

## Notes

- Everything lives in one file by design: rotor tables, machine logic, and all rendering.
- `EncodingPath` records every intermediate letter index during `encode()`, which is what drives the animation — the UI replays the recorded path rather than re-simulating.

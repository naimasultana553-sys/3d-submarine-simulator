# 3D Submarine Simulator (OpenGL / freeGLUT)

A fully interactive **3D Submarine Simulator** built with **OpenGL + GLUT/freeGLUT** in
Code::Blocks (single-file C++). Operate a military-style submarine from surface
sunset run → dive → deep-sea exploration, from both **interior** and **360° exterior**
cameras.

No game engine. Just OpenGL primitives, transforms, lighting, fog and animation.

## Flow

`Ocean surface intro` → `Dive animation` → `Menu` → `Mission (interior / exterior)`

## Features

- **Intro scene** — sunset sky, low sun + glitter path, clouds, animated wave
  surface, distant coastline hills, bow-wave foam, crew silhouettes on deck/sail
- **Military-style hull** (from primitives) — long pressure hull, round sonar bow,
  swept conning tower with periscope + comms/radar masts, twin bow searchlights
  with glow, bow/sail planes, cruciform stern rudder, 7-blade bronze screw
- **7 cameras** — 360° orbit exterior (drag + wheel zoom), interior control room,
  front / left / right windows, control screen, free camera (`C` cycles)
- **Interior control room** — consoles, buttons, gauges, pipes, seats, portholes,
  4 animated crew members
- **Deep sea** — 60 fish + jellyfish, seaweed, rocks, corals, rising bubbles,
  floating particles, sandy floor
- **Lighting** — bright day sun (surface) → dark ambient + exponential fog (deep),
  spotlight headlights with visible cone (`L` toggles)
- **Controls** — full 6-DOF: forward/back, yaw, pitch, roll; propeller speed
  follows throttle; emergency stop
- **HUD + missions** — depth, speed, light/engine status, camera, mission timer;
  5 missions advance by depth/position

## Controls

| Key | Action |
|---|---|
| `W` / `S` | Forward / backward |
| `A` / `D` | Turn left / right |
| `R` / `F` | Pitch up / down |
| `Q` / `E` | Roll left / right |
| `Arrows` | Forward/back + turn |
| `L` | Headlights on/off |
| `C` | Cycle camera (7 modes) |
| `V` / `I` | Exterior / interior view |
| `Space` | Emergency stop |
| `ESC` | Menu |
| Mouse drag / wheel | Orbit / zoom (exterior) |

Menu: `Up/Down` + `Enter`. `Enter` on the intro skips it.

## Build (Code::Blocks, Windows)

1. Install Code::Blocks with MinGW + **freeGLUT**
   (headers in `MinGW\include\GL`, lib `freeglut` linked; put
   `freeglut.dll` next to the `.exe`).
2. Open `submarine.cbp` → **Build → Build and Run**.
3. Linked libs (already in `.cbp`): `opengl32 glu32 freeglut gdi32 winmm`.

## Build (command line, MinGW)

```bat
g++ submarine.cpp -o submarine.exe -lopengl32 -lglu32 -lfreeglut -lgdi32 -lwinmm
submarine.exe
```

## Files

| File | Purpose |
|---|---|
| `submarine.cpp` | Entire simulator (~2300 lines, single file) |
| `submarine.cbp` | Code::Blocks project (Debug/Release) |
| `README.md` | This file |

## Computer-graphics concepts demonstrated

3D modeling from primitives · translation / rotation / scaling · orbit +
first-person + free cameras · perspective projection · depth buffering ·
ambient / diffuse / specular / spotlight lighting · fog · blending
(transparency, glow) · frame-based animation · keyboard + mouse interaction.

## Screenshots

> Add yours: run → intro surface shot, exterior orbit underwater with
> headlights on, interior control room.

## License

MIT — free for coursework and portfolios.

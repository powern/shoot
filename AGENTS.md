# Shooter Project — Full Reference

> **Progress tracking:** [PLAN.md](./PLAN.md) — поточний статус інтеграції Doom MAP01

## Overview
3D multiplayer first-person shooter built entirely on a **self-written 3D engine** ("3Dzavr").  
Author: Иван Ильин (Ivan Ilin).  
Tech: C++20, SFML 2.5.1, OpenGL (optional), UDP networking, CMake.

## Project Structure

```
shooter-0.1.0/
├── Shooter.cpp/h        # Main game class (extends Engine)
├── ShooterConsts.h      # Game-wide constants (HP, speed, file paths, etc.)
├── Source.cpp           # Entry point
├── CMakeLists.txt       # Build config (SFML + OpenGL)
├── AGENTS.md            # This file
│
├── engine/              # *** Self-written 3D engine ***
│   ├── Engine.h/cpp     # Game loop, init, render, debug info
│   ├── Object.h/cpp     # Base scene object (transform, position, attachment system)
│   ├── Mesh.h/cpp       # Triangular mesh (loads .obj, color, visibility)
│   ├── Camera.h/cpp     # Camera projection, sorting, software rendering
│   ├── World.h/cpp      # Scene graph, collision detection, raycasting
│   ├── Consts.h         # Engine constants
│   ├── Triangle.h/cpp   # Triangle with normals, point-in-triangle test
│   │
│   ├── math/            # Linear algebra
│   │   ├── Vec2D.h/cpp  # 2D vector
│   │   ├── Vec3D.h/cpp  # 3D vector (dot, cross, normalized, abs)
│   │   ├── Vec4D.h/cpp  # 4D vector (homogeneous)
│   │   └── Matrix4x4.h/cpp  # 4x4 matrix (rotation, translation, scale, view, inverse)
│   │
│   ├── physics/         # *** Collision detection & response (MODERNIZED) ***
│   │   ├── RigidBody.h/cpp  # GJK + EPA + impulse-based response
│   │   ├── HitBox.h/cpp     # AABB or convex hull for GJK
│   │   └── Simplex.h        # GJK simplex (0-4 points)
│   │
│   ├── animation/       # Timeline-based animation system
│   │   ├── Animations.h     # Aggregator header
│   │   ├── Timeline.h/cpp   # Singleton manager
│   │   ├── Animation.h/cpp  # Base animation class
│   │   ├── Interpolation.h/cpp
│   │   └── *.h              # Concrete: AScale, ARotate, ATranslate, AColor, AFunction, AWait, etc.
│   │
│   ├── network/         # Low-level UDP
│   │   ├── ClientUDP.h/cpp
│   │   ├── ServerUDP.h/cpp
│   │   ├── UDPSocket.h/cpp
│   │   ├── UDPConnection.h/cpp
│   │   ├── MsgType.h/cpp
│   │   └── ReliableMsg.h/cpp
│   │
│   ├── io/              # I/O wrappers
│   │   ├── Screen.h/cpp      # Window, OpenGL, sprite/text drawing
│   │   ├── Keyboard.h/cpp    # Key state tracking
│   │   ├── Mouse.h/cpp       # Mouse displacement tracking
│   │   └── SoundController.h/cpp  # Singleton sound manager
│   │
│   ├── gui/             # UI
│   │   ├── Window.h/cpp      # Menu window with buttons
│   │   └── Button.h/cpp      # Clickable button
│   │
│   └── utils/           # Utilities
│       ├── Time.h/cpp        # Singleton: deltaTime, fps, profiling timers
│       ├── Timer.h/cpp       # Timer class
│       ├── Log.h/cpp         # Logging (console + file)
│       ├── ResourceManager.h/cpp  # Singleton: textures, fonts, sound buffers, .obj meshes
│       └── ObjectController.h/cpp # Simple WASD + mouse controller helper
│
├── player/              # Player logic
│   ├── Player.h/cpp     # Health, ability, weapons, kills/deaths, callbacks
│   └── PlayerController.h/cpp  # WASD, mouse look, jump, slow-mo, camera bob
│
├── weapon/              # Weapon definitions
│   ├── Weapon.h/cpp     # Base: ammo, clip, reload, fire, raycast, spread
│   ├── Gun.h            # 6-clip revolver
│   ├── Ak47.h           # 30-clip automatic
│   ├── Gold_Ak47.h      # 60-clip upgraded Ak47
│   ├── Shotgun.h        # 1-shell, 15 pellets
│   └── Rifle.h          # 1-shot sniper (10000 dmg)
│
├── network/             # Game networking
│   ├── ShooterClient.h/cpp   # UDP client, player sync, bonus sync
│   ├── ShooterServer.h/cpp   # UDP server, broadcast, bonuses
│   └── ShooterMsgType.h/cpp  # Custom message types (Damage, Kill, etc.)
│
├── connect.txt          # Client IP, port, player name
├── server.txt           # Server port
│
├── obj/                 # .obj model files (maps, weapons, characters)
├── textures/            # GUI and background textures
├── sound/               # OGG sound files
└── img/                 # README screenshots
```

## Build System
- **Toolchain:** GCC 16.1.0 (MinGW-w64 UCRT 64-bit) at `WinLibs.POSIX.UCRT...\mingw64\bin\`
- **Dependencies:** SFML 2.6.1 (GCC 13.1.0 MinGW 64-bit shared DLLs), OpenAL, OpenGL (optional)
- **Config:** `CMakeLists.txt` — dynamic SFML on Windows
- **Build:** see commands below
- **SFML_DIR:** `C:/Libraries/SFML/lib/cmake/SFML`
- **SFML install:** Downloaded from official releases (SFML-2.6.1-windows-gcc-13.1.0-mingw-64-bit.zip), extracted to `C:/Libraries/SFML/`

## Engine Architecture

### Engine (`Engine.h/cpp`)
- **Singleton-free** game loop base class
- `create()` → opens screen, calls `start()`, then runs the **fixed-timestep main loop**:
  1. `Time::update()` — delta time
  2. `update()` — game logic override
  3. `Timeline::update()` — animations
  4. `world->stepPhysics(fixedDt)` — **fixed timestep physics**
  5. Render (OpenGL or software rasterizer)
  6. `gui()` — HUD overlay
- Protected members: `screen`, `keyboard`, `mouse`, `world`, `camera`

### Physics System (`engine/physics/`) — MODERNIZED
- **Collision Detection:** GJK (Gilbert-Johnson-Keerthi) on convex hitboxes
- **Collision Resolution:** EPA (Expanding Polytope Algorithm)
- **Integration:** Semi-implicit Euler (symplectic)
- **Response:** Impulse-based with restitution + Coulomb friction
- **Broadphase:** Spatial grid (`PHYSICS_GRID_SIZE = 10.0`)
- **Fixed timestep:** `PHYSICS_FIXED_DT = 1/60`, max 8 substeps
- **RigidBody properties:** mass, inertia, restitution, friction, static flag, angular velocity
- **HitBox:** simple (8-vertex AABB) or detailed (all unique mesh vertices)

### Object System (`engine/Object.h/cpp`)
- Hierarchical scene graph via `attach()` / `unattach()`
- Transform: `_transformMatrix` + `_position` + `_angle` + `_angleLeftUpLookAt`
- No quaternions (uses Euler angles with TODO comment)

### Animation System (`engine/animation/`)
- Singleton `Timeline` manages named animation lists (`AnimationListTag`)
- Template `addAnimation<T>(listName, args...)` for type-safe creation
- Concrete types: ATranslate, ARotate, AScale, AColor, AFunction (lambda), AWait, AAttractToPoint, AShowCreation, ADecompose, etc.

### Networking (`engine/network/`)
- UDP-based client-server with reliable message layer
- `ClientUDP` / `ServerUDP` base classes
- Custom message types for game events

## Gameplay Architecture

### `Shooter` (main game class)
- Inherits `Engine`
- Manages: `Player`, `PlayerController`, `ShooterServer`, `ShooterClient`, `Window` (main menu)
- Callback-based: spawn/remove players, add/remove fire traces, bonuses, weapons
- Networking: reads `connect.txt` and `server.txt`, starts server if localhost

### `Player`
- Extends `RigidBody` (participates in physics)
- Health (0–100), Ability (0–10, slow-mo resource)
- Weapon inventory: vector of `shared_ptr<Weapon>`, selected via `_selectedWeapon`
- Collision callbacks detect bonus pickups
- Weapon callbacks: `fire()`, `reload()`, `selectNextWeapon()`, `selectPreviousWeapon()`

### `PlayerController`
- **WASD** — movement (frame-rate-dependent via `Time::deltaTime()`)
- **Mouse** — look (head angle clamped to ±90°)
- **Space** — jump (velocity impulse)
- **Shift** — slow-mo (drains ability, divides speed by 5)
- **E / Q** or **← / →** — switch weapon
- **R** — reload
- **RMB (Left mouse)** — fire (shotgun adds knockback)
- Camera oscillation while running (horizontal bob + vertical bounce)
- High-speed motion wobble

### `Weapon` & Subclasses
- **Fire:** raycast with spread → damage = `_damage / (1 + distance)`
- **Damage multipliers:** headshot ×2, footshot ×0.5
- **Ammo:** clip + stock, `addAPack()` for bonus ammo
- **Shotgun** fires 15 pellets (overrides `processFire`)
- **Weapon params:**

| Weapon     | Stock | Clip | FireRate | Spread | Damage | Reload |
|------------|-------|------|----------|--------|--------|--------|
| Gun        | 30    | 6    | 0.3s     | 3.0    | 150    | 2.0s   |
| Ak47       | 100   | 30   | 0.1s     | 2.0    | 50     | 3.0s   |
| Gold_Ak47  | 200   | 60   | 0.07s    | 1.0    | 50     | 1.5s   |
| Shotgun    | 15    | 1    | 1.0s     | 5.0    | 50×15  | 1.0s   |
| Rifle      | 5     | 1    | 1.0s     | 0.5    | 10000  | 1.0s   |

### Networking (`network/ShooterClient/Server`)
- **Server:** broadcasts player states, manages bonuses, processes damage/kills
- **Client:** sends input/position, receives world state, spawns/removes enemies
- **Custom packets:** Damage, Kill, FireTrace, InitBonuses, AddBonus, RemoveBonus, ChangeWeapon
- **Config:** `NETWORK_WORLD_UPDATE_RATE = 30`, `NETWORK_VERSION = 3`

## Recent Physics Modernization (2026-07-06)
1. **Semi-implicit Euler** — velocity updated before position
2. **Restitution + Friction** — bounce and Coulomb friction in collision response
3. **Angular velocity** — `_angularVelocity` + `_angularAcceleration`
4. **Static bodies** — map objects set `setStatic(true)` (zero inverse mass)
5. **Spatial grid broadphase** — O(n²) → ~O(n) for typical scenes
6. **Contact caching** — `ContactPair` list with warm starting (plumbing in place)
7. **Fixed timestep** — `PHYSICS_FIXED_DT = 1/60` accumulator in `Engine::create()`
8. **AABB** — `getAABB()` computed from world-space triangle vertices
9. **Inertia** — approximated from bounding box extents

## Controls
| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Space | Jump |
| Shift | Slow-mo (hold) |
| E / → | Next weapon |
| Q / ← | Previous weapon |
| R | Reload |
| Escape | Pause / Menu |
| Tab | Debug overlay |
| O | Toggle OpenGL |
| P / L | Start/stop render |

## Game Constants
| Constant | Value |
|----------|-------|
| GRAVITY | 35 |
| HEALTH_MAX | 100 |
| ABILITY_MAX | 10 |
| JUMP_HEIGHT | 3 |
| WALK_SPEED | 10 |
| MOUSE_SENSITIVITY | 0.001 |
| SLOW_MO_COEFFICIENT | 5 |
| FIRE_DISTANCE | 1000 |

## Build Commands
```powershell
# Configure
mkdir build; cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER="<mingw64>/bin/gcc.exe" -DCMAKE_CXX_COMPILER="<mingw64>/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="<mingw64>/bin/mingw32-make.exe" -DSFML_DIR="C:/Libraries/SFML/lib/cmake/SFML"

# Build
mingw32-make -j4

# Run (DLLs copied to output dir by post-build step)
./shooter.exe
```

# wxGLUbuntuTouchCarousell

Ubuntu Touch-style 3D carousel widget built with **wxWidgets** and **OpenGL** (immediate mode). Features a coverflow-like card layout with inertial drag, snap-to-item settling, and **160+ display modes** ranging from classic to experimental.

## Features

- Header-only design (`Carousel.h` + `main.cpp`)
- 160+ display modes (Coverflow, Flat, Stack, Wheel, Fan, Tilted, Spiral, Wave, Carousel3D, Cylinder, Helix, Flip, and many more)
- Inertial scrolling with friction and snap-to-item
- Keyboard (←/→), mouse drag, and scroll wheel navigation
- Interactive display mode picker via `wxChoice` dropdown
- Dynamic card textures rendered with wxGraphicsContext
- Subtle backdrop glow effect
- ~60 FPS animation via `wxTimer`

## Building

### Requirements

- CMake 3.16+
- wxWidgets (core, base, gl components)
- OpenGL

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

Run the executable and browse through the items using mouse drag, scroll wheel, or arrow keys. Use the dropdown at the top to switch between display modes.

```cpp
CarouselCanvas* car = new CarouselCanvas(parent);
car->AddItem("Music",  wxColour(79, 209, 197));
car->AddItem("Videos", wxColour(255, 159, 67));
// ...
```

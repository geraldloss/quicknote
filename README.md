# QuickNote

A simple system-wide note editor with the following features:
- Global shortcut to show/hide the window
- Multi-language interface (EN, DE, FR, ES, IT, CN)
- Unlimited undo/redo history
- Customizable colors
- Tray icon integration
- Single-instance application

## Build Requirements

- Qt6 (Core, Widgets, Network)
- CMake (3.10 or higher)
- zlib
- C++ compiler with C++17 support
- Git (for downloading QHotkey dependency)

## Build Instructions

1. Create a build directory and navigate into it:
bash
mkdir build
cd build

2. Configure the project using CMake:
bash
cmake ..

3. Build the project:
bash
make
The executable `quicknote` will be created in the build directory.

4. Run the application:

bash
./QuickNote

## Installation

You can copy the `quicknote` executable to a directory in your PATH, for example:

bash
sudo cp quicknote /usr/local/bin

## Usage

Run `quicknote` from the command line to start the application.

With ScrollLock pressed, the application will be shown/hidden.
With Ctrl+Z pressed, you undo the last action.
With Ctrl+Y pressed, you redo the last action.

All data will be saved in the user's home directory in the `~/.local/share/quicknote/` folder.


## Configuration

Press right mouse button to enter the settings menu.

All settings are saved in the `~/.config/quicknote/settings.conf` file.
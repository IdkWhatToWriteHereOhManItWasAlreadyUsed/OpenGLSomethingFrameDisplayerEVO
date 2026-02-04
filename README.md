# OpenGLSomethingFrameDisplayerEVO

**A Beautiful and Modern 3D Display for RGB Data**

[![Linux](https://img.shields.io/badge/Linux-supported-success)](https://ubuntu.com/)
[![macOS](https://img.shields.io/badge/macOS-supported-success)](https://www.apple.com/macos/)
[![Windows](https://img.shields.io/badge/Windows-coming_soon-blue)](https://www.microsoft.com/windows/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3%2B-orange)](https://www.opengl.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

## 🌟 About

**OpenGLSomethingFrameDisplayerEVO** is a graphical API for displaying RGB data in a Minecraft-like world. Currently, only chunk mode is available, but additional display modes will be added in the future.

> 🔮 **RGBA support coming soon!**

## 📋 Requirements

### Platforms
- ✅ **Linux** (fully supported)
- ✅ **macOS** (fully supported)
- ⏳ **Windows** (coming soon)

### Dependencies
- **OpenGL** (scene rendering)
- **GLFW** (window creation)

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/OpenGLSomethingFrameDisplayerEVO.git
```

### 2. CMake Integration
Add to your `CMakeLists.txt`:

```cmake
# Add subdirectory
add_subdirectory(OpenGLSomethingFrameDisplayerEVO)

# Link the library
target_link_libraries(your_project_name 
    PRIVATE 
    OpenGLSomethingFrameDisplayerEVO
)

# Copy resources
copy_opengl_resources(${PROJECT_NAME})
```

### 3. Build the Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 📖 API Usage

### Include the Header
```cpp
#include "OpenGLSomethingFrameDisplayerEVO.h"
```

### Basic Example

```cpp
// Create a global instance (important for multi-threading)
OpenGLSomethingFrameDisplayerEVO::OpenGLSomethingFrameDisplayerEVO displayer;

// Set video size BEFORE initialization
displayer.SetVideoSize(480, 360);

// Initialize the engine (window size: 1280x720)
displayer.InitialiseGame(1280, 720);

// Start the engine
displayer.Start();

// Display a frame from another thread
std::vector<uint8_t> frame_data(width * height * 3); // RGB data
displayer.DisplayFrame(frame_data.data());
```

## ⚙️ Advanced Usage

### Thread Management
```cpp
// Specify number of threads for the engine
// Consider your application's total thread usage
displayer.SetThreadCount(4);
```

### Thread Synchronization
```cpp
// Thread A: Set video size
displayer.SetVideoSize(640, 480);

// Thread B: Wait for video size to be set
displayer.WaitForSetVideoSize();

// Thread C: Wait for engine initialization
displayer.WaitForGameInit();
```

### macOS Specifics
```cpp
// IMPORTANT: On macOS, initialization and running must
// occur in the main thread of the process
int main() {
    // Initialization and start MUST be here
    displayer.InitialiseGame(800, 600);
    displayer.Start();
    return 0;
}
```

## 🎮 Engine Window Controls

### Basic Controls
1. **Take control**: Hover over window and press `Left Ctrl`
2. **Movement**: `W` `A` `S` `D`
3. **Camera rotation**: Mouse movement
4. **Exit**: `Esc` (with cursor captured) or window close button

### Scene Adjustment
- Unlock mouse (`Left Ctrl`)
- Use ImGui panel for real-time scene adjustments
- Modify lighting, textures, and other parameters

## ⚠️ Important Notes

### Limitations
- ❌ **Video resizing not supported** (and probably never will be)
- 📏 **Recommended video size**: up to 480×360
- 🔧 **Not thread-safe**: `DisplayFrame` can be called from any thread, but not concurrently

### Performance Recommendations
- Use small video resolutions (up to 480×360)
- Optimally distribute threads between your app and the engine
- Increase engine thread count for higher resolutions

## 🎯 Usage Examples

### Video Player (badPlayerMax)
Full-featured video player using FFmpeg and OpenGL:
[GitHub Repository](https://github.com/IdkWhatToWriteHereOhManItWasAlreadyUsed/badPlayerMax)

### 3D Model Visualizer (with SDL2)
*Example coming soon*

### Resource Structure
```
build/
├── your_app
├── res/
│   ├── shaders/
│   ├── textures/
│   └── blocks/
```

## 🤝 Contributing

We welcome contributions!
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📄 License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

**Enjoy!** 🚀

For questions or suggestions, please create an issue in the project repository.
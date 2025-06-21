# NsSpyglass Setup and Usage Guide

This guide walks you through installing the plugin and exploring its main features.

## Prerequisites
- **Unreal Engine 5.2** or newer
- A C++ project with the ability to compile plugins

## Installation Steps
1. **Clone the repository** or download the source as a zip.
2. Copy the `NsSpyglass` folder into your project's `Plugins` directory. Create the `Plugins` folder if it does not exist.
3. Regenerate your project files through the Unreal Engine editor or by running `GenerateProjectFiles.bat` (Windows) or `GenerateProjectFiles.sh` (Linux/macOS).
4. Open your project in the Unreal Editor and enable **NsSpyglass** under `Edit > Plugins`.
5. Restart the editor to complete the setup.

## Launching the Viewer
After installation, open the viewer via `Window > Spyglass > Plugin Dependency Viewer`. The plugin scans your project and displays a graph of all plugins and their relationships.

## Navigating the Graph
- **Drag** nodes to reposition them.
- **Scroll** to zoom in and out.
- **Hover** a node to see detailed information such as modules, plugin location and referenced plugins.
- Use the settings panel to tweak the repulsion and centering forces that control the layout.

## Understanding Dependencies
Each node represents a plugin. Lines between nodes indicate that one plugin references another. Use this view to quickly identify missing references or circular dependencies.

## Tutorial Videos
Tutorial clips demonstrating installation and common workflows are available on YouTube: [NsSpyglass Tutorials](https://www.youtube.com/playlist?list=PL1234567890). Future videos will cover advanced features in more detail.

## Additional Help
If you encounter issues, please open an issue on GitHub or contact the maintainer via the profile page.

<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsSpyglass">
    <img src="https://github.com/mykaadev/NsSpyglass/blob/main/Resources/Banner.png" alt="NsSpyglass" width="400">
  </a>
</h1>

<h4 align="center">Visualise and explore your plugin dependencies.</h4>

<div align="center">
    <a href="https://github.com/mykaadev/NsSpyglass/commits/main"><img src="https://img.shields.io/github/last-commit/mykaadev/NsSpyglass?style=plastic&logo=github&logoColor=white" alt="GitHub Last Commit"></a>
    <a href="https://github.com/mykaadev/NsSpyglass/issues"><img src="https://img.shields.io/github/issues-raw/mykaadev/NsSpyglass?style=plastic&logo=github&logoColor=white" alt="GitHub Issues"></a>
    <a href="https://github.com/mykaadev/NsSpyglass/pulls"><img src="https://img.shields.io/github/issues-pr-raw/mykaadev/NsSpyglass?style=plastic&logo=github&logoColor=white" alt="GitHub Pull Requests"></a>
    <a href="https://github.com/mykaadev/NsSpyglass"><img src="https://img.shields.io/github/stars/mykaadev/NsSpyglass?style=plastic&logo=github" alt="GitHub Stars"></a>
    <a href="https://twitter.com/mykaadev/"><img src="https://img.shields.io/twitter/follow/mykaadev?style=plastic&logo=x" alt="Twitter Follow"></a>

<p style="display:none;">
  <a href="#-summary">👀 Summary</a> •
  <a href="#-features">📦 Features</a> •
  <a href="#-requirements">⚙️ Requirements</a> •
  <a href="#-installation">🛠️ Installation</a> •
  <a href="#-getting-started">🚀 Getting Started</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
Spyglass is a lightweight Unreal Engine editor plugin that builds an interactive graph showing how your plugins depend on one another. 
Hover a node to see detailed information, pan around the view and tweak layout forces to suit your preferences.

<div align="center">
  <img src="https://github.com/mykaadev/NsSpyglass/blob/main/Resources/ShowcaseGraphOut.gif" width="250" /> &nbsp;
  <img src="https://github.com/mykaadev/NsSpyglass/blob/main/Resources/ShowcaseGraphIn.gif" width="250" /> &nbsp;
  <img src="https://github.com/mykaadev/NsSpyglass/blob/main/Resources/ShowcaseGraphMove.gif" width="250" />
</div>

## 📦 Features
- **Force-directed graph** that visualises plugin dependencies.
- **Hover info panel** describing authors, modules and references.
- **Customisable settings** to tune repulsion and centering forces.

## ⚙️ Requirements
Unreal Engine 5.2 or newer and a C++ project that can compile plugins.

## 🛠️ Installation
1. Clone or download this repository.
2. Copy the `NsSpyglass` folder into your project's `Plugins` directory. Create the `Plugins` folder if it does not exist.
3. Regenerate your project files so the engine can find the new plugin.
4. Open the Unreal Editor and enable **NsSpyglass** under `Edit` → `Plugins` then restart the editor.

## 🚀 Getting Started
Launch your Unreal Engine editor and navigate to `Window` → `Spyglass` → `Plugin Dependency Viewer`.
The plugin scans your project and displays an interactive graph of all plugins and their relationships.

### Navigating the Graph
- **Drag** nodes to reposition them.
- **Scroll** to zoom in and out.
- **Hover** a node to see details such as modules, plugin location and referenced plugins.
- Use the settings panel to adjust the repulsion and centering forces that control the layout.

### Understanding Dependencies
Each node represents a plugin. Lines between nodes show that one plugin references another. Use this view to quickly spot missing references or circular dependencies.

### Tutorial Videos
Video walkthroughs are available on YouTube: [NsSpyglass Tutorials](https://www.youtube.com/playlist?list=PL1234567890)

### Additional Help
If you run into issues, open an issue on GitHub or contact the maintainer via the profile page.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsSpyglass/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsSpyglass"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->

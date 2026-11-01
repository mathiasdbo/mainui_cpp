# Resonance3D fork

This is [FWGS/mainui_cpp](https://github.com/FWGS/mainui_cpp) with the
changes [Resonance3D](https://github.com/mathiasdbo/Resonance3D) needs to run the menu on the Original Xbox, where
the whole game shares 64 MB with the GPU. It is pulled in as
`3rdparty/mainui`, from the `resonance3d` branch.

Examples of what it changes on the console: a font atlas cache stored as an
8-, 16- or 32-bit TGA, fonts released when a map load starts and restored
afterwards, a glow behind the focused item, and the audio menu's "Hardware
audio" and "Voices volume" controls. The PC build is not changed. Each
deviation is in Resonance3D's [`docs/r3d/divergences.md`](https://github.com/mathiasdbo/Resonance3D/blob/main/docs/r3d/divergences.md). Upstream's README follows.

# mainui_cpp

Main Menu UI for Xash3D FWGS engine, built entirely in C++.

Key features:
* Custom font renderer with multiple backends based on FreeType, stb_truetype and GDI on Windows
* Unicode and l10n support
* Completely self-contained, just clone this repo (with --recursive) and add to your mod
* Complex widget containers system, ability to put widget into containers into container
* Window system as a continuation of containers system
* Client-side menus (needs to be documented)

### Including mainui_cpp in your mod

1. Clone this repo: `git clone --recursive https://github.com/FWGS/mainui_cpp` to your mod source code tree.
2. If you use:
* CMake: invoke `add_subdirectory(mainui_cpp)` in your CMakeLists.txt file
* Waf/WAiFu: invoke `ctx.add_subdirectory('mainui_cpp')` in your wscript file
* Visual Studio: if you want to build it from your mod tree, include `vs2022/mainui_cpp.vcxproj` to your mod solution, or use `vs2022/mainui_cpp.sln`.
3. Place your built `menu.dll`/`libmenu.so` ALONGSIDE your `client.dll`/`client.so`.
4. Check that everything is working and happy hacking!

If you have any troubles setting this up, create an issue in https://github.com/FWGS/mainui_cpp/issues.

### Notes and restrictions

* mainui_cpp doesn't supports original Xash3D anymore. If it's possible, you can switch to Xash3D FWGS, otherwise you're on your own. I will accept patches to enable other Xash3D forks, but I won't support them on my own.


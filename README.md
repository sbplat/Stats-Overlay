<h1 align="center">
  <br>
  <a href="https://github.com/sbplat/Stats-Overlay">
    <img src="/res/icon.ico" alt="Stats Overlay" width="200">
  </a>
  <br>
  Stats Overlay
  <br>
</h1>

<h4 align="center">A minimalistic stats overlay for <a href="https://hypixel.net/" target="_blank">Hypixel</a> built on top of <a href="https://www.libsdl.org/" target="_blank">SDL</a>.</h4>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#download">Download</a> •
  <a href="#usage">Usage</a> •
  <a href="#building">Building</a> •
  <a href="#contributing">Contributing</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

![screenshot](/images/screenshot.png)

## Features

* Simple to use
* Shows the stats of other players in your game almost instantly
  * Avoid nicked and sweaty players
* Shows stats for BedWars and Mini Walls
* Highly customizable
  * Custom screen size, opacity, background color, scale, font, etc.
* Built in fake full-screen support
* Configurable game-mode to display
* Supports most clients
* Very low CPU and memory usage
  * Written in native C++
* Open source

## Download

You can [download](https://github.com/sbplat/Stats-Overlay/releases/latest) the latest release of this Stats Overlay for Windows. Other platforms are currently not supported yet.

## Usage

To run this program, you will need to first download the [latest release](#download).

Then, extract the zip folder and run `Overlay.exe`. This will create a file called `config.json` inside the assets folder. Close the overlay (x button) and open the JSON config file using any text editor (ex. Notepad). Modify the `config.json` file accordingly by filling in the values (api key, log file path, etc.), save it and reopen `Overlay.exe`. The modifications you have made should take effect immediately.

## Building

If you'd like to build this project from source, you can follow the process shown below.

1. Install [MinGW](https://www.mingw-w64.org/) and [git](https://git-scm.com/).
2. Add the MinGW `bin` directory to your system environment variables.
3. Get the following libraries: [SDL](https://github.com/libsdl-org/SDL), [SDL_ttf](https://github.com/libsdl-org/SDL_ttf), [SDL_image](https://github.com/libsdl-org/SDL_image), [JSON](https://github.com/nlohmann/json), [cpr](https://github.com/libcpr/cpr), [spdlog](https://github.com/gabime/spdlog), [fmt](https://github.com/fmtlib/fmt).
4. Clone this GitHub repository.
```
> git clone https://github.com/sbplat/Stats-Overlay.git
```
5. Navigate to the cloned repository, open the `res` folder and run `compile.bat` to compile the resource script.
6. Compile this project with `mingw32-make`.
```
> mingw32-make [debug/release]
```
7. If the build succeeds, there should be an executable called `Overlay.exe` which is the compiled Stats Overlay!

## Contributing

All feedback, issues and PRs are welcome!

## Credits

This project uses the following open source libraries:

- [SDL](https://github.com/libsdl-org/SDL), [SDL_ttf](https://github.com/libsdl-org/SDL_ttf), [SDL_image](https://github.com/libsdl-org/SDL_image)
- [JSON](https://github.com/nlohmann/json)
- [cpr](https://github.com/libcpr/cpr)
- [spdlog](https://github.com/gabime/spdlog)
- [fmt](https://github.com/fmtlib/fmt)

## License

This project is licensed under the [MIT license](LICENSE).

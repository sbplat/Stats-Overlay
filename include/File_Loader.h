/*
MIT License

Copyright (c) 2022 sbplat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "Player.h"
#include "Types.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>


#ifndef FILE_LOADER_H
#define FILE_LOADER_H

namespace FL {

    namespace JSON = nlohmann;

    inline std::string buildInformationString() {
        std::string appdataEnvPath(std::getenv("USERPROFILE"));

        std::replace(appdataEnvPath.begin(), appdataEnvPath.end(), '\\', '/');

        std::stringstream ss;
        ss << "\n\n"
              "// screenWidth: width of overlay (px)\n"
              "// opacity: opacity of overlay (%)\n"
              "// backgroundHexColor: background color (hex)"
              "// scale: text scale (%)\n"
              "// fileDelay: time before parsing log file again (ms)\n"
              "// cachePlayerTime: time before removing player from cache (s)\n"
              "// renderHeadOverlay: render extra head/face details (true/false)\n"
              "// fakeFullscreen: fake fullscreen support (true/false)\n"
              "// apiKey: Hypixel API key (/api new)\n"
              "// displayMode: mode to display (bw_solos/bw_doubles/bw_threes/bw_fours/bw_overall/miniwalls)\n"
              "// titleFontPath: location of font for the title bar\n"
              "// statsFontPath: location of font for the player stats\n"
              "// minecraftLogPath: location of Minecraft's log path\n";
        ss << "//   Examples:\n";
        ss << "//   Vanilla - " << appdataEnvPath << "/AppData/Roaming/.minecraft/logs/latest.log\n";
        ss << "//   Lunar - " << appdataEnvPath << "/.lunarclient/offline/1.8/logs/latest.log\n";
        ss << "//   Badlion - " << appdataEnvPath << "/AppData/Roaming/.minecraft/logs/blclient/minecraft/latest.log\n";

        return ss.str();
    }

    enum class Mode {
        BEDWARS,
        MINI_WALLS
    };

    inline std::string modeToString(Mode mode) {
        switch (mode) {
            case Mode::BEDWARS:
                return "BedWars";
            case Mode::MINI_WALLS:
                return "Mini Walls";
            default:
                return "???";
        }
    }

    struct Data {
        int screenWidth = 800, opacity = 70, scale = 100, fileDelay = 100, cachePlayerTime = 4 * 60;
        bool renderHeadOverlay = true, fakeFullscreen = true;
        SDL_Color backgroundColor = {50, 50, 50, 255};
        std::string apiKey = "YOUR-HYPIXEL-API-KEY-HERE", displayMode = "bw_overall", minecraftLogPath = "C:/Users/YourName/AppData/Roaming/.minecraft/logs/latest.log",
                    titleFontPath = "./assets/SourceCodePro.ttf", statsFontPath = "./assets/SourceCodePro.ttf";
        Mode mode = Mode::BEDWARS;
    };

    Data config;

    std::string configFilePath = "./assets/config.json";

    std::string information = buildInformationString();

    Data load() {
        spdlog::info("Attempting to load config data");
        config = {};

        std::ifstream fileStream(configFilePath, std::ios::binary | std::ios::app);

        std::stringstream buffer;
        buffer << fileStream.rdbuf();

        std::string loadedData = buffer.str();

        std::replace(loadedData.begin(), loadedData.end(), '\\', '/');

        try {
            spdlog::info("Parsing config data...");
            JSON::json data = JSON::json::parse(loadedData,
                                                /*callback*/ nullptr,
                                                /*allow exceptions*/ true,
                                                /*allow comments*/ true);

            try {
                int screenWidth = data.at("screenWidth");

                if (screenWidth >= 10) {
                    config.screenWidth = screenWidth;
                    spdlog::info("Set screenWidth={}", config.screenWidth);

                } else {
                    spdlog::info("Screen width too small");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load screenWidth");
            }

            try {
                int opacity = data.at("opacity");

                if (0 <= opacity && opacity <= 100) {
                    config.opacity = opacity;
                    spdlog::info("Set opacity={}", config.opacity);

                } else {
                    spdlog::info("Invalid opacity");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load opacity");
            }

            try {
                std::string backgroundHexColor = data.at("backgroundHexColor");

                if (backgroundHexColor.at(0) == '#') {
                    backgroundHexColor.erase(0, 1);
                }

                int backgroundHexValue = std::stoi(backgroundHexColor, nullptr, 16);

                config.backgroundColor = hexToRGB(backgroundHexValue, 255);

                spdlog::info("Set backgroundHexValue=({}, {}, {})", config.backgroundColor.r, config.backgroundColor.g, config.backgroundColor.b);

            } catch (...) {
                // catch all exceptions including:
                // JSON::json::out_of_range, std::invalid_argument, std::out_of_range
                spdlog::warn("Could not load backgroundColor");
            }

            try {
                int scale = data.at("scale");

                if (0 <= scale && scale <= 1000) {
                    config.scale = scale;
                    spdlog::info("Set scale={}", config.scale);

                } else {
                    spdlog::info("Invalid scale");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load scale");
            }

            try {
                int fileDelay = data.at("fileDelay");

                if (fileDelay >= 0) {
                    config.fileDelay = fileDelay;
                    spdlog::info("Set fileDelay={}", config.fileDelay);

                } else {
                    spdlog::info("Invalid fileDelay");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load fileDelay");
            }

            try {
                int cachePlayerTime = data.at("cachePlayerTime");

                if (cachePlayerTime > 0) {
                    config.cachePlayerTime = cachePlayerTime;
                    spdlog::info("Set cachePlayerTime={}", config.cachePlayerTime);

                } else {
                    spdlog::info("Invalid cachePlayerTime");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load cachePlayerTime");
            }

            try {
                std::string renderHeadOverlay = data.at("renderHeadOverlay");

                std::transform(renderHeadOverlay.begin(), renderHeadOverlay.end(), renderHeadOverlay.begin(), [](char &c) {
                    return std::tolower(c);
                });

                if (renderHeadOverlay == "0" || renderHeadOverlay == "f" || renderHeadOverlay == "false" || renderHeadOverlay == "n" || renderHeadOverlay == "no") {
                    config.renderHeadOverlay = false;

                } else {
                    config.renderHeadOverlay = true;
                }

                spdlog::info("Set renderHeadOverlay={}", config.renderHeadOverlay);

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load renderHeadOverlay");
            }

            try {
                std::string fakeFullscreen = data.at("fakeFullscreen");

                std::transform(fakeFullscreen.begin(), fakeFullscreen.end(), fakeFullscreen.begin(), [](char &c) {
                    return std::tolower(c);
                });

                if (fakeFullscreen == "0" || fakeFullscreen == "f" || fakeFullscreen == "false" || fakeFullscreen == "n" || fakeFullscreen == "no") {
                    config.fakeFullscreen = false;

                } else {
                    config.fakeFullscreen = true;
                }

                spdlog::info("Set fakeFullscreen={}", config.fakeFullscreen);

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load fakeFullscreen");
            }

            try {
                std::string apiKey = data.at("apiKey");

                if (apiKey.size() > 0 && apiKey != config.apiKey && MPI::testApiKey(apiKey)) {
                    config.apiKey = apiKey;
                    spdlog::info("Set apiKey={}", config.apiKey);

                } else {
                    spdlog::error("Invalid apiKey");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::error("Invalid apiKey");
            }

            try {
                std::string displayMode = data.at("displayMode");

                std::transform(displayMode.begin(), displayMode.end(), displayMode.begin(), [](char &c) {
                    return std::tolower(c);
                });

                if (displayMode == "bw_overall" || displayMode == "bw_solos" || displayMode == "bw_doubles" || displayMode == "bw_threes" || displayMode == "bw_fours" || displayMode == "miniwalls") {
                    config.displayMode = displayMode;
                    spdlog::info("Set displayMode={}", config.displayMode);

                } else {
                    spdlog::info("Invalid displayMode");
                }

                if (config.displayMode.rfind("bw_", 0) == 0) { // starts with bw_
                    config.mode = Mode::BEDWARS;

                } else if (config.displayMode == "miniwalls") {
                    config.mode = Mode::MINI_WALLS;
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load displayMode");
            }

            try {
                std::string minecraftLogPath = data.at("minecraftLogPath");

                std::ifstream logFile(minecraftLogPath);

                if (logFile.good()) {
                    config.minecraftLogPath = minecraftLogPath;
                    spdlog::info("Set minecraftLogPath={}", config.minecraftLogPath);

                } else {
                    spdlog::error("Invalid minecraftLogPath");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::error("Could not load minecraftLogPath");
            }

            try {
                std::string titleFontPath = data.at("titleFontPath");

                SDL2::TTF_Font titleFont(TTF_OpenFont(titleFontPath.c_str(), 12));

                if (titleFont != NULL) {
                    config.titleFontPath = titleFontPath;
                    spdlog::info("Set titleFontPath={}", config.titleFontPath);

                } else {
                    spdlog::info("Invalid titleFontPath");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load titleFontPath");
            }

            try {
                std::string statsFontPath = data.at("statsFontPath");

                SDL2::TTF_Font statsFont(TTF_OpenFont(statsFontPath.c_str(), 12));

                if (statsFont != NULL) {
                    config.statsFontPath = statsFontPath;
                    spdlog::info("Set statsFontPath={}", config.statsFontPath);

                } else {
                    spdlog::info("Invalid statsFontPath");
                }

            } catch (const JSON::json::out_of_range &e) {
                spdlog::warn("Could not load statsFontPath");
            }

        } catch (const JSON::json::parse_error &e) {
            // corrupt file
            spdlog::error("Could not load from config file:\n{}", e.what());
        }

        return config;
    }

    void write() {
        JSON::ordered_json data;

        data["screenWidth"] = config.screenWidth;
        data["opacity"] = config.opacity;

        std::stringstream ssBgHexColor;
        ssBgHexColor << "#";
        ssBgHexColor << std::hex << (config.backgroundColor.r << 16 | config.backgroundColor.g << 8 | config.backgroundColor.b);
        data["backgroundHexColor"] = ssBgHexColor.str();

        data["scale"] = config.scale;
        data["fileDelay"] = config.fileDelay;
        data["cachePlayerTime"] = config.cachePlayerTime;

        data["renderHeadOverlay"] = config.renderHeadOverlay ? "true" : "false";
        data["fakeFullscreen"] = config.fakeFullscreen ? "true" : "false";

        data["apiKey"] = config.apiKey;
        data["displayMode"] = config.displayMode;
        data["minecraftLogPath"] = config.minecraftLogPath;
        data["titleFontPath"] = config.titleFontPath;
        data["statsFontPath"] = config.statsFontPath;

        std::string formattedData = data.dump(4);
        formattedData += information;

        spdlog::info("Writing back to config file");

        std::ofstream fileStream(configFilePath, std::ios::binary);
        fileStream << formattedData;
    }

}  // namespace FL

#endif  // FILE_LOADER_H

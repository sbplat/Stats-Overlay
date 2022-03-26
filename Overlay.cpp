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

#define SPDLOG_FMT_EXTERNAL
#define JSON_DIAGNOSTICS 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "include/Types.h"

SDL_Color hexToRGB(int hex, int alpha = 255) {
    SDL_Color color;
    color.a = alpha;

    color.r = std::min(255, std::max(0, (hex >> 16) & 0xFF));
    color.g = std::min(255, std::max(0, (hex >> 8) & 0xFF));
    color.b = std::min(255, std::max(0, (hex) & 0xFF));

    return color;
}

SDL_Color lightenRGB(SDL_Color color, float factor) {
    color.r = std::min(255, (int)(color.r + (255 - color.r) * factor));
    color.g = std::min(255, (int)(color.g + (255 - color.g) * factor));
    color.b = std::min(255, (int)(color.b + (255 - color.b) * factor));

    return color;
}

SDL_Color darkenRGB(SDL_Color color, float factor) {
    color.r = std::max(0, (int)(color.r - (255 - color.r) * factor));
    color.g = std::max(0, (int)(color.g - (255 - color.g) * factor));
    color.b = std::max(0, (int)(color.b - (255 - color.b) * factor));

    return color;
}

#include <atomic>

std::atomic<bool> running{true}, renderUpdate{true};

#include "include/Player.h"
#include "include/File_Loader.h"
#include "include/Log_Reader.h"
#include "include/WinAPI_Utils.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <chrono>
#include <sstream>
#include <thread>

const int STATS_SCENE = 1, CONFIG_SCENE = 2;

int maxFPS = 30, refreshTime = 1000 / maxFPS, screenWidth, screenHeight, titleHeight, closeButtonWidth, closeButtonPadding;
float scale, opacity, borderRatio = 0.004, titleRatio = 0.035f, titleFontRatio = 0.75f, statsFontRatio = 0.019f, closeButtonRatio = 0.0162f,
                      configImageRatio = 288.0 / 223.0, closeButtonPaddingRatio = 0.011;
SDL_Color backgroundColor, titleColor;
bool renderHeadOverlay;

SDL2::Window window;
SDL2::Renderer renderer;
SDL2::TTF_Font titleFont, statsFont, symbolsFont;
MPI::PlayerInfoTextures dummyTextInfo;

SDL_Rect titleRectangle, titleTextRectangle, closeButtonRectangle, configImageRectangle, dragAreaRectangle, playerHeadRect = {8, 8, 8, 8}, playerHeadOverlayRect = {40, 8, 8, 8};

int invalidTextureCount = 0;

// Redirect SDL logs to our own log file
void SDLLogOutputFunction(void *userData, int category, SDL_LogPriority priority, const char *message) {
    (void)userData;

    if (priority == SDL_LOG_PRIORITY_VERBOSE) {
        // ignore (too much info)
    } else if (priority == SDL_LOG_PRIORITY_DEBUG) {
        if (std::strcmp(message, "Invalid texture") == 0) {
            if (++invalidTextureCount % 20 == 0) {
                spdlog::debug("SDL ({}): {} (x{})", category, message, invalidTextureCount);
            }

        } else {
            spdlog::debug("SDL ({}): {}", category, message);
        }

    } else if (priority == SDL_LOG_PRIORITY_INFO) {
        spdlog::info("SDL ({}): {}", category, message);

    } else if (priority == SDL_LOG_PRIORITY_WARN) {
        spdlog::warn("SDL ({}): {}", category, message);

    } else if (priority == SDL_LOG_PRIORITY_ERROR) {
        spdlog::error("SDL ({}): {}", category, message);

    } else if (priority == SDL_LOG_PRIORITY_CRITICAL) {
        spdlog::critical("SDL ({}): {}", category, message);

    } else {
        spdlog::warn("SDL unknown priority ({}). Message: {}", priority, message);
    }
}

// Custom hit test so the window can be dragged around
SDL_HitTestResult SDLCALL dragHitTest(SDL_Window *window, const SDL_Point *point, void *data) {
    (void)data;

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    dragAreaRectangle = {0, titleHeight, screenWidth, screenHeight - titleHeight};

    if (SDL_PointInRect(point, &dragAreaRectangle)) {
        return SDL_HITTEST_DRAGGABLE;
    }

    return SDL_HITTEST_NORMAL;
}

void createHeadTexture(SDL2::Texture &skinTexture, std::string &skin) {
    SDL2::RWops rw(SDL_RWFromConstMem(&skin[0], skin.size()));
    SDL2::Surface skinSurface(IMG_Load_RW(rw.get(), 0));
    skinTexture.reset(SDL_CreateTextureFromSurface(renderer.get(), skinSurface.get()));
}

void renderHead(SDL2::Texture &headTexture, int xPos, int yPos) {
    const int headSize = screenWidth * statsFontRatio * 1.2, heightIncrement = screenWidth * statsFontRatio * 1.5;
    SDL_Rect faceRectangle = {xPos, yPos + (heightIncrement - headSize) / 2, headSize, headSize};
    SDL_RenderCopy(renderer.get(), headTexture.get(), &playerHeadRect, &faceRectangle);

    if (renderHeadOverlay) {
        SDL_RenderCopy(renderer.get(), headTexture.get(), &playerHeadOverlayRect, &faceRectangle);
    }
}

void createTextTexture(SDL2::Texture &textTexture, std::string text, SDL2::TTF_Font &font, SDL_Color color = {255, 255, 255, 255}) {
    SDL2::Surface textSurface(TTF_RenderUTF8_Blended(font.get(), text.c_str(), color));
    textTexture.reset(SDL_CreateTextureFromSurface(renderer.get(), textSurface.get()));
}

void renderText(SDL2::Texture &textTexture, int xPos, int yPos) {
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture.get(), NULL, NULL, &textWidth, &textHeight);

    SDL_Rect textRectangle = {xPos, yPos, textWidth, textHeight};
    SDL_RenderCopy(renderer.get(), textTexture.get(), NULL, &textRectangle);
}

void createMultiColorTextTexture(std::vector<SDL2::Texture> &textTextures, std::string text, SDL2::TTF_Font &font, std::vector<SDL_Color> colors) {
    for (unsigned i = 0; i < std::min((unsigned)text.size(), (unsigned)colors.size()); ++i) {
        char character[2] = {text[i], '\0'};  // Convert one character of a std::string into const char*
        SDL2::Surface textSurface(TTF_RenderUTF8_Blended(font.get(), character, colors[i]));
        textTextures.push_back(SDL2::Texture(SDL_CreateTextureFromSurface(renderer.get(), textSurface.get())));
    }
}

int renderMultiTexts(std::vector<SDL2::Texture> &textTextures, int xPos, int yPos) {
    for (const SDL2::Texture &textTexture : textTextures) {
        int textWidth, textHeight;
        SDL_QueryTexture(textTexture.get(), NULL, NULL, &textWidth, &textHeight);

        SDL_Rect textRectangle = {xPos, yPos, textWidth, textHeight};
        SDL_RenderCopy(renderer.get(), textTexture.get(), NULL, &textRectangle);

        xPos += textWidth;
    }

    return xPos;
}

void renderAllTextures(MPI::PlayerInfoTextures &textures, int height, bool hasError = false) {
    int width = screenWidth * statsFontRatio * 0.5;

    if (textures.skin) {
        renderHead(textures.skin, width, height);
    }

    width += screenWidth * statsFontRatio + screenWidth * statsFontRatio * 0.5;

    renderText(textures.username, width, height);
    width += 11 * screenWidth * statsFontRatio;

    if (hasError) {
        renderText(textures.errorMessage, width, height);

    } else {
        renderText(textures.level, width, height);
        width += 5 * screenWidth * statsFontRatio;

        if (textures.stars.singleColor) {
            renderText(textures.stars.single, width, height);
            // render the star symbol
            int starsTextureW, starsTextureH;
            SDL_QueryTexture(textures.stars.single.get(), NULL, NULL, &starsTextureW, &starsTextureH);
            renderText(textures.stars.symbol, width + starsTextureW, height);

        } else {
            int starsTextureXPos = renderMultiTexts(textures.stars.multi, width, height);
            // render the star symbol
            renderText(textures.stars.symbol, starsTextureXPos, height);
        }

        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.FK, width, height);
        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.FD, width, height);
        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.FKDR, width, height);
        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.W, width, height);
        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.L, width, height);
        width += 5 * screenWidth * statsFontRatio;
        renderText(textures.WLR, width, height);
    }
}

std::string to2DPString(float x) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << x;
    return ss.str();
}

int main(int argc, char *args[]) {
    (void)argc;
    (void)args;

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%n/%l] %v");
    spdlog::enable_backtrace(32);
    spdlog::flush_every(std::chrono::seconds(1));
    std::shared_ptr<spdlog::logger> dailyLogger = spdlog::daily_logger_mt("main", "logs/log.txt", 0, 0);
    dailyLogger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(dailyLogger);

    SDL_LogSetOutputFunction(&SDLLogOutputFunction, NULL);

    spdlog::info("Initializing overlay");
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest"); // linear causes the player heads to become blurry
    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        spdlog::critical("Error initializing SDL. Error: {}", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        spdlog::critical("Error initializing SDL TTF. Error: {}", TTF_GetError());
        return 1;
    }

    /*
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        spdlog::critical("Error initializing SDL IMG. Error: {}", IMG_GetError());
        return 1;
    }
    */

    // Load configuration from the JSON config file
    FL::load();
    screenWidth = FL::config.screenWidth, screenHeight = screenWidth / 10, opacity = FL::config.opacity / 100.0, scale = FL::config.scale / 100.0;
    backgroundColor = FL::config.backgroundColor;
    titleColor = darkenRGB(backgroundColor, 0.2);
    spdlog::info("Set screen width={}, opacity={}, scale={}", FL::config.screenWidth, FL::config.opacity, FL::config.scale);
    titleFontRatio *= scale, statsFontRatio *= scale;
    renderHeadOverlay = FL::config.renderHeadOverlay;
    WAPIUtil::F11Hook::fakeFullscreen = FL::config.fakeFullscreen;

    LogParser::logFilePath = FL::config.minecraftLogPath;
    spdlog::info("Set Minecraft log file path to: {}", FL::config.minecraftLogPath);

    FL::write();

    titleHeight = screenWidth * titleRatio, closeButtonWidth = screenWidth * closeButtonRatio,
    closeButtonPadding = screenWidth * closeButtonPaddingRatio;

    spdlog::info("Creating window... screenWidth={}, screenHeight={}", screenWidth, screenHeight);
    window.reset(SDL_CreateWindow("Stats Overlay (Ctrl+Shift+O)",
                                  4,                                                                                                     // x
                                  4,                                                                                                     // y
                                  screenWidth,                                                                                           // width
                                  screenHeight,                                                                                          // height
                                  SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_ALLOW_HIGHDPI  // params
                                 ));

    if (!window) {
        spdlog::critical("Error creating window. Error: {}", SDL_GetError());
        return 1;
    }

    spdlog::info("Setting window opacity to {}", opacity);

    if (SDL_SetWindowOpacity(window.get(), opacity) < 0) {
        spdlog::error("Error setting window opacity. Error: {}", SDL_GetError());
    }

    SDL_SetWindowResizable(window.get(), SDL_TRUE);

    spdlog::info("Creating renderer");
    renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

    if (!renderer) {
        spdlog::critical("Error creating renderer. Error: {}", SDL_GetError());
        return 1;
    }

    if (SDL_SetWindowHitTest(window.get(), dragHitTest, NULL) == -1) {
        spdlog::error("Could not set window hit test (dragging the window will not work). Error: {}", SDL_GetError());
    }

    {
        // Get window handle
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window.get(), &wmInfo);
        HWND windowHandle = wmInfo.info.win.window;
        long windowExStyles = GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
        // Hide window from alt-tab switcher
        SetWindowLongPtr(windowHandle, GWL_EXSTYLE, windowExStyles | WS_EX_TOOLWINDOW);
    }

    spdlog::info("Starting keyboard shortcut loop");
    std::thread(WAPIUtil::Hotkey::keyboardShortcutLoop).detach();

    spdlog::info("Starting log parser loop");
    std::thread(LogParser::updateLoop).detach();

    titleRectangle = {0, 0, screenWidth, titleHeight};

    spdlog::info("Loading title font from {}", FL::config.titleFontPath);
    titleFont.reset(TTF_OpenFont(FL::config.titleFontPath.c_str(), (int)(titleHeight * titleFontRatio)));

    if (!titleFont) {
        spdlog::critical("Could not load title font (path={}). Error: {}", FL::config.titleFontPath, TTF_GetError());
        return 1;
    }

    // Create the centered title text
    SDL2::Texture titleTextTexture;
    {
        SDL2::Surface titleTextSurface(TTF_RenderText_Blended(titleFont.get(), "Stats Overlay (Ctrl+Shift+O)", {255, 255, 255, 255}));
        titleTextTexture.reset(SDL_CreateTextureFromSurface(renderer.get(), titleTextSurface.get()));
        int titleTextWidth, titleTextHeight;
        SDL_QueryTexture(titleTextTexture.get(), NULL, NULL, &titleTextWidth, &titleTextHeight);
        titleTextRectangle = {(screenWidth - titleTextWidth) / 2, (titleHeight - titleTextHeight) / 2, titleTextWidth, titleTextHeight};
    }

    closeButtonRectangle = {screenWidth - 3 * closeButtonWidth, 0, 3 * closeButtonWidth, titleHeight};

    /*
    SDL2::Texture configImageTexture;
    {
        SDL2::Surface configImageSurface(IMG_Load("./assets/settings.png")), windowSurface(SDL_GetWindowSurface(window.get()));

        if (!configImageSurface) {
            spdlog::critical("Could not load image. Error: {}", IMG_GetError());
            return 1;
        }

        SDL2::Surface optimizedConfigImageSurface(SDL_ConvertSurface(configImageSurface.get(), windowSurface->format, 0));
        SDL_SetColorKey(optimizedConfigImageSurface.get(), SDL_TRUE, SDL_MapRGB(windowSurface->format, 0, 0, 0));
        configImageTexture.reset(SDL_CreateTextureFromSurface(renderer.get(), optimizedConfigImageSurface.get()));
        configImageRectangle = {(int)(screenWidth * borderRatio), (int)(screenWidth * borderRatio),
                                (int)((titleHeight - screenWidth *borderRatio * 2) * configImageRatio + 0.5),
                                titleHeight - (int)(screenWidth *borderRatio * 2)};
        spdlog::info("Loaded settings image");
    }
    */

    spdlog::info("Loading stats font from {}", FL::config.statsFontPath);
    statsFont.reset(TTF_OpenFont(FL::config.statsFontPath.c_str(), (int)(screenWidth * statsFontRatio)));

    if (!statsFont) {
        spdlog::critical("Could not load stats font (path={}). Error: {}", FL::config.statsFontPath, TTF_GetError());
        return 1;
    }

    // Load the three special symbols (U-272B, U-272A, U-2740)
    symbolsFont.reset(TTF_OpenFont("./assets/272B-272A-2740.ttf", (int)(screenWidth * statsFontRatio * 0.975)));

    if (!dummyTextInfo.init) {
        dummyTextInfo.init = true;
        createTextTexture(dummyTextInfo.username, "Username", statsFont);
        createTextTexture(dummyTextInfo.level, "Level", statsFont);
        createTextTexture(dummyTextInfo.stars.single, "Stars", statsFont);
        createTextTexture(dummyTextInfo.stars.symbol, "", symbolsFont);
        createTextTexture(dummyTextInfo.FK, "FK", statsFont);
        createTextTexture(dummyTextInfo.FD, "FD", statsFont);
        createTextTexture(dummyTextInfo.FKDR, "FKDR", statsFont);
        createTextTexture(dummyTextInfo.W, "W", statsFont);
        createTextTexture(dummyTextInfo.L, "L", statsFont);
        createTextTexture(dummyTextInfo.WLR, "WLR", statsFont);
    }

    spdlog::info("Raising overlay GUI");
    SDL_RaiseWindow(window.get());

    bool visible = true, closeButtonHighlight = false;
    int buttons, windowX, windowY, mouseX, mouseY;
    std::chrono::time_point<std::chrono::steady_clock> loopStartTime, lastVisibiltyChangeTime = std::chrono::steady_clock::now(),
                                                                      lastRenderUpdateTime = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> timeDifference;

    renderUpdate = true;
    int currentScene = STATS_SCENE;

    while (running.load()) {
        loopStartTime = std::chrono::steady_clock::now();

        // Force render update every 1 second
        if (std::chrono::duration_cast<std::chrono::milliseconds>(loopStartTime - lastRenderUpdateTime).count() > 1000) {
            renderUpdate = true;
        }

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Get mouse motion information
            {
                SDL_GetGlobalMouseState(&mouseX, &mouseY);
                SDL_GetWindowPosition(window.get(), &windowX, &windowY);

                if (mouseX < windowX || mouseY < windowY || mouseX > screenWidth + windowX || mouseY > screenHeight + windowY) {
                    // mouse position out of bounds
                    if (closeButtonHighlight) {
                        closeButtonHighlight = false;
                        renderUpdate = true;
                    }

                    continue;
                }

                buttons = SDL_GetMouseState(&mouseX, &mouseY);
                SDL_Point currentMousePos = {mouseX, mouseY};

                if (SDL_PointInRect(&currentMousePos, &closeButtonRectangle)) {
                    if (!closeButtonHighlight) {
                        closeButtonHighlight = true;
                        renderUpdate = true;
                    }

                    if ((buttons & SDL_BUTTON_LMASK) != 0) {
                        spdlog::info("Close button pressed");
                        running = false;
                        break;
                    }

                    /*
                    } else if (SDL_PointInRect(&currentMousePos, &configImageRectangle)) {
                        if (currentScene != CONFIG_SCENE && (buttons & SDL_BUTTON_LMASK) != 0) {
                            spdlog::info("Config button pressed");
                            currentScene = CONFIG_SCENE;
                            renderUpdate = true;
                        }
                    */

                } else {
                    if (closeButtonHighlight) {
                        closeButtonHighlight = false;
                        renderUpdate = true;
                    }
                }
            }
        }

        if (!running.load()) {
            break;
        }

        if (WAPIUtil::Hotkey::shortcutKeysPressed()) {
            timeDifference = loopStartTime - lastVisibiltyChangeTime;

            if (timeDifference.count() > 200) {
                if (visible) {
                    spdlog::info("Hiding window (shortcut keys pressed)");
                    SDL_HideWindow(window.get());
                    visible = false;

                } else {
                    spdlog::info("Showing window (shortcut keys pressed)");
                    SDL_ShowWindow(window.get());
                    SDL_RaiseWindow(window.get());
                    visible = true;
                    renderUpdate = true;
                }

                lastVisibiltyChangeTime = loopStartTime;
            }
        }

        // No point of rendering if the window isn't visible or if there weren't any changes made
        if (renderUpdate.load() && visible) {
            renderUpdate = false;
            lastRenderUpdateTime = loopStartTime;

            // Draw background
            SDL_RenderClear(renderer.get());
            SDL_SetRenderDrawColor(renderer.get(), backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);

            // Draw title (default RGB value is 10, 10, 10)
            SDL_RenderClear(renderer.get());
            SDL_SetRenderDrawColor(renderer.get(), titleColor.r, titleColor.g, titleColor.b, 255);
            SDL_RenderFillRect(renderer.get(), &titleRectangle);

            // Draw title text
            SDL_RenderCopy(renderer.get(), titleTextTexture.get(), NULL, &titleTextRectangle);

            // Draw close button (X) in the top right corner
            if (closeButtonHighlight) {
                SDL_SetRenderDrawColor(renderer.get(), 255, 0, 0, 255);
                SDL_RenderFillRect(renderer.get(), &closeButtonRectangle);
            }

            SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer.get(), screenWidth - 2 * closeButtonWidth, closeButtonPadding, screenWidth - closeButtonWidth,
                               closeButtonPadding + closeButtonWidth);
            SDL_RenderDrawLine(renderer.get(), screenWidth - 2 * closeButtonWidth, closeButtonPadding + closeButtonWidth,
                               screenWidth - closeButtonWidth, closeButtonPadding);

            if (true || currentScene == STATS_SCENE) {
                // Draw config button
                // SDL_RenderCopy(renderer.get(), configImageTexture.get(), NULL, &configImageRectangle);

                int currentHeight = titleHeight + screenWidth * statsFontRatio * 0.5;

                SDL_SetWindowSize(window.get(), screenWidth, screenHeight + (screenWidth * statsFontRatio * 1.5) * LogParser::players.size() + (currentHeight + screenWidth * statsFontRatio * 0.5));

                renderAllTextures(dummyTextInfo, currentHeight, false);
                currentHeight += screenWidth * statsFontRatio * 1.5;

                for (MPI::Player &player : LogParser::players) {
                    if (!player.updated || !player.render) {
                        continue;
                    }

                    std::string errorMessage;

                    if (player.errorMessage.size() > 0) {
                        errorMessage = player.errorMessage;

                    } else if (player.bedwars.errorMessage.size() > 0) {
                        errorMessage = player.bedwars.errorMessage;
                    }

                    if (errorMessage.size() == 0) {
                        BWI::info stats;

                        if (FL::config.displayMode == "solos") {
                            stats = player.bedwars.solos;

                        } else if (FL::config.displayMode == "doubles") {
                            stats = player.bedwars.doubles;

                        } else if (FL::config.displayMode == "threes") {
                            stats = player.bedwars.threes;

                        } else if (FL::config.displayMode == "fours") {
                            stats = player.bedwars.fours;

                        } else {
                            stats = player.bedwars.overall;
                        }

                        if (!player.textures.init) {
                            player.textures.init = true;
                            createHeadTexture(player.textures.skin, player.skin);
                            createTextTexture(player.textures.username, player.username, statsFont);
                            createTextTexture(player.textures.level, std::to_string(player.networkLevel), statsFont);

                            if (!player.bedwars.hasMultiStarColor) {
                                createTextTexture(player.textures.stars.single, std::to_string(player.bedwars.stars), statsFont, player.bedwars.starColor);

                            } else {
                                player.textures.stars.singleColor = false;
                                createMultiColorTextTexture(player.textures.stars.multi, std::to_string(player.bedwars.stars), statsFont, player.bedwars.starColors);
                            }

                            createTextTexture(player.textures.stars.symbol, player.bedwars.starSymbol, symbolsFont, player.bedwars.starSymbolColor);
                            createTextTexture(player.textures.FK, std::to_string(stats.FK), statsFont);
                            createTextTexture(player.textures.FD, std::to_string(stats.FD), statsFont);
                            createTextTexture(player.textures.FKDR, to2DPString(stats.FKDR), statsFont);
                            createTextTexture(player.textures.W, std::to_string(stats.W), statsFont);
                            createTextTexture(player.textures.L, std::to_string(stats.L), statsFont);
                            createTextTexture(player.textures.WLR, to2DPString(stats.WLR), statsFont);
                        }

                        renderAllTextures(player.textures, currentHeight, false);

                    } else {
                        if (!player.textures.init) {
                            player.textures.init = true;

                            if (player.skin.size() != 0) {
                                createHeadTexture(player.textures.skin, player.skin);
                            }

                            createTextTexture(player.textures.username, player.username, statsFont);
                            createTextTexture(player.textures.errorMessage, errorMessage, statsFont);
                        }

                        renderAllTextures(player.textures, currentHeight, true);
                    }

                    currentHeight += screenWidth * statsFontRatio * 1.5;
                }

                screenHeight = currentHeight + screenWidth * statsFontRatio * 0.5;

                SDL_SetWindowSize(window.get(), screenWidth, screenHeight);

            } else if (currentScene == CONFIG_SCENE) {}

            // Render current
            SDL_RenderPresent(renderer.get());
        }

        // Temporarily sleep this thread to limit the window FPS
        std::this_thread::sleep_until(loopStartTime + std::chrono::milliseconds(refreshTime));
    }

    spdlog::info("Exiting");

    // Free leftover resources
    SDL_Quit();
    TTF_Quit();

    return 0;
}

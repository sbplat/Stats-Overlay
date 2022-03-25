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

#include "Types.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <string>
#include <cmath>


#ifndef BEDWARS_H
#define BEDWARS_H

namespace BWI {

    namespace JSON = nlohmann;

    namespace XP {
        const int BW_EXP_NEEDED[4] = {500, 1000, 2000, 3500};
    } // XP

    struct info {
        int FK = 0, FD = 0, W = 0, L = 0;
        float FKDR = 0, WLR = 0;

        void round2DP() {
            FKDR = std::floor(FKDR * 100) / 100.0;
            WLR = std::floor(WLR * 100) / 100.0;
        }
    };

    struct BedWarsInfo {
        std::string username, errorMessage;
        JSON::json stats;

        int stars = 0;
        std::string starSymbol;

        bool hasMultiStarColor = false;
        SDL_Color starColor, starSymbolColor;
        std::vector<SDL_Color> starColors;

        BWI::info solos, doubles, threes, fours, overall;

        void init() {
            if (stats.empty()) {
                updateStarAndSymbolColors();
                return;
            }

            calculateStars();
            updateStarAndSymbolColors();
            updateStats(solos, "eight_one");
            updateStats(doubles, "eight_two");
            updateStats(threes, "four_three");
            updateStats(fours, "four_four");
            updateOverall();

            spdlog::debug("Initialized BedWars data for player={}, stars={}", username, stars);
        }

        int calculateStars() {
            spdlog::debug("Calculating BedWars stars for player={}", username);

            auto getExpForLevel = [&](int level) {
                int progress = level % 100;

                if (progress > 3) {
                    return 5000;

                } else {
                    return XP::BW_EXP_NEEDED[progress];
                }
            };

            int experience = 0;

            try {
                experience = stats.at("Experience");

            } catch (const JSON::json::out_of_range &e) {
            }

            int prestiges = experience / 487000,
                level = prestiges * 100,
                remainingExp = experience - (prestiges * 487000);

            for (int i = 0; i < 4; ++i) {
                int expForNextLevel = getExpForLevel(i);

                if (remainingExp < expForNextLevel) {
                    break;
                }

                ++level;
                remainingExp -= expForNextLevel;
            }

            stars = std::floor(level + (remainingExp / 5000.0) + 0.5);

            return stars;
        }

        void updateStats(BWI::info &mode, std::string id) {
            try {
                mode.FK = stats.at(id + "_final_kills_bedwars");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.FD = stats.at(id + "_final_deaths_bedwars");

            } catch (const JSON::json::out_of_range &e) {
            }

            mode.FKDR = mode.FK / (float)(mode.FD == 0 ? 1 : mode.FD);

            try {
                mode.W = stats.at(id + "_wins_bedwars");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.L = stats.at(id + "_losses_bedwars");

            } catch (const JSON::json::out_of_range &e) {
            }

            mode.WLR = mode.W / (float)(mode.L == 0 ? 1 : mode.L);

            mode.round2DP();
        }

        void updateOverall() {
            overall.FK = solos.FK + doubles.FK + threes.FK + fours.FK;
            overall.FD = solos.FD + doubles.FD + threes.FD + fours.FD;
            overall.FKDR = overall.FK / (float)(overall.FD == 0 ? 1 : overall.FD);

            overall.W = solos.W + doubles.W + threes.W + fours.W;
            overall.L = solos.L + doubles.L + threes.L + fours.L;
            overall.WLR = overall.W / (float)(overall.L == 0 ? 1 : overall.L);

            overall.round2DP();
        }

        void updateStarAndSymbolColors() {
            int starHexColor = -1, symbolHexColor;
            int starHexColors[4];

            if (stars < 100) {
                starHexColor = 0xAAAAAA;
                symbolHexColor = 0xAAAAAA;

            } else if (stars < 200) {
                starHexColor = 0xFFFFFF;
                symbolHexColor = 0xFFFFFF;

            } else if (stars < 300) {
                starHexColor = 0xFFAA00;
                symbolHexColor = 0xFFAA00;

            } else if (stars < 400) {
                starHexColor = 0x55FFFF;
                symbolHexColor = 0x55FFFF;

            } else if (stars < 500) {
                starHexColor = 0x00AA00;
                symbolHexColor = 0x00AA00;

            } else if (stars < 600) {
                starHexColor = 0x00AAAA;
                symbolHexColor = 0x00AAAA;

            } else if (stars < 700) {
                starHexColor = 0xAA0000;
                symbolHexColor = 0xAA0000;

            } else if (stars < 800) {
                starHexColor = 0xFF55FF;
                symbolHexColor = 0xFF55FF;

            } else if (stars < 900) {
                starHexColor = 0x5555FF;
                symbolHexColor = 0x5555FF;

            } else if (stars < 1000) {
                starHexColor = 0xAA00AA;
                symbolHexColor = 0xAA00AA;

            } else if (stars < 1100) {
                // rainbow colour here
                starHexColors[0] = 0xFFAA00;
                starHexColors[1] = 0xFFFF55;
                starHexColors[2] = 0x55FF55;
                starHexColors[3] = 0x55FFFF;
                symbolHexColor = 0xFF55FF;

            } else if (stars < 1200) {
                starHexColor = 0xFFFFFF;
                symbolHexColor = 0xAAAAAA;

            } else if (stars < 1300) {
                starHexColor = 0xFFFF55;
                symbolHexColor = 0xFFAA00;

            } else if (stars < 1400) {
                starHexColor = 0x55FFFF;
                symbolHexColor = 0x00AAAA;

            } else if (stars < 1500) {
                starHexColor = 0x55FF55;
                symbolHexColor = 0x00AA00;

            } else if (stars < 1600) {
                starHexColor = 0xAAAAAA;
                symbolHexColor = 0x5555FF;

            } else if (stars < 1700) {
                starHexColor = 0xFF5555;
                symbolHexColor = 0xAA0000;

            } else if (stars < 1800) {
                starHexColor = 0xFF55FF;
                symbolHexColor = 0xAA00AA;

            } else if (stars < 1900) {
                starHexColor = 0x5555FF;
                symbolHexColor = 0x0000AA;

            } else if (stars < 2000) {
                starHexColor = 0xAA00AA;
                symbolHexColor = 0xFF55FF;

            } else if (stars < 2100) {
                // multicolour starts here again
                // mirror
                starHexColors[0] = 0xAAAAAA;
                starHexColors[1] = 0xFFFFFF;
                starHexColors[2] = 0xFFFFFF;
                starHexColors[3] = 0xAAAAAA;
                symbolHexColor = 0xAAAAAA;

            } else if (stars < 2200) {
                // light
                starHexColors[0] = 0xFFFFFF;
                starHexColors[1] = 0xFFFF55;
                starHexColors[2] = 0xFFFF55;
                starHexColors[3] = 0xFFAA00;
                symbolHexColor = 0xFFAA00;

            } else if (stars < 2300) {
                // dawn
                starHexColors[0] = 0xFFAA00;
                starHexColors[1] = 0xFFFFFF;
                starHexColors[2] = 0xFFFFFF;
                starHexColors[3] = 0x55FFFF;
                symbolHexColor = 0x00AAAA;

            } else if (stars < 2400) {
                // dusk
                starHexColors[0] = 0xAA00AA;
                starHexColors[1] = 0xFF55FF;
                starHexColors[2] = 0xFF55FF;
                starHexColors[3] = 0xFFAA00;
                symbolHexColor = 0xFFFF55;

            } else if (stars < 2500) {
                // air
                starHexColors[0] = 0x55FFFF;
                starHexColors[1] = 0xFFFFFF;
                starHexColors[2] = 0xFFFFFF;
                starHexColors[3] = 0xAAAAAA;
                symbolHexColor = 0xAAAAAA;

            } else if (stars < 2600) {
                // wind
                starHexColors[0] = 0xFFFFFF;
                starHexColors[1] = 0x55FF55;
                starHexColors[2] = 0x55FF55;
                starHexColors[3] = 0x00AA00;
                symbolHexColor = 0x00AA00;

            } else if (stars < 2700) {
                // nebula
                starHexColors[0] = 0xAA0000;
                starHexColors[1] = 0xFF5555;
                starHexColors[2] = 0xFF5555;
                starHexColors[3] = 0xFF55FF;
                symbolHexColor = 0xFF55FF;

            } else if (stars < 2800) {
                // thunder
                starHexColors[0] = 0xFFFF55;
                starHexColors[1] = 0xFFFFFF;
                starHexColors[2] = 0xFFFFFF;
                starHexColors[3] = 0x555555;
                symbolHexColor = 0x555555;

            } else if (stars < 2900) {
                // earth
                starHexColors[0] = 0x55FF55;
                starHexColors[1] = 0x00AA00;
                starHexColors[2] = 0x00AA00;
                starHexColors[3] = 0xFFAA00;
                symbolHexColor = 0xFFAA00;

            } else if (stars < 3000) {
                // water
                starHexColors[0] = 0x55FFFF;
                starHexColors[1] = 0x00AAAA;
                starHexColors[2] = 0x00AAAA;
                starHexColors[3] = 0x5555FF;
                symbolHexColor = 0x5555FF;

            } else {
                // >= 3000 stars
                // fire
                starHexColors[0] = 0xFFFF55;
                starHexColors[1] = 0xFFAA00;
                starHexColors[2] = 0xFFAA00;
                starHexColors[3] = 0xFF5555;
                symbolHexColor = 0xFF5555;
            }

            // symbol

            if (stars < 1100) {
                starSymbol = "✫";

            } else if (stars < 2100) {
                starSymbol = "✪";

            } else {
                starSymbol = "❀";
            }

            if (starHexColor != -1) {
                starColor = lightenRGB(hexToRGB(starHexColor), 0);

            } else {
                hasMultiStarColor = true;

                for (int i = 0; i < 4; ++i) {
                    starColors.push_back(lightenRGB(hexToRGB(starHexColors[i]), 0));
                }
            }

            starSymbolColor = lightenRGB(hexToRGB(symbolHexColor), 0);
        }
    };

}  // namespace BWI

#endif  // BEDWARS_H

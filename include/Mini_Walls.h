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


#ifndef MINI_WALLS_H
#define MINI_WALLS_H

namespace MWI {

    namespace JSON = nlohmann;

    struct info {
        std::string activeKit;
        int K = 0, D = 0, FK = 0, W = 0, witherKills = 0, witherDamage = 0,
            arrowsShot = 0, arrowsHit = 0;
        float KDR = 0, AHP = 0;

        void round2DP() {
            KDR = std::floor(KDR * 100) / 100.0;
            AHP = std::floor(AHP * 100) / 100.0; // Arrow hit percentage
        }
    };

    struct MiniWallsInfo {
        std::string username, errorMessage;
        JSON::json stats;

        MWI::info overall;

        void init() {
            if (stats.empty()) {
                return;
            }

            updateOverall();

            spdlog::debug("Initialized Mini Walls data for player={}", username);
        }

        void updateStats(MWI::info &mode, std::string id) {
            try {
                mode.activeKit = stats.at(id + "miniwalls_activeKit");

            } catch (const JSON::json::out_of_range &e) {
            }

            if (mode.activeKit.empty()) {
                mode.activeKit = "X";

            } else {
                mode.activeKit = std::toupper(mode.activeKit.front());
            }

            try {
                mode.K = stats.at(id + "kills_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.D = stats.at(id + "deaths_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            mode.KDR = mode.K / (float)(mode.D == 0 ? 1 : mode.D);

            try {
                mode.FK = stats.at(id + "final_kills_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.W = stats.at(id + "wins_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.witherKills = stats.at(id + "wither_kills_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.witherDamage = stats.at(id + "wither_damage_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.arrowsShot = stats.at(id + "arrows_shot_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            try {
                mode.arrowsHit = stats.at(id + "arrows_hit_mini_walls");

            } catch (const JSON::json::out_of_range &e) {
            }

            mode.AHP = mode.arrowsHit / (float)(mode.arrowsShot == 0 ? 1 : mode.arrowsShot);

            mode.round2DP();
        }

        void updateOverall() {
            updateStats(overall, "");
        }
    };

}  // namespace MWI

#endif  // MINI_WALLS_H

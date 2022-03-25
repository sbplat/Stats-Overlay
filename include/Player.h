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

#include "Bedwars.h"
#include "Types.h"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <ctime>
#include <future>
#include <string>


#ifndef PLAYER_H
#define PLAYER_H

// From: https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/34571089#34571089
std::string base64_decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256, -1);

    for (int i = 0; i < 64; i++) {
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    }

    int val = 0, valb = -8;

    for (char c : in) {
        if (T[c] == -1) {
            break;
        }

        val = (val << 6) + T[c];
        valb += 6;

        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return out;
}

namespace MPI {

    namespace JSON = nlohmann;

    std::string HYPIXEL_API_KEY, MOJANG_API_URL = "https://api.mojang.com/users/profiles/minecraft/",
                                 MOJANG_SESSION_SERVER_URL = "https://sessionserver.mojang.com/session/minecraft/profile/";

    bool apiKeyValid = false;

    cpr::Url HYPIXEL_API_TEST_URL{"https://api.hypixel.net/key"}, HYPIXEL_API_PLAYER_URL{"https://api.hypixel.net/player"};

    bool testApiKey(std::string key) {
        spdlog::debug("Testing Hypixel API key...");
        cpr::Response response = cpr::Get(HYPIXEL_API_TEST_URL, cpr::Parameters{{"key", key}});

        JSON::json data = JSON::json::parse(response.text);

        apiKeyValid = data["success"];
        spdlog::debug("API key status: {}", apiKeyValid);

        if (apiKeyValid) {
            HYPIXEL_API_KEY = key;

        } else {
            spdlog::warn("Invalid Hypixel API key (key={})", key);
        }

        return apiKeyValid;
    }

    bool testApiKey() {
        return testApiKey(HYPIXEL_API_KEY);
    }

    namespace XP {
        const float BASE = 10000, GROWTH = 2500;

        const float REVERSE_PQ_PREFIX = -(BASE - 0.5 * GROWTH) / GROWTH, REVERSE_CONST = REVERSE_PQ_PREFIX * REVERSE_PQ_PREFIX,
                    GROWTH_DIVIDES_TWO = 2.0 / GROWTH;
    }  // namespace XP

    struct StarTextures {
        bool singleColor = true;
        SDL2::Texture single, symbol;
        std::vector<SDL2::Texture> multi;
    };

    struct PlayerInfoTextures {
        bool init = false;
        SDL2::Texture skin, username, level,
             FK, FD, FKDR, W, L, WLR,
             errorMessage;
        StarTextures stars;
    };

    struct Player {
        long long timestamp;
        std::future<cpr::Response> asyncResponse;
        JSON::json data;

        bool canUpdateUUID = false, canUpdateProfile = false, canUpdateData = false, canUpdateSkin = false, updated = false, render = true;

        std::string username, mojangUsername, uuid, skinURL, skin, errorMessage;
        int networkLevel = 1;

        BWI::BedWarsInfo bedwars;

        PlayerInfoTextures textures;

        Player(std::string name) {
            timestamp = (long long)time(NULL);
            username = name;
        }

        // fetch functions are async
        // update (get data) functions are blocking

        int fetchUUID() {
            spdlog::debug("Fetching UUID for player={}", username);

            if (username.size() == 0) {
                spdlog::warn("Invalid username (size=0)");
                errorMessage = "Invalid username (size=0)";

                canUpdateUUID = false;

                return 0;

            } else {
                canUpdateUUID = true;
                asyncResponse = cpr::GetAsync(cpr::Url{MOJANG_API_URL + username});

                return 1;
            }
        }

        int updateUUID() {
            spdlog::debug("Updating UUID for {}", username);

            if (!canUpdateUUID) {
                return 0;

            } else {
                cpr::Response response = asyncResponse.get();

                if (response.status_code == 200) {
                    data = JSON::json::parse(response.text);

                    uuid = data["id"];
                    mojangUsername = data["name"];

                    spdlog::debug("Got UUID for player={} (UUID={})", username, uuid);

                    return 1;

                } else if (response.status_code == 204) {
                    spdlog::debug("Could not update UUID for {}. (player is nicked)", username);
                    errorMessage = "Invalid Username (player is nicked)";

                    return 2;

                } else if (response.status_code == 429) {
                    spdlog::debug("Could not update UUID for {}. (Mojang API ratelimited)", username);
                    errorMessage = "Mojang API ratelimited";

                    return 3;

                } else {
                    spdlog::error("Could not update UUID for {}. Mojang API status code: {}", username, response.status_code);
                    errorMessage = "Mojang API: status_code=" + std::to_string(response.status_code);

                    return 4;
                }
            }
        }

        int fetchProfile() {
            spdlog::debug("Fetching Minecraft profile for player={}", username);

            if (errorMessage.size() != 0) {
                spdlog::debug("Stopped fetching Minecraft profile due to previous error(s)");
                canUpdateProfile = false;

                return 0;

            } else if (uuid.size() == 0) {
                spdlog::critical("THIS SHOULD NOT HAPPEN!! Invalid uuid (size=0), player={}", username);
                errorMessage = "??? Invalid uuid (size=0)";

                canUpdateProfile = false;

                return 1;

            } else {
                canUpdateProfile = true;
                asyncResponse = cpr::GetAsync(cpr::Url{MOJANG_SESSION_SERVER_URL + uuid});

                return 2;
            }
        }

        int updateProfile() {
            spdlog::debug("Updating Minecraft profile for player={}", username);

            if (!canUpdateProfile) {
                return 0;

            } else {
                cpr::Response response = asyncResponse.get();

                if (response.status_code == 200) {
                    data = JSON::json::parse(base64_decode(JSON::json::parse(response.text).at("properties").at(0).at("value")));

                    try {
                        skinURL = data.at("textures").at("SKIN").at("url");
                        spdlog::debug("Got skin URL for player={} (url={})", username, skinURL);

                    } catch (...) {
                        spdlog::error("Player does not have a skin! (username={}, uuid={})", username, uuid);
                    }

                    return 1;

                } else if (response.status_code == 204) {
                    // ignore nicks
                    // shouldn't have a uuid if they're nicked, right?
                    return 2;

                } else if (response.status_code == 429) {
                    // sessionserver has a ratelimit?
                    // wiki says it doesn't have one
                    // this check is here just in case
                    spdlog::debug("Could not update skin URL for {}. (Mojang sessionserver ratelimited)", username);
                    errorMessage = "Mojang sessionserver ratelimited";

                    return 3;

                } else {
                    spdlog::error("Could not update skin URL for player={}. Mojang sessionserver status code: {}", username, response.status_code);
                    errorMessage = "Mojang sessionserver: status_code=" + std::to_string(response.status_code);

                    return 4;
                }
            }
        }

        int fetchSkin() {
            spdlog::debug("Fetching Minecraft skin for player={}", username);

            if (errorMessage.size() != 0) {
                spdlog::debug("Stopped fetching Minecraft skin due to previous error(s)");
                canUpdateSkin = false;

                return 0;

            } else {
                canUpdateSkin = true;
                asyncResponse = cpr::GetAsync(cpr::Url{skinURL});

                return 1;
            }
        }

        int updateSkin() {
            spdlog::debug("Updating Minecraft skin for player={}", username);

            if (!canUpdateSkin) {
                return 0;

            } else {
                cpr::Response response = asyncResponse.get();

                if (response.status_code == 200) {
                    skin = response.text;

                    spdlog::debug("Got Minecraft skin for player={}", username, uuid);

                    return 1;

                } else {
                    spdlog::error("Could not update skin for player={}. Mojang textures status code: {}", username, response.status_code);
                    errorMessage = "Mojang textures: status_code=" + std::to_string(response.status_code);

                    return 2;
                }
            }
        }

        int fetchData() {
            spdlog::debug("Fetching Hypixel data for player={}", username);

            if (!apiKeyValid || HYPIXEL_API_KEY.size() == 0) {
                spdlog::error("Hypixel API key is invalid");
                errorMessage = "Invalid Hypixel API key";
                canUpdateData = false;

                return 0;

            } else if (username.size() == 0 || uuid.size() == 0 || errorMessage.size() > 0) {
                spdlog::debug("Stopped fetching Hypixel data for player={} due to invalid username/uuid and/or previous error(s)", username);

                if (errorMessage.size() == 0) {
                    // set an error message if there currently isn't one
                    errorMessage = "Invalid username/uuid";
                }

                canUpdateData = false;

                return 0;

            } else {
                canUpdateData = true;
                asyncResponse = cpr::GetAsync(HYPIXEL_API_PLAYER_URL, cpr::Parameters{{"key", HYPIXEL_API_KEY}, {"uuid", uuid}});

                return 1;
            }
        }

        int updateData() {
            spdlog::debug("Updating Hypixel data for player={}", username);

            if (!canUpdateData) {
                return 0;

            } else {
                cpr::Response response = asyncResponse.get();

                if (response.status_code == 200) {
                    data = JSON::json::parse(response.text);

                    // make sure the player has data (have Hypixel stats)
                    if (!verifyPlayerData()) {
                        return 1;
                    }

                    // make sure the player isn't nicked as someone else (using an existing nickname)
                    if (!verifyUsername()) {
                        return 2;
                    }

                    calculateLevel();
                    initBedwarsInfo();

                    spdlog::debug("Done updating Hypixel data for player={}", username);

                    return 1;

                } else if (response.status_code == 403) {
                    spdlog::error("Forbidden response when fetching Hypixel data for player={}", username);
                    errorMessage = "Forbidden (invalid API key)";
                    apiKeyValid = false;

                    return 3;

                } else if (response.status_code == 429) {
                    spdlog::warn("Ratelimit reached when fetching Hypixel data for player={}", username);
                    errorMessage = "Ratelimit reached (please slow down)";

                    return 4;

                } else {
                    spdlog::error("Could not fetch Hypixel data for player={}. Hypixel API status code: {}", username, response.status_code);
                    errorMessage = "Hypixel API: status_code=" + std::to_string(response.status_code);

                    return 5;
                }
            }
        }

        bool verifyPlayerData() {
            spdlog::debug("Verifying data for player={}", username);
            JSON::json playerData = data["player"];

            if (!playerData.empty()) {
                return true;

            } else {
                spdlog::debug("Player={} has no data available (no Hypixel stats)", username);
                errorMessage = "No Hypixel stats available";
                return false;
            }
        }

        bool verifyUsername() {
            spdlog::debug("Verifying username for player={}", username);

            std::string hypixelDisplayName;

            try {
                hypixelDisplayName = data.at("player").at("displayname");

            } catch (const JSON::json::out_of_range &e) {
                spdlog::error("Player={} does not have a display name?\nException: {}\nData:\n{}", username, e.what(), data);
                return false;
            }

            if (username == mojangUsername && username == hypixelDisplayName) {
                return true;

            } else {
                spdlog::debug("Unable to verify username ({} != {} OR {} != {}). The player has nickname permissions and is nicked (is YT, Admin, etc.)?", username, mojangUsername, username, hypixelDisplayName);
                errorMessage = "Has nickname permissions and is nicked (is YT, Admin, etc.)";
                return false;
            }
        }

        int calculateLevel() {
            spdlog::debug("Calculating network level for player={}", username);

            try {
                int networkExp = data.at("player").at("networkExp");

                if (networkExp < 0) {
                    networkLevel = 1;

                } else {
                    networkLevel = std::floor(1 + XP::REVERSE_PQ_PREFIX + std::sqrt(XP::REVERSE_CONST + XP::GROWTH_DIVIDES_TWO * networkExp) + 0.5);
                }

                return networkLevel;

            } catch (const JSON::json::out_of_range &e) {
                return 1;
            }
        }

        void initBedwarsInfo() {
            spdlog::debug("Initializing BedWars info for player={}", username);

            bedwars.username = username;

            try {
                bedwars.stats = data.at("player").at("stats").at("Bedwars");

            } catch (const JSON::json::out_of_range &e) {
                spdlog::debug("No BedWars data available for player={}", username);
            }

            bedwars.init();
        }
    };

}  // namespace MPI

#endif  // PLAYER_H

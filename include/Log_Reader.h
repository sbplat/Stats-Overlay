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

#include "File_Loader.h"
#include "Player.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <exception>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef LOGREADER_H
#define LOGREADER_H

namespace LogParser {

    std::string logFilePath;
    long long previousFileIndex = 0, previousFileSize = 0;

    std::vector<MPI::Player> players;

    const std::regex joinMiniServerRegex("^\\[\\d\\d:\\d\\d:\\d\\d\\] \\[Client thread/INFO\\]: \\[CHAT\\] (Sending you to mini(\\S+)|       )$"),
          hasJoinedRegex("^\\[\\d\\d:\\d\\d:\\d\\d\\] \\[Client thread/INFO\\]: \\[CHAT\\] (\\S+) has joined \\((\\d|\\d\\d)/(\\d|\\d\\d)\\)!$"),
          hasQuitRegex("^\\[\\d\\d:\\d\\d:\\d\\d\\] \\[Client thread/INFO\\]: \\[CHAT\\] (\\S+) has quit!$"),
          whoCommandRegex("^\\[\\d\\d:\\d\\d:\\d\\d\\] \\[Client thread/INFO\\]: \\[CHAT\\] ONLINE: (.+)$"),
          apiNewRegex("^\\[\\d\\d:\\d\\d:\\d\\d\\] \\[Client thread/INFO\\]: \\[CHAT\\] Your new API key is (.+)$"),
          extractCompactChat("(.+)( \\[x\\d\\])$");

    void filterPlayers() {
        players.erase(std::remove_if(players.begin(), players.end(),
        [](const MPI::Player & player) {
            if ((long long)time(NULL) - player.timestamp > FL::config.cachePlayerTime) {
                return true;
            }

            return false;
        }),
        players.end());
    }

    int find(std::string username) {
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (username == players[i].username) {
                return i;
            }
        }

        return -1;
    }

    void hideAllPlayers() {
        for (std::size_t i = 0; i < players.size(); ++i) {
            players[i].render = false;
        }
    }

    void updateAllPlayers() {
        // 1. fetch (async)
        // 2. update aka get data (blocking)

        // uuid
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (players[i].uuid.size() == 0 && players[i].errorMessage.size() == 0) {
                players[i].updateUUID();
                players[i].fetchProfile();
            }
        }

        // profile & fetch skin
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (players[i].skinURL.size() == 0 && players[i].errorMessage.size() == 0) {
                players[i].updateProfile();
                players[i].fetchSkin();
            }
        }

        // profile get skin
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (players[i].skin.size() == 0 && players[i].errorMessage.size() == 0) {
                players[i].updateSkin();
                players[i].fetchData();
            }
        }

        // Hypixel info
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (!players[i].updated && players[i].render && players[i].errorMessage.size() == 0) {
                players[i].updateData();
                renderUpdate = true;
            }

            players[i].updated = true;
        }
    }

    void addPlayer(std::string username) {
        int playerIndex = find(username);

        if (playerIndex == -1) {
            spdlog::debug("Adding player={} to queue", username);
            players.push_back(MPI::Player{username});

            players.back().fetchUUID();

        } else {
            if (players[playerIndex].errorMessage.length() == 0) {
                spdlog::debug("Found player={} in cache", username);

                players[playerIndex].render = true;

            } else {
                spdlog::debug("Reattempting to update player={} due to previous error ({})", username, players[playerIndex].errorMessage);

                players.erase(players.begin() + playerIndex);

                players.push_back(MPI::Player{username});
                players.back().fetchUUID();
            }
        }
    }

    void removePlayer(std::string username) {
        spdlog::debug("Removing player={}", username);
        int playerIndex = find(username);

        if (playerIndex == -1) {
            return;

        } else {
            players[playerIndex].render = false;
        }
    }

    void parseLine(std::string line) {
        // try to extract the line from compact chat ([x2])
        {
            std::smatch match;

            if (std::regex_search(line, match, extractCompactChat)) {
                line = match[1];
            }
        }

        if (std::regex_match(line, joinMiniServerRegex)) {
            hideAllPlayers();

        } else if (std::regex_match(line, hasJoinedRegex)) {
            std::smatch match;
            std::regex_search(line, match, hasJoinedRegex);

            addPlayer(match.str(1));

        } else if (std::regex_match(line, hasQuitRegex)) {
            std::smatch match;
            std::regex_search(line, match, hasQuitRegex);

            removePlayer(match.str(1));

        } else if (std::regex_match(line, whoCommandRegex)) {
            std::smatch match;
            std::regex_search(line, match, whoCommandRegex);

            spdlog::debug("Hypixel /who command detected: {}", match.str(1));

            hideAllPlayers();

            std::stringstream ss(match.str(1));
            std::string name;

            while (std::getline(ss, name, ',')) {
                name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
                addPlayer(name);
            }

        } else if (std::regex_match(line, apiNewRegex)) {
            std::smatch match;
            std::regex_search(line, match, apiNewRegex);

            spdlog::debug("Hypixel /api new command detected");

            if (MPI::testApiKey(match.str(1))) {
                FL::config.apiKey = MPI::HYPIXEL_API_KEY;
                FL::write();
            }
        }
    }

    long long getFileSize(std::string filePath) {
        struct stat statBuffer;
        int res = stat(filePath.c_str(), &statBuffer);
        return res == 0 ? statBuffer.st_size : -1;
    }

    void readFileUpdates(bool initLoop = false) {
        long long currentLineIndex = 0, currentFileSize = 0;
        std::string line;

        std::ifstream logFile(logFilePath);

        if (logFile) {
            currentFileSize = getFileSize(logFilePath);

            if (currentFileSize < previousFileSize) {
                // log file was modified or reset
                previousFileSize = 0;
            }

            while (std::getline(logFile, line, '\n')) {
                if (currentLineIndex >= previousFileIndex) {
                    // new line added
                    if (!initLoop) {
                        // call callback function
                        parseLine(line);
                    }
                }

                ++currentLineIndex;
            }

            previousFileIndex = currentLineIndex;
            previousFileSize = currentFileSize;
        }
    }

    void updateLoop() {
        readFileUpdates(true);

        while (running.load()) {
            try {
                readFileUpdates();
                filterPlayers();
                updateAllPlayers();

            } catch (std::exception &e) {
                // Log the exception and crash the program
                spdlog::dump_backtrace();
                spdlog::critical("Unhandled std::exception in loop: {}", e.what());
                throw e;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(FL::config.fileDelay));
        }
    }

}  // namespace LogParser

#endif  // LOGREADER_H

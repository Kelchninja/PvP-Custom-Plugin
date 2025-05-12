#pragma once

#include <fstream>
#include "json.hpp"
#include <Windows.h>
#include <sstream>
#include <regex>

namespace Helper
{

    inline std::string GetConfigPath()
    {
        return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/PVPCustom/config.json";
    }

    inline std::string GetPluginPath()
    {
        return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/PVPCustom/PVPCustom.dll";
    }

    inline void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
    {
        FString reply = msg + "\n";
        rcon_connection->SendMessageW(packet_id, 0, &reply);
    }

    inline nlohmann::json ReadConfig()
    {
        const std::string config_path = GetConfigPath();
        std::ifstream file{ config_path };
        if (!file.is_open())
            throw std::runtime_error("Can't open config.json");

        nlohmann::json config;
        file >> config;

        file.close();
        return config;
    }

    inline TArray<FString> ParseArguments(const FString& command)
    {
        TArray<FString> result;
        try
        {
            std::string commandStr = command.ToString();
            std::regex re(R"(([^=\s]+=".*?"|[^=\s]+=[^=\s]+|[^=\s]+))");
            std::sregex_iterator it(commandStr.begin(), commandStr.end(), re);
            std::sregex_iterator end;

            while (it != end) {
                std::smatch match = *it;
                std::string match_str = match.str();

                // Trim whitespace
                match_str.erase(0, match_str.find_first_not_of(" \t\n\r\f\v"));
                match_str.erase(match_str.find_last_not_of(" \t\n\r\f\v") + 1);

                if (!match_str.empty())
                {
                    result.Add(FString(match_str));
                }
                else
                {
                    break;
                }
                ++it;
            }
        }
        catch (const std::regex_error& e)
        {
            // Handle regex error
            Log::GetLog()->info("Regex error: {}", e.what());
        }
        catch (const std::exception& e)
        {
            // Handle other potential exceptions
            Log::GetLog()->info("Error: {}", e.what());
        }

        return result;
    }
}

#pragma once

#include "json.hpp"

#define PLUGIN_NAME "PVPCustom"
#define PLUGIN_VERSION "1.0.0"

namespace Data
{
	inline nlohmann::json config;
	inline std::string PVPCommand;
	inline std::string AcceptCommand;
	inline std::string DeclineCommand;
	inline std::string DuelCommand;
}
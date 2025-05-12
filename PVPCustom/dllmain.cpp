#pragma once
#include "API/ARK/Ark.h"
#include <fstream>

#include <iostream>
#include <string>

#include "Runtime/Runtime.hpp"
#include "hooks/hooks.h"
#include "Commands/Commands.h"

#include "json.hpp"
#include "Helper.h"

#pragma comment(lib, "ArkApi.lib")

namespace Core {
	void Load()
	{
		// Log init
		Log::Get().Init(PLUGIN_NAME);
		Log::GetLog()->info("Initializing {} v{}", PLUGIN_NAME, PLUGIN_VERSION);

		// Read Config
		try {
			Data::config = Helper::ReadConfig();
		}
		catch (std::exception& e) {
			Log::GetLog()->error("Error Reading config : {}", e.what());
			return;
		}

		// Add commands
		// Note: this won't reload with the reload command
		Data::PVPCommand = Data::config["1V1 Command"];
		Data::AcceptCommand = Data::config["Accept Command"];
		Data::DeclineCommand = Data::config["Decline Command"];
		Data::DuelCommand = Data::config["Duel Command"];

		Log::GetLog()->info("Loaded plugin!");

		Hooks::Load();
		Commands::Load();
	}

	void Unload() {
		Hooks::Unload();
		Commands::Unload();
	}
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
	Core::Load();
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
	Core::Unload();
}


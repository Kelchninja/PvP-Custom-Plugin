#include "Commands.h"

void HandlePVPCommand(AShooterPlayerController* shooter_controller, FString* message2, bool bIsDuel)
{
	FString message = *message2;

	TArray<FString> parsed;
	message.ParseIntoArray(parsed, TEXT(" "), true);

	// check is already in match
	bool bIsPendingCombat;
	if (Combat::IsPlayerInMatches(shooter_controller->GetUniqueNetIdAsUINT64(), &bIsPendingCombat))
	{
		if (bIsPendingCombat)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "You have a pending match, use /accept to accept it!");
			return;
		}
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "You have a pending match, wait for the user to accept it!");
		return;
	}

	// Look for first argument which has to be an CharacterName 
	if (parsed.Num() > 1)
	{
		FString CharacterName = parsed[1];
		TArray<AShooterPlayerController*> players = ArkApi::GetApiUtils().FindPlayerFromCharacterName(CharacterName, ESearchCase::CaseSensitive, true);

		if (players.Num() == 0)
		{
			// No match
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "No player matches the given character Name!");
			return;
		}

		AShooterPlayerController* controller = players[0];

		// Checks for character
		if (!controller->GetPlayerCharacter())
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Given player does not have a valid character!");
			return;
		}

		// build struct
		RequestMatch Rq;
		Rq.ChallengedSteamID = controller->GetUniqueNetIdAsUINT64();
		Rq.RequestTime = ArkApi::GetApiUtils().GetWorld()->TimeSecondsField();
		Rq.bIsDuel = bIsDuel;

		Combat::matches[shooter_controller->GetUniqueNetIdAsUINT64()] = Rq;

		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Waiting for player to accept");

		std::string typeMatch = "1v1";
		if (bIsDuel)
			typeMatch = "Duel";

		// Request to the challenged
		std::string msg = shooter_controller->GetPlayerCharacter()->PlayerNameField().ToString() + " Requested you for a" + typeMatch + ", use / accept to accept the match";
		ArkApi::GetApiUtils().SendServerMessage(controller, FColorList::Orange, msg.c_str());
	}
	else {
		// Did not provide a character name
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Usage: /1v1 CharacterName");
		return;
	}
}

// Handles the 1v1 command
void ChallengeCommand(AShooterPlayerController* player_controller, FString* message2, EChatSendMode::Type /*Unusued*/) {
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	HandlePVPCommand(shooter_controller, message2, false);
}

// Handles the duel command
void DuelCommand(AShooterPlayerController* player_controller, FString* message2, EChatSendMode::Type /*Unusued*/) {
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	HandlePVPCommand(shooter_controller, message2, true);
}

void DeclineCommand(AShooterPlayerController* player_controller, FString* message2, EChatSendMode::Type /*Unusued*/) {
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	uint64 steamID = shooter_controller->GetUniqueNetIdAsUINT64();

	for (auto pair : Combat::matches) {
		uint64 ChallengerSteamID = pair.first;
		uint64 ChallengedSteamID = pair.second.ChallengedSteamID;

		if (ChallengedSteamID == steamID || pair.first == steamID)
		{
			AShooterPlayerController* challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengedSteamID);
			AShooterPlayerController* challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengerSteamID);

			if (challenger)
				ArkApi::GetApiUtils().SendServerMessage(challenger, FColorList::Red, "Match cancelled!");

			if (challenged)
				ArkApi::GetApiUtils().SendServerMessage(challenged, FColorList::Red, "Match cancelled!");
			
			Combat::matches.erase(pair.first);
			return;
		}

	}
}

// Handles accept command
void AcceptCommand(AShooterPlayerController* player_controller, FString* message2, EChatSendMode::Type /*Unusued*/) {
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
	
	uint64 steamID = shooter_controller->GetUniqueNetIdAsUINT64();
	
	for (auto pair : Combat::matches) {	
		uint64 ChallengerSteamID = pair.first;
		uint64 ChallengedSteamID = pair.second.ChallengedSteamID;
		
		if (ChallengedSteamID == steamID)
		{
			// He was challenged
			AShooterPlayerController* challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengerSteamID);

			// Safety checks
			if (!challenger->GetPlayerCharacter() || !shooter_controller->GetPlayerCharacter())
			{
				if (challenger)
					ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Character not found!");

				if (shooter_controller)
					ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Character not found!");
				return;
			}
			// distance check
			if (FVector::Dist(challenger->GetPlayerCharacter()->RootComponentField()->RelativeLocationField(), shooter_controller->GetPlayerCharacter()->RootComponentField()->RelativeLocationField()) > 2000)
			{
				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "You need to be near the other person!");
				return;
			}

			if (!challenger || !challenger->GetPlayerCharacter())
			{
				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "The challenger is not online or does not have a character, MATCH CANCELLED!");
				return;
			}

			FString FailReason;
			bool Succeed = Combat::Start1V1(challenger->GetPlayerCharacter()->RootComponentField()->RelativeLocationField(), ChallengerSteamID, ChallengedSteamID, Data::config["Arena"].value("Bubble Size", 6000), Data::config["PVP Time"], &FailReason, pair.second.bIsDuel);
				
			if (!Succeed)
				Log::GetLog()->info("1V1 Start failed: {}", FailReason.ToString());

			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Match accepted!");
			ArkApi::GetApiUtils().SendServerMessage(challenger, FColorList::Green, "Match accepted!");
			Combat::matches.erase(pair.first);
		}
	}
}

void ReloadPluginCmd(APlayerController* player_controller, FString* /*unused*/, bool /*unused*/)
{
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
	try
	{
		Data::config = Helper::ReadConfig();

	}
	catch (const std::exception& error)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Failed to reload config");
		Log::GetLog()->error(error.what());
		return;
	}

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Config reloaded successfully !");
}

void ReloadPluginRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
{
	FString reply;
	try
	{
		Data::config = Helper::ReadConfig();

	}
	catch (const std::exception& error)
	{
		reply = error.what();
		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
		Log::GetLog()->error(error.what());
		return;
	}

	reply = "Config reloaded successfully !";
	rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

void CheckCombats()
{
	// Timeout check
	for (auto pair : Combat::matches)
	{
		// Timeout check
		if (ArkApi::GetApiUtils().GetWorld()->TimeSecondsField() - pair.second.RequestTime > Data::config["Timeout Time"])
		{
			uint64 ChallengedSteamID = pair.second.ChallengedSteamID;
			uint64 ChallengerSteamID = pair.first;

			AShooterPlayerController* Challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengerSteamID);
			AShooterPlayerController* Challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengedSteamID);

			if (Challenger)
				ArkApi::GetApiUtils().SendServerMessage(Challenger, FColorList::Red, "1v1 Timedout");

			if (Challenged)
				ArkApi::GetApiUtils().SendServerMessage(Challenged, FColorList::Red, "1v1 Timedout");

			Combat::matches.erase(pair.first);
		}
	}

	for (auto pair : Combat::OnGoingMatches)
	{
		uint64 ChallengerSteamID = pair.first;
		uint64 ChallengedSteamID = pair.second.ChallengedSteamID;

		AShooterPlayerController* Challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengerSteamID);
		AShooterPlayerController* Challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengedSteamID);

		if (!Challenged || !Challenger || !Challenger->GetPlayerCharacter() || !Challenged->GetPlayerCharacter())
		{
			if (Challenger)
				ArkApi::GetApiUtils().SendServerMessage(Challenger, FColorList::Red, "Match cancelled: Player was defeated or left");

			if (Challenged)
				ArkApi::GetApiUtils().SendServerMessage(Challenged, FColorList::Red, "Match cancelled: Player was defeated or left");

			Combat::OnGoingMatches.erase(pair.first);
			continue;
		}

		int TimeSinceStart = ArkApi::GetApiUtils().GetWorld()->TimeSecondsField() - pair.second.StartTime;

		if (TimeSinceStart > Data::config["PVP Time"])
		{
			if (Challenger)
				ArkApi::GetApiUtils().SendServerMessage(Challenger, FColorList::Red, "Match cancelled: PVP time ran out!");

			if (Challenged)
				ArkApi::GetApiUtils().SendServerMessage(Challenged, FColorList::Red, "Match cancelled: PVP time ran out!");

			Combat::OnGoingMatches.erase(pair.first);
			continue;
		}

		if (!Combat::IsInsideSphere(Challenged, Challenged, Data::config["Arena"]["Bubble Size"]))
		{
			FString message = FString("Return inside the bubble!");
			Challenged->ClientServerNotificationSingle(&message, FColorList::Red, 2, 3, nullptr, nullptr, 0);

			// "+1000" to give some range before being killed
			if (!Combat::IsInsideSphere(Challenged, Challenged, Data::config["Arena"]["Bubble Size"] + 1000))
				Challenged->GetPlayerCharacter()->Suicide();
		}

		if (!Combat::IsInsideSphere(Challenger, Challenger, Data::config["Arena"]["Bubble Size"]))
		{
			FString message = FString("Return inside the bubble!");
			Challenger->ClientServerNotificationSingle(&message, FColorList::Red, 2, 3, nullptr, nullptr, 0);

			// "+1000" to give some range before being killed
			if (!Combat::IsInsideSphere(Challenger, Challenger, Data::config["Arena"]["Bubble Size"] + 1000)) 
				Challenger->GetPlayerCharacter()->Suicide();

		}

		// loop trough all players in a range
		TArray<AActor*> actors = ArkApi::GetApiUtils().GetAllActorsInRange(pair.second.StartLocation, Data::config["Arena"]["Bubble Size"] * 2, EServerOctreeGroup::PLAYERS_CONNECTED);
		for (AActor* actor : actors)
		{
			if (actor->IsA(AShooterCharacter::StaticClass()))
			{
				AShooterCharacter* character = (AShooterCharacter*)actor;
				if (!character) continue;
				AShooterPlayerController* controller = ArkApi::GetApiUtils().FindControllerFromCharacter(character);
				if (!controller) continue;
				Combat::DrawSphere(controller, pair.second.StartLocation);
			}
		}
	}
}

namespace Commands {
	void Load() {
		auto& commands = ArkApi::GetCommands();

		// Timer callback to check for timeout and ongoingmatches
		commands.AddOnTimerCallback(PLUGIN_NAME"::Check1V1", &CheckCombats);
		
		// 1V1 Command
		commands.AddChatCommand(FString(Data::PVPCommand), &ChallengeCommand); // /1v1 adasdas
		commands.AddChatCommand(FString(Data::AcceptCommand), &AcceptCommand); // /1v1 adasdas
		commands.AddChatCommand(FString(Data::DuelCommand), &DuelCommand); // /1v1 adasdas
		commands.AddChatCommand(FString(Data::DeclineCommand), &DeclineCommand); // /accept
		
		// Reload commands
		commands.AddConsoleCommand(PLUGIN_NAME".reload", &ReloadPluginCmd);
		commands.AddRconCommand(PLUGIN_NAME".reload", &ReloadPluginRcon);
	}

	void Unload() {
		auto& commands = ArkApi::GetCommands();

		// Timer callback to check for timeout and ongoingmatches
		commands.RemoveOnTimerCallback(PLUGIN_NAME"::Check1V1");

		commands.RemoveChatCommand(FString(Data::PVPCommand));
		commands.RemoveChatCommand(FString(Data::AcceptCommand));
		commands.RemoveChatCommand(FString(Data::DuelCommand));
		commands.RemoveChatCommand(FString(Data::DeclineCommand));

		commands.RemoveConsoleCommand(PLUGIN_NAME".reload");
		commands.RemoveRconCommand(PLUGIN_NAME".reload");
	}
}
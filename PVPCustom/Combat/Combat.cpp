#include "Combat.h"

namespace Combat {
	bool IsPlayerInMatches(uint64 steamID, bool* bIsPendingCombat)
	{
		if (matches.find(steamID) != matches.end())
			return true;

		for (auto pair : matches)
		{
			if (pair.second.ChallengedSteamID == steamID)
			{
				*bIsPendingCombat = true;
				return true;
			}
		}

		return false;
	}

	// Starts the 1v1
	bool Start1V1(FVector StartLocation, uint64 ChallengerSteamID, uint64 ChallengedSteamID, int sphereRadius, int PVPTime, FString* FailReason, bool bIsDuel)
	{
		AShooterPlayerController* Challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengerSteamID);
		AShooterPlayerController* Challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(ChallengedSteamID);

		// Safety checks
		if (!Challenger || !Challenger->GetPlayerCharacter() || !Challenged || !Challenged->GetPlayerCharacter())
		{
			*FailReason = FString("Failed to find character");
			return false;
		}

		// init struct 
		OnGoinMatch OGM;
		OGM.ChallengedSteamID = Challenged->GetUniqueNetIdAsUINT64();
		OGM.StartLocation = StartLocation;
		OGM.StartTime = ArkApi::GetApiUtils().GetWorld()->TimeSecondsField();
		OGM.bIsDuel = bIsDuel;

		// Add
		OnGoingMatches[Challenger->GetUniqueNetIdAsUINT64()] = OGM;

		return true;
	}

	void DrawSphere(AShooterPlayerController* controller, FVector location) {
		// "SpawnBubble" -> FVector location, int Bubble Size
		
		

		// get color from config
		FColor color = FColor{ Data::config["Arena"]["Bubble Color"]["R"], Data::config["Arena"]["Bubble Color"]["G"], Data::config["Arena"]["Bubble Color"]["B"], Data::config["Arena"]["Bubble Color"]["A"] };

		// draw sphere
		controller->MulticastDrawDebugSphere(location, Data::config["Arena"].value("Bubble Size", 4000), 16, color, 2, true);
	}

	bool IsInsideSphere(AShooterPlayerController* controller, AShooterPlayerController* matchPlayer, int SphereRadius) {
		if (!controller || !controller->GetPlayerCharacter()) return false;

		if (OnGoingMatches.find(matchPlayer->GetUniqueNetIdAsUINT64()) != OnGoingMatches.end())
		{
			FVector SphereStart = OnGoingMatches[matchPlayer->GetUniqueNetIdAsUINT64()].StartLocation;
			FVector controllerLocation = controller->GetPlayerCharacter()->RootComponentField()->RelativeLocationField();

			if (SphereStart.X == 0 || SphereStart.Y == 0 || SphereStart.Z == 0) return true;

			if (FVector::Dist(SphereStart, controllerLocation) < SphereRadius)
				return true;
		}
		else {
			for (auto pair : OnGoingMatches)
			{
				if (pair.second.ChallengedSteamID == controller->GetUniqueNetIdAsUINT64())
				{
					FVector SphereStart = pair.second.StartLocation;
					FVector controllerLocation = controller->GetPlayerCharacter()->RootComponentField()->RelativeLocationField();
					
					if (SphereStart.X == 0 || SphereStart.Y == 0 || SphereStart.Z == 0) return true;

					if (FVector::Dist(SphereStart, controllerLocation) < SphereRadius)
						return true;
				}
			}
		}

		return false;
	}

	bool IsPlayerInOnGoinMatches(uint64 steamID, bool* bIsPendingCombat)
	{
		if (OnGoingMatches.find(steamID) != OnGoingMatches.end())
			return true;

		for (auto pair : OnGoingMatches)
		{
			if (pair.second.ChallengedSteamID == steamID)
			{
				*bIsPendingCombat = true;
				return true;
			}
		}

		return false;
	}
}
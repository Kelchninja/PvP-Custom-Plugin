#pragma once
#include "API/ARK/Ark.h"
#include "../Runtime/Runtime.hpp"
#include "../Helper.h"

// Handles matches between players
// First argument : the challenging guy (/1v1)
// Second argument : Request structure

struct RequestMatch {
	uint64 ChallengedSteamID;
	int RequestTime;
	bool bIsDuel;
};

struct OnGoinMatch {
	int StartTime;
	FVector StartLocation;
	bool bIsDuel;
	uint64 ChallengedSteamID;
};

namespace Combat {
	inline std::map<uint64, RequestMatch> matches;
	inline std::map<uint64, OnGoinMatch> OnGoingMatches;

	void DrawSphere(AShooterPlayerController* controller, FVector location);

	bool IsPlayerInMatches(uint64 steamID, bool* bIsPendingCombat);
	bool Start1V1(FVector StartLocation, uint64 ChallengerSteamID, uint64 ChallengedSteamID, int sphereRadius, int PVPTime, FString* FailReason, bool bIsDuel);
	bool IsInsideSphere(AShooterPlayerController* controller, AShooterPlayerController* matchPlayer, int SphereRadius);

	bool IsPlayerInOnGoinMatches(uint64 steamID, bool* bIsPendingCombat);
}
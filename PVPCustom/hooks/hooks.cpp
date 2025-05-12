#include "hooks.h"

DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(APrimalCharacter_TakeDamage, float, APrimalCharacter*, float, FDamageEvent*, AController*, AActor*);

float Hook_APrimalCharacter_TakeDamage(APrimalCharacter* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser){
	if (!_this) return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	if (!_this->IsA(AShooterCharacter::StaticClass())) return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	AShooterCharacter* characterA = (AShooterCharacter*)_this;

	if (DamageCauser && DamageCauser->IsA(AShooterCharacter::StaticClass()))
	{
		AShooterCharacter* character = (AShooterCharacter*)DamageCauser;
		if (!character || !characterA) return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
		AShooterPlayerController* controllerDamageCauser = ArkApi::GetApiUtils().FindControllerFromCharacter(character);
		AShooterPlayerController* controller = ArkApi::GetApiUtils().FindControllerFromCharacter(characterA);
		if (!controller || !controllerDamageCauser) return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);

		for (APrimalBuff* buff : controller->GetPlayerCharacter()->BuffsField())
		{
			if (buff->NameField().ToString().ToString()._Starts_with("BP_VTXPvPModBuff_C"))
			{
				Log::GetLog()->info("Got Buff: {}", buff->NameField().ToString().ToString());
				static FName BubbleName = FName("SpawnBubble", EFindName::FNAME_Add);
				UFunction* func = buff->FindFunctionChecked(BubbleName);
				if (!func) continue;
				Log::GetLog()->info("Got Func: {}", func->NameField().ToString().ToString());

				buff->ProcessEvent(func, &character->RootComponentField()->RelativeLocationField());
				TArray<AActor*> actors= ArkApi::GetApiUtils().GetAllActorsInRange(character->RootComponentField()->RelativeLocationField(), 10000, EServerOctreeGroup::MAX);
				FString vortex2 = FString("Blueprint'/Game/Mods/VTXPvPMod/SMA_VortexBubble.SMA_VortexBubble'");
				UObject* loadedObject2 = UVictoryCore::BPLoadObject(&vortex2);
				if (loadedObject2)
				{
					Log::GetLog()->info("loaded object for: {}", loadedObject2->NameField().ToString().ToString());
				}
			}
		}
		bool bPendingCombat = false;
		if (!Combat::IsPlayerInOnGoinMatches(controller->GetUniqueNetIdAsUINT64(), &bPendingCombat))
			return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);

		// Check if the damage causer and controller is in an ongoing match
		if (Combat::IsPlayerInOnGoinMatches(controller->GetUniqueNetIdAsUINT64(), &bPendingCombat) && Combat::IsPlayerInOnGoinMatches(controllerDamageCauser->GetUniqueNetIdAsUINT64(), &bPendingCombat))
			return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);

		FString message = FString("You cannot damage players that are in an ongoing match!");
		controllerDamageCauser->ClientServerNotificationSingle(&message, FColorList::Red, 2, 3, nullptr, nullptr, 0);

		return 0;
	}

	return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!_this) return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
	AShooterPlayerController* controller = ArkApi::GetApiUtils().FindControllerFromCharacter(_this);
	if (!controller) return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);

	if (!Killer) {
		for (auto pair : Combat::OnGoingMatches)
		{
			if (pair.first == controller->GetUniqueNetIdAsUINT64() || pair.second.ChallengedSteamID == controller->GetUniqueNetIdAsUINT64())
			{
				if (pair.second.bIsDuel)
				{
					AShooterPlayerController* Challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(pair.second.ChallengedSteamID);
					AShooterPlayerController* Challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(pair.first);
					if (Challenged)
						ArkApi::GetApiUtils().SendServerMessage(Challenged, FColorList::Red, "Match Cancelled: Player left!");
					if (Challenger)
						ArkApi::GetApiUtils().SendServerMessage(Challenger, FColorList::Red, "Match Cancelled: Player left!");

					Combat::OnGoingMatches.erase(pair.first);
					_this->MyCharacterStatusComponentField()->ModifyCurrentStatusValue(EPrimalCharacterStatusValue::Health, _this->ReplicatedCurrentHealthField(), true, true, true, true, nullptr, true, true);

					return 0;
				}
			}
		}
	}
	// Check if Killer is a Player
	if (Killer && Killer->IsA(AShooterPlayerController::StaticClass()))
	{
		// Obtain his controller
		AShooterPlayerController* KillerController = (AShooterPlayerController*)Killer;
		if (!KillerController) return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
		
		bool PendingCombat = false;
		if (Combat::IsPlayerInOnGoinMatches(controller->GetUniqueNetIdAsUINT64(), &PendingCombat))
		{
			if (!PendingCombat)
			{
				// Obtain structure + check if its a duel combat
				OnGoinMatch match = Combat::OnGoingMatches[controller->GetUniqueNetIdAsUINT64()];
				if (!match.bIsDuel) return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);

				AShooterPlayerController* Challenged = ArkApi::GetApiUtils().FindPlayerFromSteamId(match.ChallengedSteamID);
				if (Challenged)
					ArkApi::GetApiUtils().SendServerMessage(Challenged, FColorList::Green, "You won!");

				Combat::OnGoingMatches.erase(controller->GetUniqueNetIdAsUINT64());

				if (Data::config["Duel"].value("SetMaxHealth", true))
				{
					// Set Max health
					if (Challenged->GetPlayerCharacter())
						Challenged->GetPlayerCharacter()->MyCharacterStatusComponentField()->ModifyCurrentStatusValue(EPrimalCharacterStatusValue::Health, _this->ReplicatedMaxHealthField(), true, true, true, true, nullptr, true, true);
					
					_this->MyCharacterStatusComponentField()->ModifyCurrentStatusValue(EPrimalCharacterStatusValue::Health, _this->ReplicatedMaxHealthField(), true, true, true, true, nullptr, true, true);
				}
				return false;
			}

			if (PendingCombat)
			{
				for (auto pair : Combat::OnGoingMatches)
				{
					if (!pair.second.bIsDuel) continue;

					if (pair.second.ChallengedSteamID == controller->GetUniqueNetIdAsUINT64())
					{
						AShooterPlayerController* Challenger = ArkApi::GetApiUtils().FindPlayerFromSteamId(pair.first);
						if (Challenger)
							ArkApi::GetApiUtils().SendServerMessage(Challenger, FColorList::Green, "You won!");

						Combat::OnGoingMatches.erase(pair.first);

						if (Data::config["Duel"].value("SetMaxHealth", true))
						{
							// Set Max health
							if (Challenger->GetPlayerCharacter())
								Challenger->GetPlayerCharacter()->MyCharacterStatusComponentField()->ModifyCurrentStatusValue(EPrimalCharacterStatusValue::Health, _this->ReplicatedMaxHealthField(), true, true, true, true, nullptr, true, true);

							_this->MyCharacterStatusComponentField()->ModifyCurrentStatusValue(EPrimalCharacterStatusValue::Health, _this->ReplicatedMaxHealthField(), true, true, true, true, nullptr, true, true);
						}
						
						return false;
					}
				}
			}
		}
	}

	return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
}

namespace Hooks {
	void Load() {
		ArkApi::GetHooks().SetHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die, &AShooterCharacter_Die_original);
		ArkApi::GetHooks().SetHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage, &APrimalCharacter_TakeDamage_original);
	}

	void Unload() {
		ArkApi::GetHooks().DisableHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die);
		ArkApi::GetHooks().DisableHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage);
	}
}
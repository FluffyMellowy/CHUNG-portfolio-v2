#include "Core/RunGameState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/RunGameInstance.h"
#include "Quietus.h"

ARunGameState* ARunGameState::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetGameState<ARunGameState>() : nullptr;
}

void ARunGameState::AddShard()
{
	++HeldShards;

	UE_LOG(LogQuietus, Log, TEXT("Dream shard found (%d/%d)"), GetFoundShards(), RequiredShards);
	OnObjectiveChanged.Broadcast();
}

void ARunGameState::SetRequiredShards(int32 Count)
{
	RequiredShards = FMath::Max(1, Count);
	OnObjectiveChanged.Broadcast();
}

int32 ARunGameState::InsertShards()
{
	const int32 Count = FMath::Min(HeldShards, RequiredShards - InsertedShards);
	if (bExitOpen || Count <= 0)
	{
		return 0;
	}

	HeldShards -= Count;
	InsertedShards += Count;
	bExitOpen = InsertedShards >= RequiredShards;

	UE_LOG(LogQuietus, Log, TEXT("Inserted %d shard(s) into the exit door (%d/%d)%s"),
		Count, InsertedShards, RequiredShards, bExitOpen ? TEXT(" -> door opens") : TEXT(""));
	OnObjectiveChanged.Broadcast();
	return Count;
}

void ARunGameState::EnterSafeZone()
{
	if (bInSafeZone || IsDreamOver())
	{
		return;
	}
	bInSafeZone = true;

	UE_LOG(LogQuietus, Log, TEXT("Player reached the safe zone"));
	OnSafeZoneEntered.Broadcast();
	OnObjectiveChanged.Broadcast();
}

void ARunGameState::AddNote()
{
	++CollectedNotes;
	OnObjectiveChanged.Broadcast();
}

void ARunGameState::TriggerEscape()
{
	FinishRound(/*bWon=*/ true);
}

void ARunGameState::TriggerGameOver()
{
	if (IsDreamOver())
	{
		return;
	}

	// 命が残っていれば、この夢だけを終わらせて作り直しを待つ
	URunGameInstance* GI = GetGameInstance<URunGameInstance>();
	if (GI && GI->LoseLife())
	{
		bLifeLost = true;
		UE_LOG(LogQuietus, Log, TEXT("Player caught - still dreaming (%d lives left)"), GI->GetLivesRemaining());
		OnLifeLost.Broadcast(GI->GetLivesRemaining());
		return;
	}
	FinishRound(/*bWon=*/ false);
}

void ARunGameState::FinishRound(bool bWon)
{
	if (IsDreamOver())
	{
		return;
	}
	bEscaped = bWon;
	bGameOver = !bWon;

	UE_LOG(LogQuietus, Log, TEXT("%s"), bWon ? TEXT("Player escaped - WIN") : TEXT("Player caught - GAME OVER"));
	OnRoundFinished.Broadcast(bWon);
}

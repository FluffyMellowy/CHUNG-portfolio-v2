#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RunGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class ARunGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARunGameMode();
};

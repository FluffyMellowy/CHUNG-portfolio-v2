#include "RunGameMode.h"
#include "Core/RunGameState.h"

ARunGameMode::ARunGameMode()
{
	// このGameModeを使うレベルでは目標進行の管理にARunGameStateを使う
	GameStateClass = ARunGameState::StaticClass();
}

#include "Generation/LevelGenPreset.h"
#include "Enemies/EnemyCharacter.h"
#include "Interactables/Balloon.h"
#include "Interactables/Battery.h"
#include "Interactables/Cabinet.h"
#include "Interactables/Door.h"
#include "Environment/WakeStairs.h"
#include "Interactables/DreamShard.h"
#include "Interactables/ExitDoor.h"
#include "Interactables/Locker.h"
#include "Interactables/NoteItem.h"
#include "Interactables/Throwable.h"

ULevelGenPreset::ULevelGenPreset()
{
	// 見た目付きの BP を割り当てるまでは C++ クラスのまま動くようにしておく
	Objectives.ShardClass = ADreamShard::StaticClass();
	Objectives.ExitStairsClass = AWakeStairs::StaticClass();
	Doors.ExitDoorClass = AExitDoor::StaticClass();
	Doors.InteriorDoorClass = ADoor::StaticClass();
	Enemies.EnemyClass = AEnemyCharacter::StaticClass();
	Items.BatteryClass = ABattery::StaticClass();
	Items.ThrowableClass = AThrowable::StaticClass();
	Items.NoteClass = ANoteItem::StaticClass();
	Items.CabinetClass = ACabinet::StaticClass();
	Items.LockerClass = ALocker::StaticClass();
	Items.BalloonClass = ABalloon::StaticClass();
}

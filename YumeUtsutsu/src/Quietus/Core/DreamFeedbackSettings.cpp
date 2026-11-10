#include "Core/DreamFeedbackSettings.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"

namespace
{
	/** /Game/Audio/<Group>/<Name>_01.. を Count 個並べた候補（Count が 0 なら番号なしの 1 個） */
	FSfxCue MakeCue(const TCHAR* Group, const TCHAR* Name, int32 Count = 0, float PitchJitter = 0.0f)
	{
		FSfxCue Cue;
		Cue.PitchJitter = PitchJitter;
		auto Add = [&Cue, Group](const FString& Asset)
		{
			Cue.Sounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/Audio/%s/%s.%s"), Group, *Asset, *Asset))));
		};
		if (Count == 0)
		{
			Add(Name);
		}
		for (int32 i = 1; i <= Count; ++i)
		{
			Add(FString::Printf(TEXT("%s_%02d"), Name, i));
		}
		return Cue;
	}

	FSfxAttenuation MakeAttenuation(float Inner, float End, bool bOcclusion = false)
	{
		FSfxAttenuation Att;
		Att.InnerRadius = Inner;
		Att.FalloffEnd = End;
		Att.bOcclusion = bOcclusion;
		return Att;
	}
}

UDreamFeedbackSettings::UDreamFeedbackSettings()
{
	MasterClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Audio/Classes/SC_Master.SC_Master")));

	StepWalk = MakeCue(TEXT("Player"), TEXT("S_Player_Step_Walk"), 8, 0.03f);
	StepRun = MakeCue(TEXT("Player"), TEXT("S_Player_Step_Run"), 6, 0.03f);
	StepCrouch = MakeCue(TEXT("Player"), TEXT("S_Player_Step_Crouch"), 5, 0.03f);
	BreathRun = MakeCue(TEXT("Player"), TEXT("S_Player_Breath_Run_Loop"));
	BreathExhausted = MakeCue(TEXT("Player"), TEXT("S_Player_Breath_Exhausted_Loop"));
	FlashlightOn = MakeCue(TEXT("Player"), TEXT("S_Flashlight_On"));
	FlashlightOff = MakeCue(TEXT("Player"), TEXT("S_Flashlight_Off"));
	Heartbeat = MakeCue(TEXT("Player"), TEXT("S_Heartbeat"), 4);

	PickupBattery = MakeCue(TEXT("Interact"), TEXT("S_Pickup_Battery"));
	PickupNote = MakeCue(TEXT("Interact"), TEXT("S_Pickup_Note"));
	PickupGeneric = MakeCue(TEXT("Interact"), TEXT("S_Pickup_Generic"));
	DoorOpen = MakeCue(TEXT("Interact"), TEXT("S_Door_Open"), 2, 0.03f);
	DoorClose = MakeCue(TEXT("Interact"), TEXT("S_Door_Close"), 1, 0.03f);
	LockerOpen = MakeCue(TEXT("Interact"), TEXT("S_Locker_Open"), 2, 0.03f);
	LockerClose = MakeCue(TEXT("Interact"), TEXT("S_Locker_Close"), 2, 0.03f);
	LockerBust = MakeCue(TEXT("Interact"), TEXT("S_Locker_Bust"));
	CabinetDrawer = MakeCue(TEXT("Interact"), TEXT("S_Cabinet_Drawer"), 0, 0.03f);
	ThrowableImpact = MakeCue(TEXT("Interact"), TEXT("S_Throwable_Impact"), 4, 0.03f);
	InteractAttenuation = MakeAttenuation(150.0f, 1800.0f);
	ImpactAttenuation = MakeAttenuation(200.0f, 2500.0f);

	ClownStep = MakeCue(TEXT("Clown"), TEXT("S_Clown_Step"), 6, 0.03f);
	ClownBell = MakeCue(TEXT("Clown"), TEXT("S_Clown_Bell"), 6, 0.03f);
	ClownSqueak = MakeCue(TEXT("Clown"), TEXT("S_Clown_Squeak"), 4, 0.03f);
	ClownStepAttenuation = MakeAttenuation(200.0f, 3000.0f, true);

	RoomTone = MakeCue(TEXT("Ambience"), TEXT("S_Amb_RoomTone_Loop"));
	DarkVent = MakeCue(TEXT("Ambience"), TEXT("S_Amb_DarkVent_Loop"));
	LightBuzz = MakeCue(TEXT("Ambience"), TEXT("S_Amb_LightBuzz_Loop"));
	LightBuzzAttenuation = MakeAttenuation(100.0f, 800.0f);

	ChaseStinger = MakeCue(TEXT("Chase"), TEXT("S_Chase_Stinger"), 2);
	ChaseBed = MakeCue(TEXT("Chase"), TEXT("S_Chase_Bed_Loop"));
}

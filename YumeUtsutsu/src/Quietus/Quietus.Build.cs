using UnrealBuildTool;

public class Quietus : ModuleRules
{
	public Quietus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"EngineCameras"
		});

		// AudioMixer は紹介動画の録音（RunGameInstance の CaptureStart、出荷ビルドでは使わない）
		PrivateDependencyModuleNames.AddRange(new string[] { "AudioMixer", "AudioMixerCore", "NonRealtimeAudioRenderer" });

		PublicIncludePaths.AddRange(new string[] {
			"Quietus"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

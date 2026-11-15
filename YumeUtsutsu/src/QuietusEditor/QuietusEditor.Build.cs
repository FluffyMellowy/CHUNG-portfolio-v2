using UnrealBuildTool;

public class QuietusEditor : ModuleRules
{
	public QuietusEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"AIModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"AIGraph",
			"BehaviorTreeEditor",
			"SlateCore",
			"AssetRegistry"
		});
	}
}

using UnrealBuildTool;
using System.Collections.Generic;

public class QuietusEditorTarget : TargetRules
{
	public QuietusEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Quietus");
		ExtraModuleNames.Add("QuietusEditor");
	}
}

using UnrealBuildTool;
public class BreakoutGameEditorTarget : TargetRules
{
    public BreakoutGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "BreakoutGame", "BreakoutGameEditor" });
    }
}

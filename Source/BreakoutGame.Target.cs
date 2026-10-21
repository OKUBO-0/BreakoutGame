using UnrealBuildTool;
public class BreakoutGameTarget : TargetRules
{
    public BreakoutGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BreakoutGame");
    }
}

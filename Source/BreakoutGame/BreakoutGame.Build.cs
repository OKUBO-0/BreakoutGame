using UnrealBuildTool;
public class BreakoutGame : ModuleRules
{
    public BreakoutGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG", "EnhancedInput", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "MovieScene" });
    }
}

using UnrealBuildTool;
public class BreakoutGameEditor : ModuleRules
{
    public BreakoutGameEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "BreakoutGame", "UnrealEd", "UMG", "UMGEditor",
            "Slate", "SlateCore", "MovieScene", "MovieSceneTracks", "AssetRegistry", "Kismet", "KismetCompiler"
        });
    }
}

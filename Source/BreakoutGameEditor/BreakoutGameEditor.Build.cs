using UnrealBuildTool;
public class BreakoutGameEditor : ModuleRules
{
    public BreakoutGameEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Source/Editor/UMGEditor/Private"));
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "BreakoutGame", "UnrealEd", "UMG", "UMGEditor",
            "Slate", "SlateCore", "MovieScene", "MovieSceneTracks", "AssetRegistry", "Kismet", "KismetCompiler", "EnhancedInput", "InputCore",
            "AssetTools", "AudioEditor", "EngineSettings", "BlueprintGraph"
        });
    }
}

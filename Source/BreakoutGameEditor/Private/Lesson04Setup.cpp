#include "BreakoutLesson02.h"
#include "BreakoutLesson04.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetImportTask.h"
#include "IAssetTools.h"
#include "Components/AudioComponent.h"
#include "Sound/AmbientSound.h"
#include "EngineUtils.h"
#include "Engine/Blueprint.h"
#include "GameMapsSettings.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "EdGraph/EdGraphNode.h"

namespace
{
void SaveLessonAsset(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

UMaterial* EnsureBlockMaterial()
{
    const TCHAR* Path = TEXT("/Game/Breakout/BlockMaterial");
    if (UMaterial* Existing = LoadObject<UMaterial>(nullptr, Path)) return Existing;
    UMaterial* Material = NewObject<UMaterial>(CreatePackage(Path), TEXT("BlockMaterial"), RF_Public | RF_Standalone);
    UMaterialExpressionVectorParameter* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
    Color->ParameterName = TEXT("BaseColor");
    Color->DefaultValue = FLinearColor::White;
    Color->MaterialExpressionEditorX = -400;
    Material->GetExpressionCollection().AddExpression(Color);
    Material->GetEditorOnlyData()->BaseColor.Expression = Color;
    UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material);
    Roughness->R = 0.6f;
    Roughness->MaterialExpressionEditorX = -400;
    Roughness->MaterialExpressionEditorY = 200;
    Material->GetExpressionCollection().AddExpression(Roughness);
    Material->GetEditorOnlyData()->Roughness.Expression = Roughness;
    Material->PostEditChange();
    FAssetRegistryModule::AssetCreated(Material);
    SaveLessonAsset(Material);
    return Material;
}

USoundWave* EnsureWave(const TCHAR* Name)
{
    const FString AssetPath = FString(TEXT("/Game/Breakout/Audio/")) + Name;
    if (USoundWave* Existing = LoadObject<USoundWave>(nullptr, *AssetPath)) return Existing;
    UAssetImportTask* Task = NewObject<UAssetImportTask>();
    Task->Filename = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("SourceArt/Audio") / (FString(Name) + TEXT(".wav")));
    Task->DestinationPath = TEXT("/Game/Breakout/Audio");
    Task->DestinationName = Name;
    Task->bAutomated = true;
    Task->bSave = true;
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});
    return LoadObject<USoundWave>(nullptr, *AssetPath);
}

USoundCue* EnsureBgm(USoundWave* Wave)
{
    const TCHAR* Path = TEXT("/Game/Breakout/Audio/Breakout_BGM_Cue");
    if (USoundCue* Existing = LoadObject<USoundCue>(nullptr, Path)) return Existing;
    if (!Wave) return nullptr;
    FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("AudioEditor"));
    USoundCue* Cue = NewObject<USoundCue>(CreatePackage(Path), TEXT("Breakout_BGM_Cue"), RF_Public | RF_Standalone);
    USoundNodeLooping* Loop = Cue->ConstructSoundNode<USoundNodeLooping>();
    USoundNodeWavePlayer* Player = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
    Player->SetSoundWave(Wave);
    Loop->bLoopIndefinitely = true;
    if (Loop->ChildNodes.IsEmpty()) Loop->InsertChildNode(0);
    Loop->ChildNodes[0] = Player;
    Loop->GetGraphNode()->NodePosX = -250;
    Player->GetGraphNode()->NodePosX = -550;
    Cue->FirstNode = Loop;
    Cue->LinkGraphNodesFromSoundNodes();
    Cue->PostEditChange();
    FAssetRegistryModule::AssetCreated(Cue);
    SaveLessonAsset(Cue);
    return Cue;
}

void EnsureGameInstance()
{
    const TCHAR* Path = TEXT("/Game/Breakout/BreakoutGameInstance");
    if (!LoadObject<UBlueprint>(nullptr, Path))
    {
        UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UBreakoutGameInstance::StaticClass(), CreatePackage(Path),
            TEXT("BreakoutGameInstance"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
        FKismetEditorUtilities::CompileBlueprint(BP);
        FAssetRegistryModule::AssetCreated(BP);
        SaveLessonAsset(BP);
    }
    // Also update the live CDO so PIE uses the new class without another restart.
    GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(TEXT("/Game/Breakout/BreakoutGameInstance.BreakoutGameInstance_C"));
}

TArray<ABreakoutBlock*> SortedBlocks(UWorld* World)
{
    TArray<ABreakoutBlock*> Blocks;
    for (TActorIterator<ABreakoutBlock> It(World); It; ++It) Blocks.Add(*It);
    Blocks.Sort([](const ABreakoutBlock& A, const ABreakoutBlock& B)
    {
        const FVector P = A.GetActorLocation(), Q = B.GetActorLocation();
        return P.Z == Q.Z ? P.Y < Q.Y : P.Z < Q.Z;
    });
    return Blocks;
}

void EnsureAmbientMusic(UWorld* World, USoundCue* Cue)
{
    if (!Cue) return;
    for (TActorIterator<AAmbientSound> It(World); It; ++It)
        if (It->ActorHasTag(TEXT("Lesson04BGM"))) return;
    AAmbientSound* Music = World->SpawnActor<AAmbientSound>();
    Music->SetActorLabel(TEXT("Breakout_BGM_Cue"));
    Music->SetFolderPath(TEXT("Gameplay"));
    Music->Tags.Add(TEXT("Lesson04BGM"));
    Music->GetAudioComponent()->SetSound(Cue);
    Music->GetAudioComponent()->SetVolumeMultiplier(0.25f);
    Music->GetAudioComponent()->bAllowSpatialization = false;
    Music->GetAudioComponent()->bAutoActivate = true;
}
}

void SetupLesson04()
{
    EnsureBlockMaterial();
    EnsureWave(TEXT("Breakout_SE_Knock"));
    USoundCue* Music = EnsureBgm(EnsureWave(TEXT("Breakout_BGM")));
    EnsureGameInstance();
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || World->GetName() != TEXT("L_InGame")) return;
    ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
    if (!Manager) return;
    if (!Manager->ActorHasTag(TEXT("Lesson04Configured")))
    {
        for (const TCHAR* Path : { TEXT("/Game/Breakout/BP_Block"), TEXT("/Game/Breakout/BP_Ball"), TEXT("/Game/Breakout/BP_GameManager") })
        {
            UBlueprint* BP = LoadObject<UBlueprint>(nullptr, Path);
            if (BP) { FKismetEditorUtilities::CompileBlueprint(BP); SaveLessonAsset(BP); }
        }
        // Compilation can reinstance the placed manager.
        Manager = ABreakoutGameManager::Find(World);
        Manager->Modify();
        Manager->NextLevelName = TEXT("Level2");
        Manager->Tags.Add(TEXT("Lesson04Configured"));
        const TArray<ABreakoutBlock*> Blocks = SortedBlocks(World);
        for (int32 I = 0; I < Blocks.Num(); ++I)
        {
            Blocks[I]->Modify();
            Blocks[I]->Hp = I % 5 + 1;
            Blocks[I]->RerunConstructionScripts();
        }
        EnsureAmbientMusic(World, Music);
        World->MarkPackageDirty();
        FEditorFileUtils::SaveLevel(World->PersistentLevel);
    }
    if (!FPackageName::DoesPackageExist(TEXT("/Game/Level2")))
    {
        const FString Filename = FPackageName::LongPackageNameToFilename(TEXT("/Game/L_InGame"), FPackageName::GetMapPackageExtension());
        UWorld* Second = UEditorLoadingAndSavingUtils::NewMapFromTemplate(Filename, false);
        if (!Second) return;
        ABreakoutGameManager* SecondManager = ABreakoutGameManager::Find(Second);
        if (!SecondManager) return;
        SecondManager->NextLevelName = NAME_None; // No third stage is specified.
        const TArray<ABreakoutBlock*> Blocks = SortedBlocks(Second);
        int32 Index = 0;
        for (int32 Row = 0; Row < 4; ++Row)
            for (int32 Column = 0; Column <= Row && Index < Blocks.Num(); ++Column, ++Index)
            {
                ABreakoutBlock* Block = Blocks[Index];
                Block->SetActorLocation(FVector(0, (Column - Row * 0.5f) * 600, 4200 - Row * 400));
                Block->Hp = (Row + Column) % 5 + 1;
                Block->RerunConstructionScripts();
            }
        UEditorLoadingAndSavingUtils::SaveMap(Second, TEXT("/Game/Level2"));
        UEditorLoadingAndSavingUtils::LoadMap(Filename);
    }
    // Apply the easier introductory layout once, after Level2 has been cloned.
    // Subsequent launches preserve any manual edits to either stage.
    World = GEditor->GetEditorWorldContext().World();
    Manager = World ? ABreakoutGameManager::Find(World) : nullptr;
    if (Manager && World->GetName() == TEXT("L_InGame") &&
        !Manager->ActorHasTag(TEXT("Lesson04EasyIntroV1")))
    {
        const TArray<ABreakoutBlock*> Blocks = SortedBlocks(World);
        if (Blocks.Num() != 10) return; // Do not replace an unfamiliar custom layout.
        Manager->Modify();
        Manager->Tags.Add(TEXT("Lesson04EasyIntroV1"));
        for (int32 I = 0; I < Blocks.Num(); ++I)
        {
            ABreakoutBlock* Block = Blocks[I];
            Block->Modify();
            if (I >= 5)
            {
                World->EditorDestroyActor(Block, true);
                continue;
            }
            Block->SetActorLocation(FVector(0, (I - 2) * 600, 3000));
            Block->Hp = 1;
            Block->RerunConstructionScripts();
        }
        World->MarkPackageDirty();
        FEditorFileUtils::SaveLevel(World->PersistentLevel);
    }
}

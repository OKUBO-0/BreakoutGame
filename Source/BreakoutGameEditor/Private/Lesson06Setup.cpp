#include "BreakoutLesson02.h"
#include "BreakoutLesson06.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
void SaveItemBlueprint(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) { UE_LOG(LogTemp,Error,TEXT("Lesson06 compile failed: %s"),*BP->GetPathName()); return; }
    BP->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(BP->GetOutermost(),BP,*FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}

void ApplyMeshProfile(const TCHAR* Path, FName Profile)
{
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr,Path);
    if (!BP || !BP->SimpleConstructionScript) return;
    bool bChanged = false;
    for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
        if (UPrimitiveComponent* Mesh = Cast<UPrimitiveComponent>(Node->ComponentTemplate))
        {
            if (Mesh->GetCollisionProfileName() != Profile || !Mesh->GetGenerateOverlapEvents())
            {
                Mesh->Modify();
                Mesh->SetCollisionProfileName(Profile);
                Mesh->SetGenerateOverlapEvents(true);
                bChanged = true;
            }
        }
    if (bChanged) SaveItemBlueprint(BP);
}
}

void SetupLesson06()
{
    const TCHAR* Path = TEXT("/Game/Breakout/AddBallItem");
    if (!LoadObject<UBlueprint>(nullptr,Path))
    {
        UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(ABreakoutAddBallItem::StaticClass(),CreatePackage(Path),
            TEXT("AddBallItem"),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
        FAssetRegistryModule::AssetCreated(BP);
        SaveItemBlueprint(BP);
    }
    ApplyMeshProfile(TEXT("/Game/Breakout/BP_Ball"),TEXT("Ball"));
    ApplyMeshProfile(TEXT("/Game/Breakout/BP_Paddle"),TEXT("Pawn"));

    UWorld* Original = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    // Never switch away from a map containing unsaved user edits during startup.
    if (!Original || Original->GetOutermost()->IsDirty() || !ABreakoutGameManager::Find(Original)) return;
    const FString OriginalMap = Original->GetOutermost()->GetName();
    for (const TCHAR* Map : {TEXT("/Game/L_InGame"),TEXT("/Game/Level2")})
    {
        UWorld* World = GEditor->GetEditorWorldContext().World();
        if (World->GetOutermost()->GetName() != Map)
            World = UEditorLoadingAndSavingUtils::LoadMap(FPackageName::LongPackageNameToFilename(Map,FPackageName::GetMapPackageExtension()));
        if (!World) continue;
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        if (!Manager || Manager->ActorHasTag(TEXT("Lesson06CollisionConfigured"))) continue;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TInlineComponentArray<UPrimitiveComponent*> Components(*It);
            for (UPrimitiveComponent* Component : Components)
            {
                if (Component->ComponentHasTag(TEXT("MissArea")) || It->GetActorLabel() == TEXT("MissArea"))
                {
                    Component->Modify();
                    Component->SetCollisionProfileName(TEXT("MissArea"));
                    Component->SetGenerateOverlapEvents(true);
                    Component->ComponentTags.AddUnique(TEXT("MissArea"));
                }
                else if (It->IsA<AStaticMeshActor>() && It->GetActorLabel().Contains(TEXT("Wall")))
                {
                    Component->Modify();
                    Component->SetCollisionProfileName(TEXT("BlockAllDynamic"));
                }
            }
        }
        Manager->Modify();
        Manager->Tags.Add(TEXT("Lesson06CollisionConfigured"));
        World->MarkPackageDirty();
        FEditorFileUtils::SaveLevel(World->PersistentLevel);
    }
    UWorld* Current = GEditor->GetEditorWorldContext().World();
    if (Current && Current->GetOutermost()->GetName() != OriginalMap)
        UEditorLoadingAndSavingUtils::LoadMap(FPackageName::LongPackageNameToFilename(OriginalMap,FPackageName::GetMapPackageExtension()));
}

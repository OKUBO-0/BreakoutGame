#include "Editor.h"
#include "FileHelpers.h"
#include "Engine/LevelScriptBlueprint.h"
#include "GameFramework/GameUserSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Knot.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"

namespace
{
UK2Node_CallFunction* SettingsCall(UEdGraph* Graph,FName Function,int32 X)
{
    UK2Node_CallFunction* Node=NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(UGameUserSettings::StaticClass()->FindFunctionByName(Function));
    Graph->AddNode(Node,false,false); Node->CreateNewGuid(); Node->AllocateDefaultPins(); Node->NodePosX=X;
    return Node;
}
}

void SetupLesson08()
{
    UWorld* Original=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if (!Original || Original->GetOutermost()->IsDirty() || !FPackageName::DoesPackageExist(TEXT("/Game/TitleLevel"))) return;
    const FString OriginalMap=Original->GetOutermost()->GetName();
    UWorld* World=Original;
    if (OriginalMap!=TEXT("/Game/TitleLevel")) World=UEditorLoadingAndSavingUtils::LoadMap(FPackageName::LongPackageNameToFilename(TEXT("/Game/TitleLevel"),FPackageName::GetMapPackageExtension()));
    if (!World) return;
    ULevelScriptBlueprint* BP=World->PersistentLevel->GetLevelScriptBlueprint();
    UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP);
    UK2Node_CallFunction* Add=nullptr;
    bool bConfigured=false;
    for (UEdGraphNode* Node:Graph->Nodes)
        if (UK2Node_CallFunction* Call=Cast<UK2Node_CallFunction>(Node))
        {
            if (Call->FunctionReference.GetMemberName()==TEXT("AddToViewport")) Add=Call;
            if (Call->FunctionReference.GetMemberName()==TEXT("ApplySettings")) bConfigured=true;
        }
    if (!bConfigured && Add && Add->FindPinChecked(TEXT("then"))->LinkedTo.IsEmpty())
    {
        const UEdGraphSchema_K2* Schema=GetDefault<UEdGraphSchema_K2>();
        UK2Node_CallFunction* Get=SettingsCall(Graph,TEXT("GetGameUserSettings"),950);
        UK2Node_CallFunction* Mode=SettingsCall(Graph,TEXT("SetFullscreenMode"),1250);
        UK2Node_CallFunction* Resolution=SettingsCall(Graph,TEXT("SetScreenResolution"),1600);
        UK2Node_CallFunction* Apply=SettingsCall(Graph,TEXT("ApplySettings"),1950);
        Schema->TrySetDefaultValue(*Mode->FindPinChecked(TEXT("InFullscreenMode")),TEXT("Windowed"));
        UEdGraphPin* Point=Resolution->FindPinChecked(TEXT("Resolution"));
        Schema->SplitPin(Point,false);
        Schema->TrySetDefaultValue(*Resolution->FindPinChecked(TEXT("Resolution_X")),TEXT("1280"));
        Schema->TrySetDefaultValue(*Resolution->FindPinChecked(TEXT("Resolution_Y")),TEXT("720"));
        Schema->TrySetDefaultValue(*Apply->FindPinChecked(TEXT("bCheckForCommandLineOverrides")),TEXT("true"));
        // The engine exposes GetGameUserSettings as an impure node in this version.
        Schema->TryCreateConnection(Add->FindPinChecked(TEXT("then")),Get->FindPinChecked(TEXT("execute")));
        Schema->TryCreateConnection(Get->FindPinChecked(TEXT("then")),Mode->FindPinChecked(TEXT("execute")));
        Schema->TryCreateConnection(Mode->FindPinChecked(TEXT("then")),Resolution->FindPinChecked(TEXT("execute")));
        Schema->TryCreateConnection(Resolution->FindPinChecked(TEXT("then")),Apply->FindPinChecked(TEXT("execute")));
        UK2Node_Knot* Reroute=NewObject<UK2Node_Knot>(Graph); Graph->AddNode(Reroute,false,false); Reroute->CreateNewGuid(); Reroute->AllocateDefaultPins(); Reroute->NodePosX=1200; Reroute->NodePosY=280;
        Schema->TryCreateConnection(Get->FindPinChecked(TEXT("ReturnValue")),Reroute->GetInputPin());
        for (UK2Node_CallFunction* Call:{Mode,Resolution,Apply}) Schema->TryCreateConnection(Reroute->GetOutputPin(),Call->FindPinChecked(TEXT("self")));
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP);
        if (BP->Status!=BS_Error) UEditorLoadingAndSavingUtils::SaveMap(World,TEXT("/Game/TitleLevel"));
        else UE_LOG(LogTemp,Error,TEXT("Lesson08 title settings Blueprint failed to compile"));
    }
    if (OriginalMap!=TEXT("/Game/TitleLevel")) UEditorLoadingAndSavingUtils::LoadMap(FPackageName::LongPackageNameToFilename(OriginalMap,FPackageName::GetMapPackageExtension()));
}

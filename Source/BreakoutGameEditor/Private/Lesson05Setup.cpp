#include "BreakoutLesson05.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/PrimitiveComponent.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
void SaveAimingAsset(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) { UE_LOG(LogTemp, Error, TEXT("Lesson05 blueprint compilation failed: %s"), *BP->GetPathName()); return; }
    BP->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(BP->GetOutermost(), BP,
        *FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
}

void SetupLesson05()
{
    UBlueprint* Paddle = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Breakout/BP_Paddle"));
    UBlueprint* Ball = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Breakout/BP_Ball"));
    if (!Paddle || !Ball || !Paddle->SimpleConstructionScript) return;
    bool bPaddleChanged = false;
    if (!Paddle->SimpleConstructionScript->FindSCSNode(TEXT("PaddleAim")))
    {
        USCS_Node* Node = Paddle->SimpleConstructionScript->CreateNode(UBreakoutPaddleAimComponent::StaticClass(), TEXT("PaddleAim"));
        Paddle->SimpleConstructionScript->AddNode(Node);
        bPaddleChanged = true;
    }
    // This project's earlier lesson renamed the slide's Cube to PaddleMesh.
    USCS_Node* CubeNode = Paddle->SimpleConstructionScript->FindSCSNode(TEXT("PaddleMesh"));
    if (!CubeNode) CubeNode = Paddle->SimpleConstructionScript->FindSCSNode(TEXT("Cube"));
    if (CubeNode)
        if (UPrimitiveComponent* Cube = Cast<UPrimitiveComponent>(CubeNode->ComponentTemplate))
            if (!Cube->ComponentHasTag(TEXT("Player")))
            {
                Cube->ComponentTags.Add(TEXT("Player"));
                bPaddleChanged = true;
            }
    if (bPaddleChanged) SaveAimingAsset(Paddle);

    for (UEdGraph* Graph : Ball->UbergraphPages)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
            if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
                if (Call->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UBreakoutAimingLibrary, ReflectDirection)) return;

        UK2Node_Event* HitEvent = nullptr;
        UK2Node_VariableGet* Direction = nullptr;
        UK2Node_VariableSet* SetDirection = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
                if (Event->EventReference.GetMemberName() == TEXT("ReceiveHit")) HitEvent = Event;
            if (UK2Node_VariableGet* Get = Cast<UK2Node_VariableGet>(Node))
                if (Get->VariableReference.GetMemberName() == TEXT("Direction")) Direction = Get;
            if (UK2Node_VariableSet* Set = Cast<UK2Node_VariableSet>(Node))
                if (Set->VariableReference.GetMemberName() == TEXT("Direction")) SetDirection = Set;
        }
        if (!HitEvent || !Direction || !SetDirection) continue;
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
        UK2Node_CallFunction* Reflect = NewObject<UK2Node_CallFunction>(Graph);
        Reflect->SetFromFunction(UBreakoutAimingLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBreakoutAimingLibrary, ReflectDirection)));
        Graph->AddNode(Reflect, false, false);
        Reflect->CreateNewGuid();
        Reflect->AllocateDefaultPins();
        Reflect->NodePosX = 650; Reflect->NodePosY = 700;
        Reflect->NodeComment = TEXT("Lesson05: top-surface aiming + minimum horizontal angle; other surfaces use the original reflection.");
        UK2Node_Self* Self = NewObject<UK2Node_Self>(Graph);
        Graph->AddNode(Self, false, false); Self->CreateNewGuid(); Self->AllocateDefaultPins();
        Self->NodePosX = 350; Self->NodePosY = 850;
        auto Link = [Schema](UEdGraphPin* From, UEdGraphPin* To)
        {
            return From && To && Schema->TryCreateConnection(From, To);
        };
        bool bLinked = Link(Self->FindPin(TEXT("self")), Reflect->FindPin(TEXT("Ball")));
        bLinked &= Link(Direction->FindPin(TEXT("Direction")), Reflect->FindPin(TEXT("InDirection")));
        for (const auto& Names : { TPair<FName,FName>(TEXT("Other"), TEXT("OtherActor")),
            TPair<FName,FName>(TEXT("OtherComp"), TEXT("OtherComp")),
            TPair<FName,FName>(TEXT("HitLocation"), TEXT("HitLocation")),
            TPair<FName,FName>(TEXT("HitNormal"), TEXT("Normal")) })
            bLinked &= Link(HitEvent->FindPin(Names.Key), Reflect->FindPin(Names.Value));
        if (!bLinked)
        {
            FBlueprintEditorUtils::RemoveNode(Ball, Reflect, true);
            FBlueprintEditorUtils::RemoveNode(Ball, Self, true);
            UE_LOG(LogTemp, Error, TEXT("Lesson05 failed to connect the aiming inputs; existing reflection was preserved."));
            return;
        }
        UEdGraphPin* Value = SetDirection->FindPin(TEXT("Direction"));
        const TArray<UEdGraphPin*> Previous = Value->LinkedTo;
        Schema->BreakPinLinks(*Value, true);
        if (!Link(Reflect->GetReturnValuePin(), Value))
        {
            for (UEdGraphPin* Pin : Previous) Link(Pin, Value);
            FBlueprintEditorUtils::RemoveNode(Ball, Reflect, true);
            FBlueprintEditorUtils::RemoveNode(Ball, Self, true);
            UE_LOG(LogTemp, Error, TEXT("Lesson05 failed to connect Direction; restored the previous connection."));
            return;
        }
        // Remove only the replaced, now-unused mirror node; leave movement and SE alone.
        for (UEdGraphPin* Pin : Previous)
            if (UK2Node_CallFunction* Old = Cast<UK2Node_CallFunction>(Pin->GetOwningNode()))
                if (Old->FunctionReference.GetMemberName() == TEXT("MirrorVectorByNormal") && Pin->LinkedTo.IsEmpty())
                    FBlueprintEditorUtils::RemoveNode(Ball, Old, true);
        SaveAimingAsset(Ball);
        return;
    }
}

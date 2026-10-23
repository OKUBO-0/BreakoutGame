#include "BreakoutLesson07.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Camera/CameraActor.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "K2Node_Event.h"
#include "K2Node_InputKey.h"
#include "K2Node_CallFunction.h"
#include "Nodes/K2Node_CreateWidget.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "InputCoreTypes.h"

namespace
{
void SaveTitleAsset(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    UPackage::SavePackage(Asset->GetOutermost(),Asset,*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
template<class T> T* AddTitleNode(UEdGraph* Graph,int32 X,int32 Y)
{
    T* Node=NewObject<T>(Graph); Graph->AddNode(Node,false,false); Node->CreateNewGuid(); Node->NodePosX=X; Node->NodePosY=Y; return Node;
}
}

void SetupLesson07()
{
    UWidgetBlueprint* Widget=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/Breakout/UI/TitleWidget"));
    if (!Widget)
    {
        UWidgetBlueprintFactory* Factory=NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass=UUserWidget::StaticClass();
        Widget=CastChecked<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),CreatePackage(TEXT("/Game/Breakout/UI/TitleWidget")),TEXT("TitleWidget"),RF_Public|RF_Standalone,nullptr,GWarn));
        UCanvasPanel* Root=Widget->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Root")); Widget->WidgetTree->RootWidget=Root;
        UScaleBox* Scale=Widget->WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit);
        UCanvasPanelSlot* Full=Root->AddChildToCanvas(Scale); Full->SetAnchors(FAnchors(0,0,1,1)); Full->SetOffsets(FMargin(0));
        USizeBox* Design=Widget->WidgetTree->ConstructWidget<USizeBox>(); Design->SetWidthOverride(1920); Design->SetHeightOverride(1080); Scale->AddChild(Design);
        UCanvasPanel* Canvas=Widget->WidgetTree->ConstructWidget<UCanvasPanel>(); Design->AddChild(Canvas);
        for (int32 I=0;I<2;++I)
        {
            UTextBlock* Text=Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),I==0?TEXT("TitleText"):TEXT("PushSpaceText"));
            Text->SetText(FText::FromString(I==0?TEXT("ブロック崩し"):TEXT("[PUSH SPACE]")));
            FSlateFontInfo Font=Text->GetFont(); Font.Size=I==0?160:64; Font.OutlineSettings.OutlineSize=I==0?2:1;
            Font.OutlineSettings.OutlineColor=FLinearColor::Black; Text->SetFont(Font);
            Text->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Text->SetJustification(ETextJustify::Center);
            UCanvasPanelSlot* Slot=Canvas->AddChildToCanvas(Text); Slot->SetAnchors(FAnchors(.5f,I==0?.38f:.78f)); Slot->SetAlignment(FVector2D(.5f,.5f)); Slot->SetAutoSize(true); Slot->SetPosition(FVector2D::ZeroVector);
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget); FKismetEditorUtilities::CompileBlueprint(Widget);
        FAssetRegistryModule::AssetCreated(Widget); SaveTitleAsset(Widget);
    }
    if (FPackageName::DoesPackageExist(TEXT("/Game/TitleLevel"))) return;
    UWorld* Original=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if (!Original || Original->GetOutermost()->IsDirty()) return;
    const FString OriginalMap=Original->GetOutermost()->GetName();
    UWorld* World=UEditorLoadingAndSavingUtils::NewMapFromTemplate(TEXT("/Engine/Maps/Templates/Template_Default"),false);
    if (!World) return;
    World->GetWorldSettings()->DefaultGameMode=ABreakoutTitleGameMode::StaticClass();
    ACameraActor* Camera=World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),FVector(0,-600,300),FRotator(0,90,0));
    Camera->SetActorLabel(TEXT("TitleCamera"));
    ULevelScriptBlueprint* BP=World->PersistentLevel->GetLevelScriptBlueprint();
    UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP);
    const UEdGraphSchema_K2* Schema=GetDefault<UEdGraphSchema_K2>();
    UK2Node_Event* Begin=AddTitleNode<UK2Node_Event>(Graph,0,0); Begin->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"),AActor::StaticClass()); Begin->bOverrideFunction=true; Begin->AllocateDefaultPins();
    UK2Node_CreateWidget* Create=AddTitleNode<UK2Node_CreateWidget>(Graph,300,0); Create->AllocateDefaultPins();
    Schema->TrySetDefaultObject(*Create->GetClassPin(),Widget->GeneratedClass); Create->PinDefaultValueChanged(Create->GetClassPin());
    UK2Node_CallFunction* Add=AddTitleNode<UK2Node_CallFunction>(Graph,650,0); Add->SetFromFunction(UUserWidget::StaticClass()->FindFunctionByName(TEXT("AddToViewport"))); Add->AllocateDefaultPins();
    Schema->TryCreateConnection(Begin->FindPinChecked(TEXT("then")),Create->FindPinChecked(TEXT("execute")));
    Schema->TryCreateConnection(Create->FindPinChecked(TEXT("then")),Add->FindPinChecked(TEXT("execute")));
    Schema->TryCreateConnection(Create->GetResultPin(),Add->FindPinChecked(TEXT("self")));
    UK2Node_InputKey* Space=AddTitleNode<UK2Node_InputKey>(Graph,0,400); Space->InputKey=EKeys::SpaceBar; Space->bConsumeInput=true; Space->AllocateDefaultPins();
    UK2Node_CallFunction* Open=AddTitleNode<UK2Node_CallFunction>(Graph,350,400); Open->SetFromFunction(UGameplayStatics::StaticClass()->FindFunctionByName(TEXT("OpenLevel"))); Open->AllocateDefaultPins();
    Schema->TrySetDefaultValue(*Open->FindPinChecked(TEXT("LevelName")),TEXT("L_InGame"));
    Schema->TryCreateConnection(Space->FindPinChecked(TEXT("Pressed")),Open->FindPinChecked(TEXT("execute")));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status!=BS_Error) UEditorLoadingAndSavingUtils::SaveMap(World,TEXT("/Game/TitleLevel"));
    else UE_LOG(LogTemp,Error,TEXT("Lesson07 title level Blueprint failed to compile"));
    UEditorLoadingAndSavingUtils::LoadMap(FPackageName::LongPackageNameToFilename(OriginalMap,FPackageName::GetMapPackageExtension()));
}

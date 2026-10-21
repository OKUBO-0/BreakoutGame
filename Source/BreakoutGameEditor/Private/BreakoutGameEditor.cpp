#include "Modules/ModuleManager.h"
#include "BreakoutLesson02.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/MovieSceneMarginTrack.h"
#include "Animation/MovieSceneMarginSection.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneColorSection.h"
#include "MovieScene.h"
#include "MovieSceneTimeHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Containers/Ticker.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "FileHelpers.h"

namespace
{
void SaveAsset(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

void EnsureActorBlueprint(const TCHAR* Path, UClass* Parent)
{
    if (LoadObject<UBlueprint>(nullptr, Path)) return;
    UPackage* Package = CreatePackage(Path);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(Parent, Package,
        *FPackageName::GetLongPackageAssetName(Path), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    FKismetEditorUtilities::CompileBlueprint(BP);
    FAssetRegistryModule::AssetCreated(BP);
    SaveAsset(BP);
}

void EnsureWidget(const TCHAR* Path, UClass* Parent, bool bClear)
{
    if (LoadObject<UWidgetBlueprint>(nullptr, Path)) return;
    UPackage* Package = CreatePackage(Path);
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
    Factory->ParentClass = Parent;
    UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(Factory->FactoryCreateNew(
        UWidgetBlueprint::StaticClass(), Package, *FPackageName::GetLongPackageAssetName(Path), RF_Public | RF_Standalone, nullptr, GWarn));
    UCanvasPanel* Canvas = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel"));
    BP->WidgetTree->RootWidget = Canvas;
    UTextBlock* Text = BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
        bClear ? TEXT("GameClearBox") : TEXT("GameOverBox"));
    Text->SetText(FText::FromString(bClear ? TEXT("GameClear!!") : TEXT("GameOver!")));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = bClear ? 200 : 180;
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Text);
    Slot->SetAnchors(FAnchors(0.5f));
    Slot->SetAlignment(FVector2D(0.5f, 0.5f));
    Slot->SetPosition(FVector2D::ZeroVector);
    Slot->SetAutoSize(true);

    UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(BP,
        bClear ? TEXT("FloatAnimation") : TEXT("ColorAnimation"), RF_Transactional);
    Animation->MovieScene = NewObject<UMovieScene>(Animation, Animation->GetFName(), RF_Transactional);
    UMovieScene* Scene = Animation->MovieScene;
    Scene->SetTickResolutionDirectly(FFrameRate(30000, 1));
    Scene->SetDisplayRate(FFrameRate(30, 1));
    Scene->SetPlaybackRange(0, 60000);
    Animation->SetDisplayLabel(bClear ? TEXT("FloatAnimation") : TEXT("ColorAnimation"));
    const FGuid BindingID = Scene->AddPossessable(Text->GetName(), bClear ? UCanvasPanelSlot::StaticClass() : UTextBlock::StaticClass());
    FWidgetAnimationBinding Binding;
    Binding.WidgetName = Text->GetFName();
    Binding.AnimationGuid = BindingID;
    if (bClear) Binding.SlotWidgetName = Text->GetFName();
    Animation->AnimationBindings.Add(Binding);
    if (bClear)
    {
        UMovieSceneMarginTrack* Track = Scene->AddTrack<UMovieSceneMarginTrack>(BindingID);
        Track->SetPropertyNameAndPath(TEXT("Offsets"), TEXT("LayoutData.Offsets"));
        UMovieSceneMarginSection* Section = CastChecked<UMovieSceneMarginSection>(Track->CreateNewSection());
        Section->SetRange(TRange<FFrameNumber>(0, 60001));
        Section->LeftCurve.SetDefault(0);
        Section->RightCurve.SetDefault(0);
        Section->BottomCurve.SetDefault(0);
        Section->TopCurve.AddCubicKey(0, 0);
        Section->TopCurve.AddCubicKey(30000, -50);
        Section->TopCurve.AddCubicKey(60000, 0);
        Track->AddSection(*Section);
    }
    else
    {
        UMovieSceneColorTrack* Track = Scene->AddTrack<UMovieSceneColorTrack>(BindingID);
        Track->SetPropertyNameAndPath(TEXT("ColorAndOpacity"), TEXT("ColorAndOpacity"));
        UMovieSceneColorSection* Section = CastChecked<UMovieSceneColorSection>(Track->CreateNewSection());
        Section->SetRange(TRange<FFrameNumber>(0, 60001));
        Section->GetRedChannel().SetDefault(1);
        Section->GetAlphaChannel().SetDefault(1);
        Section->GetGreenChannel().AddLinearKey(0, 1);
        Section->GetGreenChannel().AddLinearKey(60000, 0);
        Section->GetBlueChannel().AddLinearKey(0, 1);
        Section->GetBlueChannel().AddLinearKey(60000, 0);
        Track->AddSection(*Section);
    }
    BP->Animations.Add(Animation);
    FKismetEditorUtilities::CompileBlueprint(BP);
    FAssetRegistryModule::AssetCreated(BP);
    SaveAsset(BP);
}

void EnsureLesson03Assets()
{
    const TCHAR* ActionPath = TEXT("/Game/Breakout/Input/IA_Action");
    UInputAction* Action = LoadObject<UInputAction>(nullptr, ActionPath);
    if (!Action)
    {
        Action = NewObject<UInputAction>(CreatePackage(ActionPath), TEXT("IA_Action"), RF_Public | RF_Standalone);
        Action->ValueType = EInputActionValueType::Boolean;
        FAssetRegistryModule::AssetCreated(Action);
        SaveAsset(Action);
    }
    const TCHAR* MappingPath = TEXT("/Game/Breakout/Input/IMC_InGame");
    if (!LoadObject<UInputMappingContext>(nullptr, MappingPath))
    {
        UInputMappingContext* Mapping = NewObject<UInputMappingContext>(CreatePackage(MappingPath), TEXT("IMC_InGame"), RF_Public | RF_Standalone);
        Mapping->MapKey(Action, EKeys::SpaceBar);
        FAssetRegistryModule::AssetCreated(Mapping);
        SaveAsset(Mapping);
    }
    const TCHAR* InfoPath = TEXT("/Game/Breakout/UI/GameInformation");
    if (!LoadObject<UWidgetBlueprint>(nullptr, InfoPath))
    {
        UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
        Factory->ParentClass = UBreakoutGameInformation::StaticClass();
        UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),
            CreatePackage(InfoPath), TEXT("GameInformation"), RF_Public | RF_Standalone, nullptr, GWarn));
        UCanvasPanel* Canvas = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel"));
        BP->WidgetTree->RootWidget = Canvas;
        UTextBlock* Text = BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LeftBallTextBox"));
        Text->bIsVariable = true;
        Text->SetText(FText::FromString(TEXT("LeftBall : 3")));
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = 70;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Text);
        Slot->SetPosition(FVector2D(50, 40));
        Slot->SetAutoSize(true);
        FKismetEditorUtilities::CompileBlueprint(BP);
        FAssetRegistryModule::AssetCreated(BP);
        SaveAsset(BP);
    }
    UWidgetBlueprint* Over = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Breakout/UI/GameOverWidget"));
    if (Over && !Over->WidgetTree->FindWidget(TEXT("PushSpaceTextBox")))
    {
        UCanvasPanel* Canvas = CastChecked<UCanvasPanel>(Over->WidgetTree->RootWidget);
        UTextBlock* Text = Over->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PushSpaceTextBox"));
        Over->OnVariableAdded(Text->GetFName());
        Text->SetText(FText::FromString(TEXT("Push SPACE to Restart")));
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = 50;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Text);
        Slot->SetAnchors(FAnchors(0.5f));
        Slot->SetAlignment(FVector2D(0.5f, 0.5f));
        Slot->SetPosition(FVector2D(0, 300));
        Slot->SetAutoSize(true);
        for (UWidgetAnimation* Animation : Over->Animations)
        {
            if (Animation->GetFName() != TEXT("ColorAnimation")) continue;
            Over->OnVariableRenamed(TEXT("ColorAnimation"), TEXT("GameOverAnimation"));
            Animation->Rename(TEXT("GameOverAnimation"), Over);
            Animation->SetDisplayLabel(TEXT("GameOverAnimation"));
            Animation->MovieScene->Rename(TEXT("GameOverAnimation"), Animation);
        }
        FKismetEditorUtilities::CompileBlueprint(Over);
        SaveAsset(Over);
    }
    UBlueprint* Paddle = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Breakout/BP_Paddle"));
    if (Paddle && !Paddle->SimpleConstructionScript->FindSCSNode(TEXT("PaddleActions")))
    {
        USCS_Node* Node = Paddle->SimpleConstructionScript->CreateNode(UBreakoutPaddleActionsComponent::StaticClass(), TEXT("PaddleActions"));
        Paddle->SimpleConstructionScript->AddNode(Node);
        FKismetEditorUtilities::CompileBlueprint(Paddle);
        SaveAsset(Paddle);
    }
}

void FitLessonWidgetsToCamera()
{
    // Preserve a 1920x1080 design surface inside the viewport. ScaleToFit keeps
    // HUD and result text inside the camera image, including letterboxed PIE.
    for (const TCHAR* Path : { TEXT("/Game/Breakout/UI/GameInformation"), TEXT("/Game/Breakout/UI/GameOverWidget"), TEXT("/Game/Breakout/UI/ClearWidget") })
    {
        UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, Path);
        if (!BP) continue;
        bool bAnimationChanged = false;
        for (UWidgetAnimation* Animation : BP->Animations)
        {
            UMovieScene* Scene = Animation->MovieScene;
            if (Scene && Scene->GetTickResolution().AsDecimal() < 30000)
            {
                // The playback manager evaluates the exclusive end minus one
                // tick. High-resolution ticks keep the 2s endpoint accurate.
                UE::MovieScene::TimeHelpers::MigrateFrameTimes(Scene->GetTickResolution(), FFrameRate(30000, 1), Scene);
                Scene->SetTickResolutionDirectly(FFrameRate(30000, 1));
                bAnimationChanged = true;
            }
        }
        if (BP->WidgetTree->FindWidget(TEXT("CameraAspectFit")))
        {
            if (bAnimationChanged) { FKismetEditorUtilities::CompileBlueprint(BP); SaveAsset(BP); }
            continue;
        }
        UWidget* Canvas = BP->WidgetTree->RootWidget;
        UScaleBox* Fit = BP->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("CameraAspectFit"));
        USizeBox* Reference = BP->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DesignResolution"));
        BP->OnVariableAdded(Fit->GetFName());
        BP->OnVariableAdded(Reference->GetFName());
        Reference->SetWidthOverride(1920);
        Reference->SetHeightOverride(1080);
        Reference->AddChild(Canvas);
        Fit->SetStretch(EStretch::ScaleToFit);
        Fit->AddChild(Reference);
        BP->WidgetTree->RootWidget = Fit;
        FKismetEditorUtilities::CompileBlueprint(BP);
        SaveAsset(BP);
    }
}

void ConfigureLesson03Level()
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || World->GetName() != TEXT("L_InGame")) return;
    ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
    if (!Manager || Manager->ActorHasTag(TEXT("Lesson03Configured"))) return;
    UClass* Ball = LoadClass<AActor>(nullptr, TEXT("/Game/Breakout/BP_Ball.BP_Ball_C"));
    TArray<AActor*> PlacedBalls;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (Ball && It->IsA(Ball)) PlacedBalls.Add(*It);
    for (AActor* Actor : PlacedBalls) World->EditorDestroyActor(Actor, true);
    AActor* Respawn = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->GetActorLabel() == TEXT("RespawnLocationActor")) { Respawn = *It; break; }
    if (!Respawn)
    {
        Respawn = World->SpawnActor<AActor>();
        USceneComponent* Root = NewObject<USceneComponent>(Respawn, TEXT("DefaultSceneRoot"), RF_Transactional);
        Respawn->SetRootComponent(Root);
        Respawn->AddInstanceComponent(Root);
        Root->RegisterComponent();
        Respawn->SetActorLabel(TEXT("RespawnLocationActor"));
    }
    Respawn->SetActorLocation(FVector(0, 0, 1000));
    Respawn->SetFolderPath(TEXT("Gameplay"));
    Manager->Modify();
    Manager->SpawnLocationActor = Respawn;
    Manager->LeftBallNum = 3;
    Manager->IsBallSpawned = false;
    Manager->Tags.Add(TEXT("Lesson03Configured"));
    World->MarkPackageDirty();
    FEditorFileUtils::SaveLevel(World->PersistentLevel);
}
}

class FBreakoutGameEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // Creates only missing lesson assets. Existing edited assets are never regenerated.
        Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
        {
            EnsureWidget(TEXT("/Game/Breakout/UI/ClearWidget"), UBreakoutClearWidget::StaticClass(), true);
            EnsureWidget(TEXT("/Game/Breakout/UI/GameOverWidget"), UBreakoutGameOverWidget::StaticClass(), false);
            EnsureActorBlueprint(TEXT("/Game/Breakout/BP_GameManager"), ABreakoutGameManager::StaticClass());
            EnsureActorBlueprint(TEXT("/Game/Breakout/BP_Block"), ABreakoutBlock::StaticClass());
            EnsureLesson03Assets();
            FitLessonWidgetsToCamera();
            ConfigureLesson03Level();
            return false;
        }), 3.0f);
    }
    virtual void ShutdownModule() override
    {
        FTSTicker::GetCoreTicker().RemoveTicker(Handle);
    }
private:
    FTSTicker::FDelegateHandle Handle;
};
IMPLEMENT_MODULE(FBreakoutGameEditorModule, BreakoutGameEditor)

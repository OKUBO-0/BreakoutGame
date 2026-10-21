#include "Modules/ModuleManager.h"
#include "BreakoutLesson02.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/MovieSceneMarginTrack.h"
#include "Animation/MovieSceneMarginSection.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneColorSection.h"
#include "MovieScene.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Containers/Ticker.h"

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
    Animation->MovieScene = NewObject<UMovieScene>(Animation, TEXT("MovieScene"), RF_Transactional);
    UMovieScene* Scene = Animation->MovieScene;
    Scene->SetTickResolutionDirectly(FFrameRate(30, 1));
    Scene->SetDisplayRate(FFrameRate(30, 1));
    Scene->SetPlaybackRange(0, 60);
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
        Section->SetRange(TRange<FFrameNumber>(0, 61));
        Section->LeftCurve.SetDefault(0);
        Section->RightCurve.SetDefault(0);
        Section->BottomCurve.SetDefault(0);
        Section->TopCurve.AddCubicKey(0, 0);
        Section->TopCurve.AddCubicKey(30, -50);
        Section->TopCurve.AddCubicKey(60, 0);
        Track->AddSection(*Section);
    }
    else
    {
        UMovieSceneColorTrack* Track = Scene->AddTrack<UMovieSceneColorTrack>(BindingID);
        Track->SetPropertyNameAndPath(TEXT("ColorAndOpacity"), TEXT("ColorAndOpacity"));
        UMovieSceneColorSection* Section = CastChecked<UMovieSceneColorSection>(Track->CreateNewSection());
        Section->SetRange(TRange<FFrameNumber>(0, 61));
        Section->GetRedChannel().SetDefault(1);
        Section->GetAlphaChannel().SetDefault(1);
        Section->GetGreenChannel().AddLinearKey(0, 1);
        Section->GetGreenChannel().AddLinearKey(60, 0);
        Section->GetBlueChannel().AddLinearKey(0, 1);
        Section->GetBlueChannel().AddLinearKey(60, 0);
        Track->AddSection(*Section);
    }
    BP->Animations.Add(Animation);
    FKismetEditorUtilities::CompileBlueprint(BP);
    FAssetRegistryModule::AssetCreated(BP);
    SaveAsset(BP);
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

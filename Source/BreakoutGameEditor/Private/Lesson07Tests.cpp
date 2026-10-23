#include "BreakoutLesson07.h"
#include "BreakoutLesson02.h"
#include "BreakoutLesson04.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "GameMapsSettings.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FLesson07PlayTest : public IAutomationLatentCommand
{
public:
    explicit FLesson07PlayTest(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        if (FPlatformTime::Seconds()-Started>30) { Test->AddError(TEXT("Title input/travel timed out")); return true; }
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        if (!World || FPlatformTime::Seconds()<Next) return false;
        APlayerController* PC=UGameplayStatics::GetPlayerController(World,0);
        if (!PC) return false;
        const auto Key=[PC](FKey K,EInputEvent Event){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Event,Event==IE_Pressed?1.0f:0.0f));};
        if (Phase==0)
        {
            Test->TestTrue(TEXT("Title has isolated game mode"),World->GetAuthGameMode()->IsA<ABreakoutTitleGameMode>());
            Test->TestNull(TEXT("No paddle on title"),PC->GetPawn().Get());
            Test->TestNull(TEXT("No gameplay manager on title"),ABreakoutGameManager::Find(World));
            UClass* WidgetClass=LoadClass<UUserWidget>(nullptr,TEXT("/Game/Breakout/UI/TitleWidget.TitleWidget_C"));
            if (!Test->TestNotNull(TEXT("Editable TitleWidget exists"),WidgetClass)) return true;
            TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,WidgetClass,true);
            if (!Test->TestEqual(TEXT("One title widget on screen"),Widgets.Num(),1)) return true;
            Title=Widgets[0];
            Test->TestTrue(TEXT("Title widget added to viewport"),Title->IsInViewport());
            UTextBlock* Text=Cast<UTextBlock>(Title->WidgetTree->FindWidget(TEXT("TitleText")));
            UTextBlock* Prompt=Cast<UTextBlock>(Title->WidgetTree->FindWidget(TEXT("PushSpaceText")));
            if (Test->TestNotNull(TEXT("Title text exists"),Text)) Test->TestEqual(TEXT("Japanese title"),Text->GetText().ToString(),FString(TEXT("ブロック崩し")));
            if (Test->TestNotNull(TEXT("Prompt exists"),Prompt)) Test->TestEqual(TEXT("Space prompt"),Prompt->GetText().ToString(),FString(TEXT("[PUSH SPACE]")));
            Test->TestEqual(TEXT("Fresh run sentinel"),CastChecked<UBreakoutGameInstance>(World->GetGameInstance())->LeftBallNum,-1);
            Key(EKeys::Enter,IE_Pressed); Key(EKeys::Enter,IE_Released); Next=FPlatformTime::Seconds()+.5; Phase=1;
        }
        else if (Phase==1)
        {
            Test->TestTrue(TEXT("Non-Space input remains on title"),World->GetAuthGameMode()->IsA<ABreakoutTitleGameMode>());
            Key(EKeys::SpaceBar,IE_Pressed); Key(EKeys::SpaceBar,IE_Released); Phase=2; Next=FPlatformTime::Seconds()+.3;
        }
        else if (Phase==2)
        {
            ABreakoutGameManager* Manager=ABreakoutGameManager::Find(World);
            if (!Manager) return false;
            Test->TestTrue(TEXT("Space opens first gameplay map"),World->GetMapName().EndsWith(TEXT("L_InGame")));
            Test->TestTrue(TEXT("Title widget removed on travel"),!Title.IsValid()||!Title->IsInViewport());
            Test->TestNotNull(TEXT("Gameplay paddle restored"),PC->GetPawn().Get());
            Test->TestEqual(TEXT("New game starts with 3 reserves"),Manager->LeftBallNum,3);
            Test->TestEqual(TEXT("Title press does not also launch a ball"),Manager->InGameBallNum,0);
            Key(EKeys::SpaceBar,IE_Pressed); Phase=3; Next=FPlatformTime::Seconds()+.1;
        }
        else
        {
            Key(EKeys::SpaceBar,IE_Released);
            ABreakoutGameManager* Manager=ABreakoutGameManager::Find(World);
            if (Test->TestNotNull(TEXT("Manager persists"),Manager))
            {
                Test->TestEqual(TEXT("Next Space launches one ball"),Manager->InGameBallNum,1);
                Test->TestEqual(TEXT("Normal launch consumes one reserve"),Manager->LeftBallNum,2);
            }
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test; double Started,Next=0; int32 Phase=0; TWeakObjectPtr<UUserWidget> Title;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson07Test,"Breakout.Lesson07.TitleAndSpaceTravel",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson07Test::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Game boots into title"),UGameMapsSettings::GetGameDefaultMap(),FString(TEXT("/Game/TitleLevel")));
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/TitleLevel")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FLesson07PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

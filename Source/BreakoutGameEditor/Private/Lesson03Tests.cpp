#include "BreakoutLesson02.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PlayInEditorDataTypes.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// UMG does not evaluate its animations in an occluded/minimized editor viewport.
// A dedicated PIE window makes the existing visual assertion exercise a drawn UI.
class FStartLesson03VisiblePIE : public IAutomationLatentCommand
{
public:
    bool Update() override
    {
        FRequestPlaySessionParams Params;
        GEditor->RequestPlaySession(Params);
        return true;
    }
};
class FLesson03PlayTest : public IAutomationLatentCommand
{
public:
    explicit FLesson03PlayTest(FAutomationTestBase* InTest) : Test(InTest) {}
    bool Update() override
    {
        if (Started == 0) Started = FPlatformTime::Seconds();
        if (FPlatformTime::Seconds() - Started > 90)
        {
            Test->AddError(TEXT("Lesson03 PIE test timed out"));
            return true;
        }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (!World) return false;
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (!Manager || !PC || !PC->GetPawn()) return false;
        const double Now = FPlatformTime::Seconds();
        if (Now < NextStep) return false;
        const auto Key = [PC](EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, Event, Event == IE_Pressed ? 1.0f : 0.0f)); };
        const auto BallCount = [World]()
        {
            int32 Count = 0;
            UClass* BallClass = LoadClass<AActor>(nullptr, TEXT("/Game/Breakout/BP_Ball.BP_Ball_C"));
            for (TActorIterator<AActor> It(World, BallClass); It; ++It) ++Count;
            return Count;
        };
        if (Phase == 0)
        {
            UBreakoutPaddleActionsComponent* Input = PC->GetPawn()->FindComponentByClass<UBreakoutPaddleActionsComponent>();
            if (!Input || Input->IsComponentTickEnabled()) return false;
            Test->TestEqual(TEXT("Start with three reserve balls"), Manager->LeftBallNum, 3);
            Test->TestEqual(TEXT("No ball placed at start"), BallCount(), 0);
            Test->TestNotNull(TEXT("HUD created at BeginPlay"), Manager->GameInformation.Get());
            Test->TestTrue(TEXT("Respawn location is (0,0,1000)"), Manager->SpawnLocationActor && Manager->SpawnLocationActor->GetActorLocation().Equals(FVector(0,0,1000)));
            PaddleStartY = PC->GetPawn()->GetActorLocation().Y;
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1.0f));
            Phase = 10;
            NextStep = Now + 0.15;
        }
        else if (Phase == 10)
        {
            Test->TestTrue(TEXT("A moves the existing Paddle left"), PC->GetPawn()->GetActorLocation().Y < PaddleStartY);
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Released, 0.0f));
            Key(IE_Pressed);
            Phase = 1;
            NextStep = Now + 0.25;
        }
        else if (Phase == 1)
        {
            Test->TestTrue(TEXT("Space Started launches a ball"), Manager->IsBallSpawned);
            Test->TestEqual(TEXT("Exactly one ball while holding Space"), BallCount(), 1);
            Test->TestEqual(TEXT("One reserve consumed per launch"), Manager->LeftBallNum, 2 - Shot);
            if (Manager->GameInformation && Manager->GameInformation->LeftBallTextBox)
                Test->TestEqual(TEXT("HUD tracks reserves"), Manager->GameInformation->LeftBallTextBox->GetText().ToString(), FString::Printf(TEXT("LeftBall : %d"), 2 - Shot));
            Manager->Action();
            Manager->Action();
            Test->TestEqual(TEXT("Repeated Action cannot spawn another ball"), BallCount(), 1);
            Test->TestEqual(TEXT("Rejected launch does not consume reserve"), Manager->LeftBallNum, 2 - Shot);
            Key(IE_Released);
            Phase = 2;
            NextStep = Now + 0.1;
        }
        else if (Phase == 2)
        {
            if (!IsValid(Manager->ActiveBall)) { Test->AddError(TEXT("Ball disappeared before miss test")); return true; }
            // Runtime-only teleport exercises the real MissArea overlap and destruction.
            Manager->ActiveBall->SetActorLocation(FVector(0,0,120), false, nullptr, ETeleportType::TeleportPhysics);
            Phase = 3;
            NextStep = Now + 0.3;
        }
        else if (Phase == 3)
        {
            Test->TestEqual(TEXT("MissArea removes the ball"), BallCount(), 0);
            Test->TestFalse(TEXT("Miss unlocks the next launch"), Manager->IsBallSpawned);
            if (Shot < 2)
            {
                Test->TestFalse(TEXT("First two misses are not GameOver"), Manager->IsGameOver);
                Key(IE_Pressed);
                ++Shot;
                Phase = 1;
                NextStep = Now + 0.25;
            }
            else
            {
                Test->TestTrue(TEXT("Third miss is GameOver"), Manager->IsGameOver);
                Test->TestNotNull(TEXT("GameOver result widget exists"), Manager->ResultWidget.Get());
                if (Manager->ResultWidget)
                {
                    UTextBlock* Prompt = Cast<UTextBlock>(Manager->ResultWidget->GetWidgetFromName(TEXT("PushSpaceTextBox")));
                    Test->TestNotNull(TEXT("Restart prompt exists"), Prompt);
                    if (Prompt) Test->TestEqual(TEXT("Restart prompt text"), Prompt->GetText().ToString(), FString(TEXT("Push SPACE to Restart")));
                    if (Prompt) Test->TestEqual(TEXT("Restart prompt font size"), Prompt->GetFont().Size, 50.0f);
                    UBreakoutGameOverWidget* Over = Cast<UBreakoutGameOverWidget>(Manager->ResultWidget);
                    Test->TestTrue(TEXT("GameOverAnimation is bound"), Over && Over->GameOverAnimation);
                }
                OldManager = Manager;
                Phase = 4;
                NextStep = Now + 2.3;
                AnimationReadyAt = World->GetTimeSeconds() + 2.3;
            }
        }
        else if (Phase == 4)
        {
            if (World->GetTimeSeconds() < AnimationReadyAt) return false;
            if (Manager->ResultWidget)
            {
                UTextBlock* Label = Cast<UTextBlock>(Manager->ResultWidget->GetWidgetFromName(TEXT("GameOverBox")));
                Test->TestNotNull(TEXT("GameOver label exists"), Label);
                if (Label)
                {
                    const FLinearColor Color = Label->GetColorAndOpacity().GetSpecifiedColor();
                    Test->AddInfo(FString::Printf(TEXT("GameOver final color: %s"), *Color.ToString()));
                    Test->TestTrue(TEXT("GameOver text becomes red after two seconds"), Color.Equals(FLinearColor::Red, 0.001f));
                }
            }
            Phase = 6;
            NextStep = Now + 0.15;
        }
        else if (Phase == 6)
        {
            Key(IE_Pressed);
            Phase = 5;
            NextStep = Now + 1.0;
        }
        else if (Phase == 5)
        {
            if (Manager == OldManager.Get()) return false;
            Key(IE_Released);
            Test->TestEqual(TEXT("Reset restores all three reserves"), Manager->LeftBallNum, 3);
            Test->TestEqual(TEXT("Reset restores five intro blocks"), Manager->GetLeftBlockNum(), 5);
            Test->TestEqual(TEXT("Reset waits for the next Space press"), BallCount(), 0);
            Test->TestFalse(TEXT("Reset clears GameOver"), Manager->IsGameOver);
            Test->TestNull(TEXT("Reset removes result UI"), Manager->ResultWidget.Get());
            // Clear must prevent both GameOver and further launches.
            Manager->IsCleared = true;
            Manager->LeftBallNum = 0;
            Manager->MissCount();
            Manager->SpawnBall();
            Test->TestFalse(TEXT("Clear takes priority over final miss"), Manager->IsGameOver);
            Test->TestEqual(TEXT("Clear prevents new balls"), BallCount(), 0);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    int32 Phase = 0, Shot = 0;
    double Started = 0, NextStep = 0;
    double PaddleStartY = 0;
    double AnimationReadyAt = 0;
    TWeakObjectPtr<ABreakoutGameManager> OldManager;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson03Test, "Breakout.Lesson03.InputLivesAndRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson03Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/L_InGame")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartLesson03VisiblePIE());
    ADD_LATENT_AUTOMATION_COMMAND(FLesson03PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

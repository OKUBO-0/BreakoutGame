#include "BreakoutLesson02.h"
#include "BreakoutLesson04.h"
#include "BreakoutPresentation.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FEvaluationLoopTest : public IAutomationLatentCommand
{
public:
    explicit FEvaluationLoopTest(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 90) { Test->AddError(TEXT("Two-cycle test timed out")); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (!World || Now < Next) return false;
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (!PC) return false;
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        UBreakoutGameInstance* GI = CastChecked<UBreakoutGameInstance>(World->GetGameInstance());
        const auto Space = [PC]() {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Released, 0));
        };
        const auto Blocks = [World]() {
            TArray<ABreakoutBlock*> Result;
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It) { It->ItemDropChance = 0; Result.Add(*It); }
            return Result;
        };
        if (Phase == 0 || Phase == 5)
        {
            if (!World->GetMapName().EndsWith(TEXT("TitleLevel"))) return false;
            Test->TestNull(TEXT("Title has no gameplay manager"), Manager);
            Test->TestEqual(TEXT("Title resets run score"), GI->RunScore, 0);
            Test->TestEqual(TEXT("Title resets carry-over sentinel"), GI->LeftBallNum, -1);
            if (Phase == 5)
            {
                Test->TestTrue(TEXT("Session best survives a completed run"), GI->SessionBestScore > 0);
                TArray<UUserWidget*> Overlays;
                UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Overlays, UBreakoutScoreOverlay::StaticClass(), true);
                Test->TestEqual(TEXT("Gameplay overlays removed on title travel"), Overlays.Num(), 0);
                if (++Cycles == 2) return true;
            }
            Space(); Phase = 1; Next = Now + 0.8; return false;
        }
        if (!Manager) return false;
        if (Phase == 1)
        {
            if (!World->GetMapName().EndsWith(TEXT("L_InGame"))) return false;
            Test->TestEqual(TEXT("Each new run starts with three reserves"), Manager->LeftBallNum, 3);
            Test->TestEqual(TEXT("Each new run starts without balls"), Manager->InGameBallNum, 0);
            Test->TestEqual(TEXT("New run starts at score zero"), Manager->GetScore(), 0);
            TArray<UUserWidget*> Overlays;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Overlays, UBreakoutScoreOverlay::StaticClass(), true);
            Test->TestEqual(TEXT("Exactly one score overlay"), Overlays.Num(), 1);
            const TArray<ABreakoutBlock*> Targets = Blocks();
            if (!Test->TestEqual(TEXT("Easy intro remains five blocks"), Targets.Num(), 5)) return true;
            Targets[0]->BreakBlock(); Targets[0]->BreakBlock();
            Test->TestEqual(TEXT("Duplicate break awards only one score"), Manager->GetScore(), 100);
            Targets[1]->BreakBlock();
            Test->TestEqual(TEXT("Second quick break earns x2"), Manager->GetScore(), 300);
            Test->TestEqual(TEXT("Combo is x2"), Manager->GetComboMultiplier(), 2);
            WorldDeadline = World->GetTimeSeconds() + 4.2f;
            Phase = 2; Next = Now + 4.2;
        }
        else if (Phase == 2)
        {
            if (World->GetTimeSeconds() < WorldDeadline) return false;
            Test->TestEqual(TEXT("Combo expires after four seconds"), Manager->GetComboMultiplier(), 1);
            TArray<ABreakoutBlock*> Targets = Blocks();
            Targets[0]->BreakBlock();
            Test->TestEqual(TEXT("Expired combo restarts at 100"), Manager->GetScore(), 400);
            AActor* Ball = Manager->GenerateBall();
            if (!Test->TestNotNull(TEXT("Bonus ball spawns for miss test"), Ball)) return true;
            Ball->Destroy();
            Test->TestEqual(TEXT("A miss resets combo"), Manager->ComboCount, 0);
            for (int32 Index = 1; Index < Targets.Num(); ++Index) Targets[Index]->BreakBlock();
            Test->TestTrue(TEXT("Intro clears normally"), Manager->IsCleared);
            StageScore = Manager->GetScore();
            int32 Count = 0;
            for (TActorIterator<ABreakoutBlockBurst> It(World); It; ++It)
            {
                ++Count;
                Test->TestFalse(TEXT("Shards cannot collide with balls or paddle"), It->GetActorEnableCollision());
            }
            Test->TestTrue(TEXT("Destruction creates burst feedback"), Count > 0);
            WorldDeadline = World->GetTimeSeconds() + 0.7f;
            Phase = 3; Next = Now + 0.7;
        }
        else if (Phase == 3)
        {
            if (World->GetTimeSeconds() < WorldDeadline) return false;
            int32 Count = 0;
            for (TActorIterator<ABreakoutBlockBurst> It(World); It; ++It) ++Count;
            Test->TestEqual(TEXT("Burst actors expire without accumulating"), Count, 0);
            Space(); Phase = 4; Next = Now + 0.8;
        }
        else if (Phase == 4)
        {
            if (!World->GetMapName().EndsWith(TEXT("Level2"))) return false;
            Test->TestEqual(TEXT("Score carries to Level2"), Manager->GetScore(), StageScore);
            Test->TestEqual(TEXT("Combo resets between stages"), Manager->GetComboMultiplier(), 1);
            Blocks()[0]->BreakBlock();
            Manager->SpawnBall();
            if (!Test->TestNotNull(TEXT("Ball exists before GameOver"), Manager->ActiveBall.Get())) return true;
            Manager->LeftBallNum = 0; Manager->ActiveBall->Destroy();
            Test->TestTrue(TEXT("All balls exhausted triggers GameOver"), Manager->IsGameOver);
            OldManager = Manager;
            Space(); Phase = 6; Next = Now + 0.8;
        }
        else if (Phase == 6)
        {
            if (Manager == OldManager.Get()) return false;
            Test->TestTrue(TEXT("Retry stays on Level2"), World->GetMapName().EndsWith(TEXT("Level2")));
            Test->TestEqual(TEXT("Retry restores stage-entry score, preventing score farming"), Manager->GetScore(), StageScore);
            Test->TestEqual(TEXT("Retry restores three reserves"), Manager->LeftBallNum, 3);
            for (ABreakoutBlock* Block : Blocks()) Block->BreakBlock();
            Test->TestTrue(TEXT("Final stage clears"), Manager->IsCleared);
            Test->TestEqual(TEXT("Combo multiplier caps at five"), Manager->GetComboMultiplier(), 5);
            Test->TestTrue(TEXT("Final clear explains return to title"), Manager->GetActionHint().ToString().Contains(TEXT("BACK TO TITLE")));
            Space(); Phase = 5; Next = Now + 0.8;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Phase = 0, Cycles = 0, StageScore = 0;
    float WorldDeadline = 0;
    TWeakObjectPtr<ABreakoutGameManager> OldManager;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutEvaluation01Test, "Breakout.Evaluation01.TwoCyclesScoreAndFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutEvaluation01Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/TitleLevel")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FEvaluationLoopTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

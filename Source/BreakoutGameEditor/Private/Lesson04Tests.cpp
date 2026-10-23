#include "BreakoutLesson02.h"
#include "BreakoutLesson04.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/TextBlock.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "AudioDevice.h"
#include "AudioThread.h"
#include "ActiveSound.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FLesson04PlayTest : public IAutomationLatentCommand
{
public:
    explicit FLesson04PlayTest(FAutomationTestBase* InTest) : Test(InTest), HeardKnock(MakeShared<TAtomic<bool>, ESPMode::ThreadSafe>(false)) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (!Started) Started = Now;
        if (Now - Started > 120) { Test->AddError(TEXT("Lesson04 PIE test timed out")); return true; }
        if (Now < Next) return false;
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (!World) return false;
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(World->GetGameInstance());
        if (!Manager || !Instance) return false;
        if (Phase == 0)
        {
            Test->TestFalse(TEXT("GameInstance -1 is unset"), Instance->IsValidBallNum());
            Instance->LeftBallNum = 0;
            Test->TestTrue(TEXT("GameInstance zero is valid"), Instance->IsValidBallNum());
            Instance->LeftBallNum = -1;
            Test->TestEqual(TEXT("Initial reserve remains three"), Manager->LeftBallNum, 3);
            Test->TestEqual(TEXT("Configured target is Level2"), Manager->NextLevelName, FName(TEXT("Level2")));
            Test->TestEqual(TEXT("Intro has five single-hit blocks"), Manager->BlockNum, 5);
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It)
            {
                Test->TestEqual(TEXT("Intro block needs only one hit"), It->Hp, 1);
                Test->TestEqual(TEXT("Intro row is closer to the paddle"), It->GetActorLocation().Z, 3000.0);
                if (!Target.IsValid()) Target = *It;
                Test->TestNotNull(TEXT("Per-block MID exists"), It->MaterialInstance.Get());
                Test->TestTrue(TEXT("Text faces camera at X=-51"), It->TextRender->GetRelativeLocation().Equals(FVector(-51,0,0)));
                Test->TestEqual(TEXT("Text size is 150"), It->TextRender->WorldSize, 150.0f);
            }
            if (!Test->TestNotNull(TEXT("A block exists for the HP-five runtime fixture"), Target.Get())) return true;
            // Exercise the full durability system without making Level1 harder.
            Target->Hp = 5;
            Target->ReloadHp();
            Test->TestEqual(TEXT("HP text is initialized"), Target->TextRender->Text.ToString(), FString(TEXT("5")));
            const FLinearColor Before = Target->MaterialInstance->K2_GetVectorParameterValue(TEXT("BaseColor"));
            Target->Hp = 100;
            Target->ReloadHp();
            Test->TestTrue(TEXT("Invalid color index leaves the previous material unchanged"), Before.Equals(Target->MaterialInstance->K2_GetVectorParameterValue(TEXT("BaseColor"))));
            Target->Hp = 5;
            Target->ReloadHp();
            UMaterialInstanceDynamic* Unique = Target->MaterialInstance;
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It)
                if (*It != Target.Get()) Test->TestTrue(TEXT("Materials are independent per block"), It->MaterialInstance != Unique);
            USoundCue* Cue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Breakout/Audio/Breakout_BGM_Cue"));
            USoundNodeLooping* Loop = Cue ? Cast<USoundNodeLooping>(Cue->FirstNode) : nullptr;
            Test->TestTrue(TEXT("BGM cue contains an infinite loop node"), Loop && Loop->bLoopIndefinitely && Loop->ChildNodes.Num() == 1);
            USoundNodeWavePlayer* Player = Loop && Loop->ChildNodes.Num() ? Cast<USoundNodeWavePlayer>(Loop->ChildNodes[0]) : nullptr;
            Test->TestNotNull(TEXT("Loop connects to a WavePlayer"), Player);
            CheckBgm(World);
            OriginalInstance = Instance;
            Phase = 1;
            Next = Now + 17.0; // Pass the complete 16-second source WAV duration.
        }
        else if (Phase == 1)
        {
            CheckBgm(World);
            Manager->SpawnBall();
            if (!Test->TestNotNull(TEXT("Test ball spawned"), Manager->ActiveBall.Get())) return true;
            AActor* Ball = Manager->ActiveBall;
            UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Ball->GetRootComponent());
            FHitResult Hit;
            Hit.Normal = Hit.ImpactNormal = FVector(0,0,-1);
            Target->Cube->OnComponentHit.Broadcast(Target->Cube, Manager, nullptr, FVector::ZeroVector, Hit);
            Test->TestEqual(TEXT("Non-ball collision does not damage the block"), Target->Hp, 5);
            Target->Cube->OnComponentHit.Broadcast(Target->Cube, Ball, Root, FVector::ZeroVector, Hit);
            Target->Cube->OnComponentHit.Broadcast(Target->Cube, Ball, Root, FVector::ZeroVector, Hit);
            Test->TestEqual(TEXT("Duplicate collision callback costs only one HP"), Target->Hp, 4);
            Test->TestEqual(TEXT("HP text refreshes on hit"), Target->TextRender->Text.ToString(), FString(TEXT("4")));
            Test->TestTrue(TEXT("HP four uses the fourth color"), Target->MaterialInstance->K2_GetVectorParameterValue(TEXT("BaseColor")).Equals(Target->ColorTable[3]));
            Test->TestEqual(TEXT("Damaged but surviving block is not counted as broken"), Manager->BrokenBlockNum, 0);
            Root->OnComponentHit.Broadcast(Root, Target.Get(), Target->Cube, FVector::ZeroVector, Hit);
            Knock = Ball->FindComponentByClass<UBreakoutBallRulesComponent>()->KnockSound.LoadSynchronous();
            Test->TestNotNull(TEXT("Knock WAV is assigned"), Knock);
            Phase = 2;
            AudioDeadline = Now + 1.0;
        }
        else if (Phase == 2)
        {
            const FAudioDeviceHandle Device = World->GetAudioDevice();
            const auto Heard = HeardKnock;
            USoundBase* Expected = Knock;
            if (Device.IsValid()) FAudioThread::RunCommandOnAudioThread([Device, Heard, Expected]()
            {
                for (const FActiveSound* Sound : Device->GetActiveSounds())
                    if (Sound && Sound->GetSound() == Expected) Heard->Store(true);
            });
            if (!HeardKnock->Load() && Now < AudioDeadline) return false;
            Test->TestTrue(TEXT("Ball collision starts the knock sound on the audio device"), HeardKnock->Load());
            for (int32 I=0; I<4; ++I) if (Target.IsValid()) Target->HitBlock();
            Test->TestEqual(TEXT("Five total hits destroy the HP-five block once"), Manager->BrokenBlockNum, 1);
            if (IsValid(Manager->ActiveBall)) Manager->ActiveBall->Destroy();
            Manager->SpawnBall();
            Test->TestEqual(TEXT("Two launches leave one reserve"), Manager->LeftBallNum, 1);
            TArray<ABreakoutBlock*> Blocks;
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It) Blocks.Add(*It);
            for (ABreakoutBlock* Block : Blocks)
                for (int32 I=Block->Hp; I>0; --I) Block->HitBlock();
            Test->TestTrue(TEXT("Only final block destruction clears the level"), Manager->IsCleared);
            Test->TestEqual(TEXT("All five intro blocks counted exactly once"), Manager->BrokenBlockNum, 5);
            // Lesson08: explicit package paths work as well as the short-name
            // path exercised by the multiball carry-over test.
            Manager->NextLevelName = FName(TEXT("/Game/Level2"));
            Manager->Action();
            Manager->Action();
            Test->TestEqual(TEXT("Carry-over includes the in-play ball exactly once"), Instance->LeftBallNum, 2);
            Phase = 3;
            Next = Now + 0.7;
        }
        else if (Phase == 3)
        {
            if (UGameplayStatics::GetCurrentLevelName(World,true) != TEXT("Level2")) return false;
            Test->TestTrue(TEXT("GameInstance survives travel"), OriginalInstance.Get() == Instance);
            Test->TestEqual(TEXT("Level2 uses carried reserve rather than default three"), Manager->LeftBallNum, 2);
            Test->TestEqual(TEXT("Level2 has ten newly initialized blocks"), Manager->BlockNum, 10);
            Test->TestFalse(TEXT("Level2 clears the previous result state"), Manager->IsCleared);
            Test->TestNull(TEXT("Level2 has no old result widget"), Manager->ResultWidget.Get());
            Test->TestTrue(TEXT("Spawn actor reference points into Level2"), Manager->SpawnLocationActor && Manager->SpawnLocationActor->GetWorld() == World);
            Test->TestEqual(TEXT("Carried reserve appears on HUD"), Manager->GameInformation->LeftBallTextBox->GetText().ToString(), FString(TEXT("LeftBall : 2")));
            TSet<int32> Rows;
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It) Rows.Add(FMath::RoundToInt(It->GetActorLocation().Z));
            Test->TestEqual(TEXT("Level2 layout differs with four rows"), Rows.Num(), 4);
            CheckBgm(World);
            Manager->SpawnBall(); Manager->ActiveBall->Destroy();
            Manager->SpawnBall(); Manager->ActiveBall->Destroy();
            Test->TestTrue(TEXT("Exhausting carried balls triggers GameOver"), Manager->IsGameOver);
            OldManager = Manager;
            Manager->Action();
            Phase = 4;
            Next = Now + 0.7;
        }
        else if (Phase == 4)
        {
            if (Manager == OldManager.Get()) return false;
            Test->TestEqual(TEXT("GameOver restarts the current stage"), UGameplayStatics::GetCurrentLevelName(World,true), FString(TEXT("Level2")));
            Test->TestEqual(TEXT("Restart does not reload stale carry-over"), Manager->LeftBallNum, 3);
            Test->TestFalse(TEXT("Restart clears the GameInstance sentinel"), Instance->IsValidBallNum());
            CheckBgm(World);
            return true;
        }
        return false;
    }
private:
    void CheckBgm(UWorld* World)
    {
        int32 Count=0;
        for (TActorIterator<AAmbientSound> It(World); It; ++It)
            if (It->ActorHasTag(TEXT("Lesson04BGM")))
            {
                ++Count;
                Test->TestTrue(TEXT("Placed BGM component is playing"), It->GetAudioComponent()->IsPlaying());
            }
        Test->TestEqual(TEXT("Exactly one BGM actor per stage"), Count, 1);
    }
    FAutomationTestBase* Test;
    int32 Phase=0;
    double Started=0, Next=0, AudioDeadline=0;
    TWeakObjectPtr<ABreakoutBlock> Target;
    TWeakObjectPtr<ABreakoutGameManager> OldManager;
    TWeakObjectPtr<UBreakoutGameInstance> OriginalInstance;
    TSharedRef<TAtomic<bool>, ESPMode::ThreadSafe> HeardKnock;
    USoundBase* Knock=nullptr;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson04Test, "Breakout.Lesson04.HpAudioAndLevelTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson04Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/L_InGame")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FLesson04PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

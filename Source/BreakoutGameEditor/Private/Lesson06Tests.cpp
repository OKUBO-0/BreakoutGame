#include "BreakoutLesson02.h"
#include "BreakoutLesson04.h"
#include "BreakoutLesson06.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
int32 ItemCount(UWorld* World)
{
    int32 Count=0;
    for (TActorIterator<ABreakoutAddBallItem> It(World); It; ++It) ++Count;
    return Count;
}

class FLesson06PlayTest : public IAutomationLatentCommand
{
public:
    explicit FLesson06PlayTest(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (FPlatformTime::Seconds()-Started>60) { Test->AddError(TEXT("Lesson06 PIE timed out")); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (!World || FPlatformTime::Seconds()<Next) return false;
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        APawn* Paddle = UGameplayStatics::GetPlayerPawn(World,0);
        if (!Manager || !Paddle) return false;
        UPrimitiveComponent* Player = Cast<UPrimitiveComponent>(Paddle->GetRootComponent());
        if (!Test->TestNotNull(TEXT("Paddle has primitive root"),Player)) return true;
        if (Phase==0)
        {
            Test->TestEqual(TEXT("Paddle uses Pawn profile"),Player->GetCollisionProfileName(),FName(TEXT("Pawn")));
            Test->TestEqual(TEXT("Pawn overlaps Item"),Player->GetCollisionResponseToChannel(BreakoutCollision::Item),ECR_Overlap);
            Test->TestEqual(TEXT("No preplaced item fixtures remain"),ItemCount(World),0);
            Test->TestEqual(TEXT("Initially zero live balls"),Manager->InGameBallNum,0);
            Manager->SpawnBall();
            AActor* First = Manager->ActiveBall;
            AActor* Second = Manager->GenerateBall();
            if (!Test->TestNotNull(TEXT("First ball exists"),First) || !Test->TestNotNull(TEXT("Second ball can spawn at the same location"),Second)) return true;
            First->SetActorTickEnabled(false); Second->SetActorTickEnabled(false);
            Test->TestEqual(TEXT("GenerateBall adds a live ball without using a reserve"),Manager->LeftBallNum,2);
            Test->TestEqual(TEXT("Two balls are tracked independently"),Manager->InGameBallNum,2);
            Test->TestTrue(TEXT("Ball-ball overlap at spawn is allowed"),First->GetActorLocation().Equals(Second->GetActorLocation()));
            UPrimitiveComponent* BallRoot = CastChecked<UPrimitiveComponent>(First->GetRootComponent());
            Test->TestEqual(TEXT("Ball uses named profile"),BallRoot->GetCollisionProfileName(),FName(TEXT("Ball")));
            Test->TestEqual(TEXT("Ball ignores Ball"),BallRoot->GetCollisionResponseToChannel(BreakoutCollision::Ball),ECR_Ignore);
            Test->TestEqual(TEXT("Ball ignores Item"),BallRoot->GetCollisionResponseToChannel(BreakoutCollision::Item),ECR_Ignore);
            Test->TestEqual(TEXT("Ball overlaps MissArea"),BallRoot->GetCollisionResponseToChannel(BreakoutCollision::MissArea),ECR_Overlap);
            Manager->Action();
            Test->TestEqual(TEXT("Normal Space still rejects duplicate launches"),Manager->InGameBallNum,2);
            Test->TestEqual(TEXT("Rejected Space does not consume a reserve"),Manager->LeftBallNum,2);
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It)
            {
                const int32 OriginalHp=It->Hp;
                It->Hp=3;
                FHitResult Hit;
                It->Cube->OnComponentHit.Broadcast(It->Cube,First,BallRoot,FVector::ZeroVector,Hit);
                It->Cube->OnComponentHit.Broadcast(It->Cube,Second,Cast<UPrimitiveComponent>(Second->GetRootComponent()),FVector::ZeroVector,Hit);
                It->Cube->OnComponentHit.Broadcast(It->Cube,First,BallRoot,FVector::ZeroVector,Hit);
                Test->TestEqual(TEXT("Two distinct balls count twice, duplicate callback counts once"),It->Hp,1);
                It->Hp=OriginalHp; It->ReloadHp();
                break;
            }

            UClass* ItemClass = LoadClass<ABreakoutAddBallItem>(nullptr,TEXT("/Game/Breakout/AddBallItem.AddBallItem_C"));
            if (!Test->TestNotNull(TEXT("Editable AddBallItem Blueprint exists"),ItemClass)) return true;
            ABreakoutAddBallItem* Item = World->SpawnActor<ABreakoutAddBallItem>(ItemClass,First->GetActorLocation(),FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Item can overlap a ball without collecting"),Item)) return true;
            Test->TestEqual(TEXT("Item has A label"),Item->TextRender->Text.ToString(),FString(TEXT("A")));
            Test->TestEqual(TEXT("A label is readable at the existing camera distance"),Item->TextRender->WorldSize,150.0f);
            Test->TestTrue(TEXT("A faces the game camera"),Item->TextRender->GetRelativeLocation().Equals(FVector(-51,0,0)) && Item->TextRender->GetRelativeRotation().Equals(FRotator(0,180,0)));
            Test->TestEqual(TEXT("Item is query-only"),Item->Sphere->GetCollisionEnabled(),ECollisionEnabled::QueryOnly);
            Test->TestTrue(TEXT("Sphere has Item tag"),Item->Sphere->ComponentHasTag(TEXT("Item")));
            Test->TestEqual(TEXT("Item ignores world geometry"),Item->Sphere->GetCollisionResponseToChannel(ECC_WorldDynamic),ECR_Ignore);
            Test->TestEqual(TEXT("Item overlaps Pawn"),Item->Sphere->GetCollisionResponseToChannel(ECC_Pawn),ECR_Overlap);
            Item->SetActorTickEnabled(false);
            Manager->LeftBallNum=0;
            FVector Origin,Extent;
            Paddle->GetActorBounds(false,Origin,Extent);
            Item->SetActorLocation(FVector(Origin.X,Origin.Y,Origin.Z+Extent.Z+170));
            const double BeforeZ=Item->GetActorLocation().Z;
            Item->Tick(0.1f);
            Test->TestTrue(TEXT("Falling speed is multiplied by delta seconds"),FMath::IsNearlyEqual(Item->GetActorLocation().Z,BeforeZ-60,0.01));
            Item->Tick(0.2f);
            Test->TestFalse(TEXT("Real swept Pawn overlap consumes the item"),IsValid(Item));
            Test->TestEqual(TEXT("Pickup adds a third ball even with zero reserves"),Manager->InGameBallNum,3);
            Test->TestEqual(TEXT("Pickup never consumes reserves"),Manager->LeftBallNum,0);
            Manager->ActiveBall->SetActorTickEnabled(false);
            First->Destroy();
            Test->TestEqual(TEXT("Destroying a non-latest ball decrements only one"),Manager->InGameBallNum,2);
            Test->TestFalse(TEXT("Zero reserves is not GameOver while another ball exists"),Manager->IsGameOver);

            ABreakoutAddBallItem* Miss = World->SpawnActor<ABreakoutAddBallItem>(ItemClass,FVector(0,1300,500),FRotator::ZeroRotator);
            Miss->SetActorTickEnabled(false);
            Miss->Tick(2.0f);
            Test->TestFalse(TEXT("Real swept MissArea overlap removes an uncaught item"),IsValid(Miss));
            Test->TestEqual(TEXT("Missed item adds no ball"),Manager->InGameBallNum,2);
            TArray<ABreakoutBlock*> Blocks;
            for (TActorIterator<ABreakoutBlock> It(World); It; ++It) Blocks.Add(*It);
            if (!Test->TestTrue(TEXT("Intro has enough blocks for drop tests"),Blocks.Num()>=3)) return true;
            ABreakoutBlock* Target=Blocks[0];
            Test->TestEqual(TEXT("Default drop chance follows slide weight 0.5"),Target->ItemDropChance,0.5f);
            Target->Hp=2; Target->ItemDropChance=1;
            const FTransform DropTransform=Target->GetActorTransform();
            Target->HitBlock();
            Test->TestEqual(TEXT("Damage alone never drops an item"),ItemCount(World),0);
            Target->HitBlock();
            Test->TestEqual(TEXT("Guaranteed drop creates exactly one item"),ItemCount(World),1);
            for (TActorIterator<ABreakoutAddBallItem> It(World); It; ++It)
                Test->TestTrue(TEXT("Drop inherits the block actor transform"),It->GetActorTransform().Equals(DropTransform));
            Target->BreakBlock();
            Test->TestEqual(TEXT("Duplicate destruction cannot duplicate the drop"),ItemCount(World),1);
            Blocks[1]->ItemDropChance=0;
            Blocks[1]->BreakBlock();
            Test->TestEqual(TEXT("Zero chance never creates an item"),ItemCount(World),1);
            Manager->LeftBallNum=2;
            for (int32 I=2;I<Blocks.Num();++I) { Blocks[I]->ItemDropChance=0; Blocks[I]->BreakBlock(); }
            Test->TestTrue(TEXT("Level clears with two live balls"),Manager->IsCleared);
            Test->TestEqual(TEXT("Two balls remain available for carry-over"),Manager->InGameBallNum,2);
            TArray<AActor*> Balls;
            for (TActorIterator<AActor> It(World); It; ++It)
                if (It->FindComponentByClass<UBreakoutBallRulesComponent>()) Balls.Add(*It);
            for (AActor* Ball : Balls) Ball->Destroy();
            Test->TestEqual(TEXT("Post-clear falls preserve the frozen carry-over count"),Manager->InGameBallNum,2);
            Test->TestFalse(TEXT("Post-clear falls never cause GameOver"),Manager->IsGameOver);
            Test->TestNull(TEXT("Late pickups cannot inflate a completed stage"),Manager->GenerateBall());
            Manager->Action(); Manager->Action();
            Test->TestEqual(TEXT("Carry-over is reserve two plus live two exactly once"),CastChecked<UBreakoutGameInstance>(World->GetGameInstance())->LeftBallNum,4);
            Phase=1; Next=FPlatformTime::Seconds()+0.7;
        }
        else if (Phase==1)
        {
            if (UGameplayStatics::GetCurrentLevelName(World,true)!=TEXT("Level2")) return false;
            Test->TestEqual(TEXT("Next stage receives all four balls as reserves"),Manager->LeftBallNum,4);
            Test->TestEqual(TEXT("Next stage starts without live balls"),Manager->InGameBallNum,0);
            Test->TestEqual(TEXT("Carried count appears in HUD"),Manager->GameInformation->LeftBallTextBox->GetText().ToString(),FString(TEXT("LeftBall : 4")));
            Test->TestEqual(TEXT("Old dropped items do not survive travel"),ItemCount(World),0);
            AActor* First=Manager->GenerateBall();
            AActor* Second=Manager->GenerateBall();
            if (!Test->TestNotNull(TEXT("Level2 first bonus ball spawns"),First) || !Test->TestNotNull(TEXT("Level2 second bonus ball spawns"),Second)) return true;
            Manager->LeftBallNum=0;
            First->SetActorLocation(FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
            Test->TestEqual(TEXT("A real Ball-MissArea overlap removes only that ball"),Manager->InGameBallNum,1);
            Test->TestFalse(TEXT("Last surviving ball prevents GameOver"),Manager->IsGameOver);
            Second->SetActorLocation(FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
            Test->TestEqual(TEXT("All balls gone leaves zero in play"),Manager->InGameBallNum,0);
            Test->TestTrue(TEXT("GameOver requires both counts exhausted"),Manager->IsGameOver);
            OldManager=Manager;
            Manager->Action(); Phase=2; Next=FPlatformTime::Seconds()+0.7;
        }
        else
        {
            if (Manager==OldManager.Get()) return false;
            Test->TestEqual(TEXT("Restart stays on Level2"),UGameplayStatics::GetCurrentLevelName(World,true),FString(TEXT("Level2")));
            Test->TestEqual(TEXT("Restart restores three reserves"),Manager->LeftBallNum,3);
            Test->TestEqual(TEXT("Restart clears multi-ball counters"),Manager->InGameBallNum,0);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started,Next=0;
    int32 Phase=0;
    TWeakObjectPtr<ABreakoutGameManager> OldManager;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson06Test,"Breakout.Lesson06.ItemMultiballAndCarryOver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson06Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/L_InGame")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FLesson06PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

#include "BreakoutLesson02.h"
#include "BreakoutLesson05.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FLesson05PlayTest : public IAutomationLatentCommand
{
public:
    explicit FLesson05PlayTest(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 30) { Test->AddError(TEXT("Lesson05 PIE timed out")); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (!World) return false;
        APawn* Paddle = UGameplayStatics::GetPlayerPawn(World, 0);
        ABreakoutGameManager* Manager = ABreakoutGameManager::Find(World);
        if (!Paddle || !Manager) return false;
        UBreakoutPaddleAimComponent* Aim = Paddle->FindComponentByClass<UBreakoutPaddleAimComponent>();
        if (!Test->TestNotNull(TEXT("Paddle has editable aiming component"), Aim)) return true;
        UPrimitiveComponent* Cube = nullptr;
        for (UActorComponent* Component : Paddle->GetComponents())
            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
                if (Primitive->ComponentHasTag(TEXT("Player"))) Cube = Primitive;
        if (!Test->TestNotNull(TEXT("Paddle collision component has Player tag"), Cube)) return true;
        Test->TestEqual(TEXT("Slide normal tilt default is twenty degrees"), Aim->MaxTiltNormalDeg, 20.0f);
        Test->TestFalse(TEXT("Normal debug drawing is off in normal play"), Aim->bDrawTopSurfaceNormals);
        Test->TestFalse(TEXT("Disabled debug adds no component tick"), Aim->IsComponentTickEnabled());
        FVector Origin, Extent;
        Paddle->GetActorBounds(false, Origin, Extent);
        bool bTop;
        FVector Normal;
        const double Tilt = FMath::DegreesToRadians(20.0);
        for (int32 Side : {-1, 0, 1})
        {
            Aim->GetTopNormal(Origin + FVector(0, Side * Extent.Y, Extent.Z), FVector::UpVector, bTop, Normal);
            Test->TestTrue(TEXT("Top normal is recognized"), bTop);
            Test->TestTrue(TEXT("Normal tilts outward with position"), Normal.Equals(FVector(0, Side * FMath::Sin(Tilt), Side ? FMath::Cos(Tilt) : 1), 0.0001));
        }
        for (const FVector SideNormal : { FVector(0,1,0), FVector(0,-1,0), FVector(0,0,-1) })
        {
            Aim->GetTopNormal(Origin, SideNormal, bTop, Normal);
            Test->TestFalse(TEXT("Side and underside are not treated as top"), bTop);
            Test->TestTrue(TEXT("Side normal passes through unchanged"), Normal == SideNormal);
        }
        Aim->MaxTiltNormalDeg = 0;
        Aim->GetTopNormal(Origin + FVector(0,Extent.Y,Extent.Z), FVector::UpVector,bTop,Normal);
        Test->TestTrue(TEXT("Zero tilt yields a flat normal"), Normal.Equals(FVector::UpVector));
        Aim->MaxTiltNormalDeg = 60;
        Aim->GetTopNormal(Origin + FVector(0,Extent.Y,Extent.Z), FVector::UpVector,bTop,Normal);
        Test->TestTrue(TEXT("Sixty degree debug example is supported"), Normal.Equals(FRotator(0,0,60).RotateVector(FVector::UpVector),0.0001));
        Aim->MaxTiltNormalDeg = 20;
        Aim->SetDebugNormalsEnabled(true);
        Test->TestTrue(TEXT("Debug drawing can be enabled explicitly"), Aim->IsComponentTickEnabled());
        Aim->DrawDebugTopSurfaceNormals();
        Aim->SetDebugNormalsEnabled(false);
        Test->TestFalse(TEXT("Debug drawing can be disconnected again"), Aim->IsComponentTickEnabled());

        const double MinimumZ = FMath::Sin(FMath::DegreesToRadians(10.0));
        for (const FVector Input : {FVector(0,1,-1), FVector(0,-1,-1), FVector(0,1,0), FVector(0,-1,0), FVector::UpVector, FVector::ZeroVector})
        {
            const FVector Output = UBreakoutAimingLibrary::ClampDirection(Input,10);
            Test->TestTrue(TEXT("Clamp returns a finite unit vector on the YZ plane"), !Output.ContainsNaN() && FMath::IsNearlyEqual(Output.Size(),1.0,0.0001) && Output.X == 0);
            Test->TestTrue(TEXT("Clamped top bounce is at least ten degrees above horizontal"), Output.Z >= MinimumZ - 0.0001);
            if (Input.Y != 0) Test->TestTrue(TEXT("Clamp preserves left/right direction even below horizontal"), Input.Y * Output.Y > 0);
        }
        Test->TestTrue(TEXT("Ninety-degree minimum is straight up"), UBreakoutAimingLibrary::ClampDirection(FVector(0,1,-1),90).Equals(FVector::UpVector,0.0001));
        Manager->SpawnBall();
        AActor* Ball = Manager->ActiveBall;
        if (!Test->TestNotNull(TEXT("Real Blueprint ball spawns"), Ball)) return true;
        UBreakoutBallRulesComponent* Rules = Ball->FindComponentByClass<UBreakoutBallRulesComponent>();
        if (!Test->TestNotNull(TEXT("Ball retains audio and miss rules"), Rules)) return true;
        Test->TestEqual(TEXT("Slide minimum horizontal angle defaults to ten"), Rules->MinHorizontalAngleDeg,10.0f);
        const FVector Incoming(0,0,-1);
        const FVector Point = Origin + FVector(0,Extent.Y,Extent.Z);
        Test->TestTrue(TEXT("Non-player object uses original mirror reflection"), UBreakoutAimingLibrary::ReflectDirection(Ball,Manager,Cast<UPrimitiveComponent>(Ball->GetRootComponent()),Incoming,Point,FVector::UpVector).Equals(FVector::UpVector));
        Test->TestTrue(TEXT("Paddle underside preserves a downward bounce"), UBreakoutAimingLibrary::ReflectDirection(Ball,Paddle,Cube,FVector::UpVector,Point,-FVector::UpVector).Equals(-FVector::UpVector));
        const TArray<FName> Tags = Cube->ComponentTags;
        Cube->ComponentTags.Remove(TEXT("Player"));
        Test->TestTrue(TEXT("Missing Player tag does not apply aiming"), UBreakoutAimingLibrary::ReflectDirection(Ball,Paddle,Cube,Incoming,Point,FVector::UpVector).Equals(FVector::UpVector));
        Cube->ComponentTags = Tags;
        FStructProperty* Direction = FindFProperty<FStructProperty>(Ball->GetClass(),TEXT("Direction"));
        if (!Test->TestNotNull(TEXT("Original Blueprint Direction remains editable"),Direction)) return true;
        Ball->SetActorTickEnabled(false);
        FVector BallOrigin, BallExtent;
        Ball->GetActorBounds(false,BallOrigin,BallExtent);
        for (int32 Side : {-1,0,1})
        {
            *Direction->ContainerPtrToValuePtr<FVector>(Ball) = Incoming;
            Ball->SetActorLocation(FVector(Origin.X,Origin.Y + Side * Extent.Y * 0.9,Origin.Z + Extent.Z + BallExtent.Z + 20),false,nullptr,ETeleportType::TeleportPhysics);
            // Execute the existing Blueprint sweep movement, not a fabricated hit callback.
            Ball->Tick(0.1f);
            const FVector Actual = *Direction->ContainerPtrToValuePtr<FVector>(Ball);
            Test->TestTrue(TEXT("Real sweep reflects upward without entering the paddle"), Actual.Z >= MinimumZ - 0.0001);
            Test->TestTrue(TEXT("Real top hit aims left / center / right"), Side == 0 ? Actual.Equals(FVector::UpVector,0.001) : Side * Actual.Y > 0.4);
            UE_LOG(LogTemp,Display,TEXT("Lesson05 swept hit side=%d direction=%s"),Side,*Actual.ToString());
        }
        Test->TestEqual(TEXT("Aiming does not consume extra reserve balls"),Manager->LeftBallNum,2);
        return true;
    }
private:
    FAutomationTestBase* Test;
    double Started;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson05Test,"Breakout.Lesson05.AimingAndRealSweptHits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson05Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/L_InGame")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FLesson05PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakoutLesson05Level2Test,"Breakout.Lesson05.Level2AimingAndRealSweptHits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBreakoutLesson05Level2Test::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Level2")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FLesson05PlayTest(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

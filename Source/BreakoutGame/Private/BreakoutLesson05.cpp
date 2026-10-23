#include "BreakoutLesson05.h"
#include "BreakoutLesson02.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

UBreakoutPaddleAimComponent::UBreakoutPaddleAimComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBreakoutPaddleAimComponent::GetTopNormal(FVector HitLocation, FVector Normal,
    bool& IsHitTopSurface, FVector& OutNormal) const
{
    IsHitTopSurface = false;
    OutNormal = Normal;
    if (!GetOwner() || !Normal.Equals(FVector::UpVector, KINDA_SMALL_NUMBER)) return;
    FVector Origin, Extent;
    GetOwner()->GetActorBounds(false, Origin, Extent);
    if (Extent.Y <= KINDA_SMALL_NUMBER || HitLocation.ContainsNaN()) return;
    const double Position = FMath::Clamp((HitLocation.Y - Origin.Y) / Extent.Y, -1.0, 1.0);
    const double Tilt = Position * FMath::Clamp(MaxTiltNormalDeg, 0.0f, 89.0f);
    OutNormal = FRotator(0, 0, Tilt).RotateVector(Normal);
    IsHitTopSurface = true;
}

FVector UBreakoutAimingLibrary::ClampDirection(FVector InDirection, float MinHorizontalAngleDeg)
{
    if (InDirection.ContainsNaN() || InDirection.IsNearlyZero()) return FVector::UpVector;
    const double Minimum = FMath::Clamp(MinHorizontalAngleDeg, 0.0f, 90.0f);
    const double Angle = FMath::RadiansToDegrees(FMath::Atan2(InDirection.Y, InDirection.Z));
    const double Clamped = FMath::DegreesToRadians(FMath::Clamp(Angle, -90.0 + Minimum, 90.0 - Minimum));
    return FVector(0, FMath::Sin(Clamped), FMath::Cos(Clamped));
}

FVector UBreakoutAimingLibrary::ReflectDirection(AActor* Ball, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector InDirection, FVector HitLocation, FVector Normal)
{
    if (OtherActor && OtherComp && OtherComp->GetOwner() == OtherActor && OtherComp->ComponentHasTag(TEXT("Player")))
        if (const UBreakoutPaddleAimComponent* Aim = OtherActor->FindComponentByClass<UBreakoutPaddleAimComponent>())
        {
            bool bTop;
            FVector TiltedNormal;
            Aim->GetTopNormal(HitLocation, Normal, bTop, TiltedNormal);
            if (bTop)
            {
                const UBreakoutBallRulesComponent* Rules = Ball ? Ball->FindComponentByClass<UBreakoutBallRulesComponent>() : nullptr;
                return ClampDirection(InDirection.MirrorByVector(TiltedNormal), Rules ? Rules->MinHorizontalAngleDeg : 10.0f);
            }
        }
    return InDirection.MirrorByVector(Normal);
}

void UBreakoutPaddleAimComponent::DrawDebugArrow(FVector Origin, FVector Direction) const
{
#if ENABLE_DRAW_DEBUG
    ::DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + Direction.GetSafeNormal() * 200,
        100, FColor::Blue, false, 0, 0, 10);
#endif
}

void UBreakoutPaddleAimComponent::DrawDebugTopSurfaceNormals() const
{
    if (!GetOwner()) return;
    FVector Origin, Extent;
    GetOwner()->GetActorBounds(false, Origin, Extent);
    constexpr int32 DrawNumHalf = 10;
    for (int32 Index = -DrawNumHalf; Index <= DrawNumHalf; ++Index)
    {
        const FVector Point(Origin.X, Origin.Y + Extent.Y / DrawNumHalf * Index, Origin.Z + Extent.Z);
        bool bTop;
        FVector Normal;
        GetTopNormal(Point, FVector::UpVector, bTop, Normal);
        if (bTop) DrawDebugArrow(Point, Normal);
    }
}

void UBreakoutPaddleAimComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(bDrawTopSurfaceNormals);
}

void UBreakoutPaddleAimComponent::SetDebugNormalsEnabled(bool bEnabled)
{
    bDrawTopSurfaceNormals = bEnabled;
    SetComponentTickEnabled(bEnabled);
}

void UBreakoutPaddleAimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bDrawTopSurfaceNormals) DrawDebugTopSurfaceNormals();
}

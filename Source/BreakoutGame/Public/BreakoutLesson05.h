#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BreakoutLesson05.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup=(Breakout), meta=(BlueprintSpawnableComponent))
class BREAKOUTGAME_API UBreakoutPaddleAimComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBreakoutPaddleAimComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aiming", meta=(ClampMin="0",ClampMax="89"))
    float MaxTiltNormalDeg = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aiming|Debug") bool bDrawTopSurfaceNormals = false;
    UFUNCTION(BlueprintPure, Category="Aiming")
    void GetTopNormal(FVector HitLocation, FVector Normal, bool& IsHitTopSurface, FVector& OutNormal) const;
    UFUNCTION(BlueprintCallable, Category="Aiming|Debug") void DrawDebugArrow(FVector Origin, FVector Direction) const;
    UFUNCTION(BlueprintCallable, Category="Aiming|Debug") void DrawDebugTopSurfaceNormals() const;
    UFUNCTION(BlueprintCallable, Category="Aiming|Debug") void SetDebugNormalsEnabled(bool bEnabled);
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

UCLASS()
class BREAKOUTGAME_API UBreakoutAimingLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Aiming")
    static FVector ClampDirection(FVector InDirection, float MinHorizontalAngleDeg = 10.0f);
    UFUNCTION(BlueprintPure, Category="Aiming", meta=(DefaultToSelf="Ball"))
    static FVector ReflectDirection(AActor* Ball, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector InDirection, FVector HitLocation, FVector Normal);
};

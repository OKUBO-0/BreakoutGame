#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakoutLesson06.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

namespace BreakoutCollision
{
    constexpr ECollisionChannel Item = ECC_GameTraceChannel1;
    constexpr ECollisionChannel Ball = ECC_GameTraceChannel2;
    constexpr ECollisionChannel MissArea = ECC_GameTraceChannel3;
}

UCLASS(Blueprintable)
class BREAKOUTGAME_API ABreakoutAddBallItem : public AActor
{
    GENERATED_BODY()
public:
    ABreakoutAddBallItem();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UStaticMeshComponent> Sphere;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UTextRenderComponent> TextRender;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="0")) float Speed = 600.0f;
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
private:
    bool bHandled = false;
};

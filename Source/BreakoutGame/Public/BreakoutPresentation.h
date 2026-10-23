#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "BreakoutPresentation.generated.h"

class UStaticMeshComponent;

// Slate-backed, asset-free overlay: does not replace the editable lesson widgets.
UCLASS()
class BREAKOUTGAME_API UBreakoutScoreOverlay : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};

// Short-lived, non-colliding shards, using the already-cooked block mesh/material.
UCLASS()
class BREAKOUTGAME_API ABreakoutBlockBurst : public AActor
{
    GENERATED_BODY()
public:
    ABreakoutBlockBurst();
    void SetColor(FLinearColor Color);
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Shards;
    float Age = 0;
};

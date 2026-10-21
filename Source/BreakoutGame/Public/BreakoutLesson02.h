#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "BreakoutLesson02.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;
class UWidgetAnimation;
class UTextBlock;

UCLASS(Blueprintable)
class BREAKOUTGAME_API UBreakoutClearWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> FloatAnimation;
protected:
    virtual void NativeConstruct() override;
};

UCLASS(Blueprintable)
class BREAKOUTGAME_API UBreakoutGameOverWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> ColorAnimation;
protected:
    virtual void NativeConstruct() override;
};

UCLASS(Blueprintable)
class BREAKOUTGAME_API ABreakoutGameManager : public AActor
{
    GENERATED_BODY()
public:
    ABreakoutGameManager();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blocks") int32 BlockNum = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blocks") int32 BrokenBlockNum = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State") bool IsCleared = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State") bool IsGameOver = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="UI") TSoftClassPtr<UUserWidget> ClearWidgetClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="UI") TSoftClassPtr<UUserWidget> GameOverWidgetClass;
    UPROPERTY(Transient, BlueprintReadOnly, Category="UI") TObjectPtr<UUserWidget> ResultWidget;

    UFUNCTION(BlueprintCallable, Category="Blocks") void AddBlockNum();
    UFUNCTION(BlueprintCallable, Category="Blocks") void AddBrokenBlockNum();
    UFUNCTION(BlueprintPure, Category="Blocks") int32 GetLeftBlockNum() const;
    UFUNCTION(BlueprintCallable, Category="UI") void ViewClearWidget();
    UFUNCTION(BlueprintCallable, Category="UI") void ViewGameOverWidget();
    UFUNCTION(BlueprintCallable, Category="State") void MissCount();
    static ABreakoutGameManager* Find(UWorld* World);
};

UCLASS(Blueprintable)
class BREAKOUTGAME_API ABreakoutBlock : public AActor
{
    GENERATED_BODY()
public:
    ABreakoutBlock();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block") TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block") TObjectPtr<UStaticMeshComponent> Cube;
    UFUNCTION(BlueprintCallable, Category="Block") void BreakBlock();
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnCubeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
private:
    bool bBroken = false;
    TWeakObjectPtr<ABreakoutGameManager> Manager;
};

// Kept on the existing BP_Ball so its slide-01_01 movement graph stays editable.
UCLASS(ClassGroup=(Breakout), meta=(BlueprintSpawnableComponent))
class BREAKOUTGAME_API UBreakoutBallRulesComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBreakoutBallRulesComponent();
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnBallOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
private:
    bool bMissHandled = false;
};

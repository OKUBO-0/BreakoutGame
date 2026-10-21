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
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class ABreakoutGameManager;

UCLASS(Blueprintable)
class BREAKOUTGAME_API UBreakoutGameInformation : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> LeftBallTextBox;
    void Refresh(int32 LeftBallNum);
protected:
    virtual void NativeConstruct() override;
};

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
    UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> GameOverAnimation;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball", meta=(ClampMin="0")) int32 LeftBallNum = 3;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ball") bool IsBallSpawned = false;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Ball") TObjectPtr<AActor> SpawnLocationActor;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ball") TSoftClassPtr<AActor> BallClass;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ball") TObjectPtr<AActor> ActiveBall;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="UI") TSoftClassPtr<UBreakoutGameInformation> GameInformationClass;
    UPROPERTY(Transient, BlueprintReadOnly, Category="UI") TObjectPtr<UBreakoutGameInformation> GameInformation;

    UFUNCTION(BlueprintCallable, Category="Blocks") void AddBlockNum();
    UFUNCTION(BlueprintCallable, Category="Blocks") void AddBrokenBlockNum();
    UFUNCTION(BlueprintPure, Category="Blocks") int32 GetLeftBlockNum() const;
    UFUNCTION(BlueprintCallable, Category="UI") void ViewClearWidget();
    UFUNCTION(BlueprintCallable, Category="UI") void ViewGameOverWidget();
    UFUNCTION(BlueprintCallable, Category="State") void MissCount();
    UFUNCTION(BlueprintCallable, Category="Ball") void SpawnBall();
    UFUNCTION(BlueprintCallable, Category="Input") void Action();
    UFUNCTION(BlueprintPure, Category="Ball") int32 GetLeftBallNum() const { return LeftBallNum; }
    UFUNCTION(BlueprintCallable, Category="State") void LevelReset();
    static ABreakoutGameManager* Find(UWorld* World);
protected:
    virtual void BeginPlay() override;
private:
    UFUNCTION() void OnBallDestroyed(AActor* Ball);
    bool bResetPending = false;
};

// Input stays on the possessed Paddle. Its existing movement graph is unchanged.
UCLASS(ClassGroup=(Breakout), meta=(BlueprintSpawnableComponent))
class BREAKOUTGAME_API UBreakoutPaddleActionsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBreakoutPaddleActionsComponent();
    UPROPERTY(Transient, BlueprintReadOnly, Category="Input") TObjectPtr<ABreakoutGameManager> GameManager;
protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void ActionStarted();
    UPROPERTY() TObjectPtr<UInputAction> ActionInput;
    UPROPERTY() TObjectPtr<UInputMappingContext> InputMapping;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    uint32 BindingHandle = 0;
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

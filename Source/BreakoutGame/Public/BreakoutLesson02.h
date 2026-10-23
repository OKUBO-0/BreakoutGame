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
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;

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
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Levels") FName NextLevelName;

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
    UFUNCTION(BlueprintCallable, Category="Levels") void OpenNextLevel();
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", meta=(ClampMin="1")) int32 Hp = 1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block") TObjectPtr<UTextRenderComponent> TextRender;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Block") TArray<FLinearColor> ColorTable;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Block") TSoftObjectPtr<UMaterialInterface> BlockMaterial;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Block") TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
    UFUNCTION(BlueprintCallable, Category="Block") void ReloadHp();
    UFUNCTION(BlueprintCallable, Category="Block") void HitBlock();
    UFUNCTION(BlueprintCallable, Category="Block") void BreakBlock();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnCubeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
private:
    bool bBroken = false;
    TWeakObjectPtr<ABreakoutGameManager> Manager;
    uint64 LastImpactFrame = MAX_uint64;
};

// Kept on the existing BP_Ball so its slide-01_01 movement graph stays editable.
UCLASS(ClassGroup=(Breakout), meta=(BlueprintSpawnableComponent))
class BREAKOUTGAME_API UBreakoutBallRulesComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBreakoutBallRulesComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aiming", meta=(ClampMin="0",ClampMax="90")) float MinHorizontalAngleDeg = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound") TSoftObjectPtr<USoundBase> KnockSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound", meta=(ClampMin="0",ClampMax="1")) float KnockVolume = 0.65f;
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnBallOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION() void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
private:
    bool bMissHandled = false;
    uint64 LastSoundFrame = MAX_uint64;
};

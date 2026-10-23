#include "BreakoutLesson02.h"
#include "Animation/WidgetAnimation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "BreakoutLesson04.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Misc/PackageName.h"
#include "BreakoutLesson06.h"
#include "BreakoutPresentation.h"

void UBreakoutGameInformation::NativeConstruct()
{
    Super::NativeConstruct();
    if (const ABreakoutGameManager* Manager = ABreakoutGameManager::Find(GetWorld())) Refresh(Manager->GetLeftBallNum());
}

void UBreakoutGameInformation::Refresh(int32 LeftBallNum)
{
    if (LeftBallTextBox) LeftBallTextBox->SetText(FText::FromString(FString::Printf(TEXT("LeftBall : %d"), LeftBallNum)));
}

void UBreakoutClearWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (FloatAnimation) PlayAnimation(FloatAnimation, 0.0f, 3, EUMGSequencePlayMode::Forward, 1.0f, false);
}

void UBreakoutGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UWidgetAnimation* Animation = GameOverAnimation ? GameOverAnimation : ColorAnimation;
    if (Animation) PlayAnimation(Animation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, false);
}

ABreakoutGameManager::ABreakoutGameManager()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    ClearWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Breakout/UI/ClearWidget.ClearWidget_C")));
    GameOverWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Breakout/UI/GameOverWidget.GameOverWidget_C")));
    BallClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Breakout/BP_Ball.BP_Ball_C")));
    GameInformationClass = TSoftClassPtr<UBreakoutGameInformation>(FSoftObjectPath(TEXT("/Game/Breakout/UI/GameInformation.GameInformation_C")));
}

void ABreakoutGameManager::BeginPlay()
{
    Super::BeginPlay();
    if (const UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance()))
    {
        if (Instance->IsValidBallNum()) LeftBallNum = Instance->LeftBallNum;
        StageStartScore = Instance->RunScore;
    }
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (UClass* WidgetClass = GameInformationClass.LoadSynchronous())
        {
            GameInformation = CreateWidget<UBreakoutGameInformation>(PC, WidgetClass);
            if (GameInformation) GameInformation->AddToViewport();
        }
        ScoreOverlay = CreateWidget<UBreakoutScoreOverlay>(PC);
        if (ScoreOverlay) ScoreOverlay->AddToViewport(20);
    }
}

int32 ABreakoutGameManager::GetScore() const
{
    const UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance());
    return Instance ? Instance->RunScore : 0;
}

int32 ABreakoutGameManager::GetComboMultiplier() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() - LastBreakTime <= 4.0f
        ? FMath::Clamp(ComboCount, 1, 5) : 1;
}

FText ABreakoutGameManager::GetStatusText() const
{
    const UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance());
    return FText::FromString(FString::Printf(TEXT("LEVEL %d / 2\nSCORE %d   BEST %d\nBLOCKS %d / %d   COMBO x%d"),
        NextLevelName.IsNone() ? 2 : 1, GetScore(), Instance ? Instance->SessionBestScore : 0,
        GetLeftBlockNum(), BlockNum, GetComboMultiplier()));
}

FText ABreakoutGameManager::GetActionHint() const
{
    if (IsCleared) return FText::FromString(NextLevelName.IsNone()
        ? TEXT("ALL CLEAR!   SPACE: BACK TO TITLE") : TEXT("STAGE CLEAR!   SPACE: NEXT LEVEL"));
    if (IsGameOver) return FText::FromString(TEXT("SPACE: RETRY THIS STAGE"));
    return FText::FromString(InGameBallNum == 0 ? TEXT("A / D: MOVE   SPACE: LAUNCH")
        : TEXT("BREAK WITHIN 4s FOR COMBO x5!   CATCH A: EXTRA BALL"));
}

void ABreakoutGameManager::SpawnBall()
{
    if (IsCleared || IsGameOver || bResetPending || InGameBallNum > 0 || LeftBallNum <= 0) return;
    if (GenerateBall())
    {
        --LeftBallNum;
        if (GameInformation) GameInformation->Refresh(LeftBallNum);
    }
}

AActor* ABreakoutGameManager::GenerateBall()
{
    if (IsCleared || IsGameOver || bResetPending || !GetWorld() || GetWorld()->bIsTearingDown) return nullptr;
    if (!IsValid(SpawnLocationActor))
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnBall requires SpawnLocationActor."));
        return nullptr;
    }
    UClass* Class = BallClass.LoadSynchronous();
    if (!Class) return nullptr;
    const FTransform Transform(FRotator::ZeroRotator, SpawnLocationActor->GetActorLocation(), FVector::OneVector);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    AActor* Ball = GetWorld()->SpawnActor<AActor>(Class, Transform, Params);
    // Failed spawns never consume a life or lock the next attempt.
    if (!IsValid(Ball)) return nullptr;
    ActiveBall = Ball;
    LiveBalls.Add(Ball);
    ++InGameBallNum;
    Ball->OnDestroyed.AddDynamic(this, &ABreakoutGameManager::OnBallDestroyed);
    return Ball;
}

void ABreakoutGameManager::OnBallDestroyed(AActor* Ball)
{
    if (!LiveBalls.Remove(Ball)) return;
    if (Ball == ActiveBall)
    {
        ActiveBall = nullptr;
        for (const TWeakObjectPtr<AActor>& Live : LiveBalls)
            if (Live.IsValid()) { ActiveBall = Live.Get(); break; }
    }
    if (bResetPending || GetWorld()->bIsTearingDown) return;
    MissCount();
}

void ABreakoutGameManager::Action()
{
    if (IsGameOver) LevelReset();
    else if (IsCleared) OpenNextLevel();
    else SpawnBall();
}

void ABreakoutGameManager::LevelReset()
{
    if (!IsGameOver || !LiveBalls.IsEmpty() || bResetPending) return;
    if (UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance()))
    {
        Instance->LeftBallNum = -1;
        Instance->RunScore = StageStartScore;
    }
    bResetPending = true;
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void ABreakoutGameManager::OpenNextLevel()
{
    if (!IsCleared || IsGameOver || bResetPending) return;
    // Short-name disk searches cannot discover maps inside a packaged IoStore.
    // Resolve relative map names next to this map, then query the package system.
    const bool bFinalStage = NextLevelName.IsNone();
    FString PackageName = bFinalStage ? TEXT("/Game/TitleLevel") : NextLevelName.ToString();
    if (FPackageName::IsShortPackageName(PackageName))
        PackageName = FPackageName::GetLongPackagePath(GetWorld()->GetOutermost()->GetName()) / PackageName;
    if (!FPackageName::DoesPackageExist(PackageName))
    {
        UE_LOG(LogTemp, Warning, TEXT("Next level does not exist: %s"), *NextLevelName.ToString());
        return;
    }
    UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance());
    if (!Instance) { UE_LOG(LogTemp, Error, TEXT("BreakoutGameInstance must be selected in Maps & Modes.")); return; }
    // InGameBallNum stays frozen after clear, even when those balls fall.
    Instance->LeftBallNum = bFinalStage ? -1 : FMath::Max(0, LeftBallNum) + FMath::Max(0, InGameBallNum);
    bResetPending = true;
    UGameplayStatics::OpenLevel(this, FName(*PackageName));
}

UBreakoutPaddleActionsComponent::UBreakoutPaddleActionsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBreakoutPaddleActionsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    APawn* Paddle = Cast<APawn>(GetOwner());
    APlayerController* PC = Paddle ? Cast<APlayerController>(Paddle->GetController()) : nullptr;
    UEnhancedInputComponent* Input = Paddle ? Cast<UEnhancedInputComponent>(Paddle->InputComponent) : nullptr;
    if (!PC || !PC->IsLocalController() || !Input) return;
    GameManager = ABreakoutGameManager::Find(GetWorld());
    ActionInput = LoadObject<UInputAction>(nullptr, TEXT("/Game/Breakout/Input/IA_Action.IA_Action"));
    InputMapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Breakout/Input/IMC_InGame.IMC_InGame"));
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
    if (!GameManager || !ActionInput || !InputMapping || !Subsystem) return;
    Subsystem->AddMappingContext(InputMapping, 0);
    BindingHandle = Input->BindAction(ActionInput, ETriggerEvent::Started, this, &UBreakoutPaddleActionsComponent::ActionStarted).GetHandle();
    BoundInput = Input;
    SetComponentTickEnabled(false);
}

void UBreakoutPaddleActionsComponent::ActionStarted()
{
    if (IsValid(GameManager)) GameManager->Action();
}

void UBreakoutPaddleActionsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BoundInput.IsValid()) BoundInput->RemoveBindingByHandle(BindingHandle);
    if (APawn* Paddle = Cast<APawn>(GetOwner()))
        if (APlayerController* PC = Cast<APlayerController>(Paddle->GetController()))
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
                if (InputMapping) Subsystem->RemoveMappingContext(InputMapping);
    Super::EndPlay(EndPlayReason);
}

ABreakoutGameManager* ABreakoutGameManager::Find(UWorld* World)
{
    if (World) for (TActorIterator<ABreakoutGameManager> It(World); It; ++It) return *It;
    return nullptr;
}

void ABreakoutGameManager::AddBlockNum()
{
    ++BlockNum;
    UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("AddBlockNum : %d"), BlockNum),
        true, true, FLinearColor(0.0f, 0.7f, 1.0f), 2.0f);
}

void ABreakoutGameManager::AddBrokenBlockNum()
{
    if (IsCleared || IsGameOver || BrokenBlockNum >= BlockNum) return;
    const float Now = GetWorld()->GetTimeSeconds();
    ComboCount = Now - LastBreakTime <= 4.0f ? FMath::Min(ComboCount + 1, 5) : 1;
    LastBreakTime = Now;
    if (UBreakoutGameInstance* Instance = Cast<UBreakoutGameInstance>(GetGameInstance()))
    {
        Instance->RunScore += 100 * GetComboMultiplier();
        Instance->SessionBestScore = FMath::Max(Instance->SessionBestScore, Instance->RunScore);
    }
    ++BrokenBlockNum;
    UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("BrokenBlockNum : %d"), BrokenBlockNum),
        true, true, FLinearColor(0.1f, 0.2f, 1.0f), 2.0f);
    UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("LeftBlockNum : %d"), GetLeftBlockNum()),
        true, true, FLinearColor(0.0f, 1.0f, 0.3f), 2.0f);
    if (BlockNum > 0 && GetLeftBlockNum() == 0)
    {
        IsCleared = true;
        ViewClearWidget();
    }
}

int32 ABreakoutGameManager::GetLeftBlockNum() const
{
    return FMath::Max(0, BlockNum - BrokenBlockNum);
}

void ABreakoutGameManager::ViewClearWidget()
{
    if (IsGameOver || ResultWidget) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && ClearWidgetClass.LoadSynchronous())
    {
        ResultWidget = CreateWidget<UUserWidget>(PC, ClearWidgetClass.Get());
        if (ResultWidget) ResultWidget->AddToViewport(10);
    }
}

void ABreakoutGameManager::ViewGameOverWidget()
{
    if (IsCleared || ResultWidget) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && GameOverWidgetClass.LoadSynchronous())
    {
        ResultWidget = CreateWidget<UUserWidget>(PC, GameOverWidgetClass.Get());
        if (ResultWidget) ResultWidget->AddToViewport(10);
    }
}

void ABreakoutGameManager::MissCount()
{
    if (IsCleared || IsGameOver) return;
    ComboCount = 0;
    LastBreakTime = -100.0f;
    InGameBallNum = FMath::Max(0, InGameBallNum - 1);
    if (LeftBallNum > 0 || InGameBallNum > 0) return;
    IsGameOver = true;
    ViewGameOverWidget();
}

ABreakoutBlock::ABreakoutBlock()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    SetRootComponent(SceneRoot);
    Cube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));
    Cube->SetupAttachment(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> White(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Mesh.Succeeded()) Cube->SetStaticMesh(Mesh.Object);
    if (White.Succeeded()) Cube->SetMaterial(0, White.Object);
    Cube->SetRelativeScale3D(FVector(1.0, 5.0, 1.5));
    Cube->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Cube->SetNotifyRigidBodyCollision(true);
    Cube->SetGenerateOverlapEvents(false);
    Cube->OnComponentHit.AddDynamic(this, &ABreakoutBlock::OnCubeHit);
    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->SetupAttachment(SceneRoot);
    TextRender->SetRelativeLocation(FVector(-51, 0, 0));
    TextRender->SetRelativeRotation(FRotator(0, 180, 0));
    TextRender->SetHorizontalAlignment(EHTA_Center);
    TextRender->SetVerticalAlignment(EVRTA_TextCenter);
    TextRender->SetTextRenderColor(FColor::Black);
    TextRender->SetWorldSize(150);
    TextRender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlockMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Breakout/BlockMaterial.BlockMaterial")));
    ItemClass = TSoftClassPtr<ABreakoutAddBallItem>(FSoftObjectPath(TEXT("/Game/Breakout/AddBallItem.AddBallItem_C")));
    ColorTable = {FLinearColor::White, FLinearColor(FColor(153,255,255)), FLinearColor(FColor(255,153,255)),
        FLinearColor(FColor(255,153,153)), FLinearColor(FColor(255,255,153))};
}

void ABreakoutBlock::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    MaterialInstance = nullptr;
    ReloadHp();
}

void ABreakoutBlock::ReloadHp()
{
    if (TextRender) TextRender->SetText(FText::AsNumber(Hp));
    if (!MaterialInstance && Cube)
        if (UMaterialInterface* Material = BlockMaterial.LoadSynchronous())
            MaterialInstance = Cube->CreateDynamicMaterialInstance(0, Material);
    if (MaterialInstance && ColorTable.IsValidIndex(Hp - 1))
        MaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), ColorTable[Hp - 1]);
}

void ABreakoutBlock::HitBlock()
{
    if (bBroken) return;
    Hp = FMath::Max(0, Hp - 1);
    ReloadHp();
    if (Hp <= 0) BreakBlock();
}

void ABreakoutBlock::BeginPlay()
{
    Super::BeginPlay();
    ReloadHp();
    Manager = ABreakoutGameManager::Find(GetWorld());
    if (Manager.IsValid()) Manager->AddBlockNum();
    else UE_LOG(LogTemp, Error, TEXT("Block requires a GameManager in this level."));
}

void ABreakoutBlock::OnCubeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || !OtherActor->FindComponentByClass<UBreakoutBallRulesComponent>()) return;
    if (LastImpactFrame != GFrameCounter)
    {
        LastImpactFrame = GFrameCounter;
        ImpactBallsThisFrame.Reset();
    }
    if (ImpactBallsThisFrame.Contains(OtherActor)) return;
    ImpactBallsThisFrame.Add(OtherActor);
    HitBlock();
}

void ABreakoutBlock::BreakBlock()
{
    if (bBroken) return;
    bBroken = true;
    // Count before destruction and guard against two hit callbacks in the same frame.
    if (Manager.IsValid()) Manager->AddBrokenBlockNum();
    if (ABreakoutBlockBurst* Burst = GetWorld()->SpawnActor<ABreakoutBlockBurst>(GetActorLocation(), FRotator::ZeroRotator))
        Burst->SetColor(ColorTable.IsValidIndex(Hp - 1) ? ColorTable[Hp - 1] : FLinearColor::White);
    if (ItemDropChance > 0 && FMath::FRand() < FMath::Clamp(ItemDropChance, 0.0f, 1.0f))
        if (UClass* Class = ItemClass.LoadSynchronous())
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            GetWorld()->SpawnActor<ABreakoutAddBallItem>(Class, GetActorTransform(), Params);
        }
    Destroy();
}

UBreakoutBallRulesComponent::UBreakoutBallRulesComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    KnockSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Breakout/Audio/Breakout_SE_Knock.Breakout_SE_Knock")));
}

void UBreakoutBallRulesComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UPrimitiveComponent* Ball = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
    {
        Ball->SetGenerateOverlapEvents(true);
        Ball->SetNotifyRigidBodyCollision(true);
        Ball->OnComponentHit.AddDynamic(this, &UBreakoutBallRulesComponent::OnBallHit);
        Ball->OnComponentBeginOverlap.AddDynamic(this, &UBreakoutBallRulesComponent::OnBallOverlap);
    }
}

void UBreakoutBallRulesComponent::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || bMissHandled || LastSoundFrame == GFrameCounter) return;
    LastSoundFrame = GFrameCounter;
    if (USoundBase* Sound = KnockSound.LoadSynchronous()) UGameplayStatics::PlaySound2D(this, Sound, KnockVolume);
}

void UBreakoutBallRulesComponent::OnBallOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (bMissHandled || !OtherComp || !OtherComp->ComponentHasTag(TEXT("MissArea"))) return;
    bMissHandled = true;
    // OnDestroyed updates the manager after the old ball is actually removed,
    // so the next Action cannot see a stale live ball at the spawn point.
    // The slides explicitly retain ball destruction even after clear.
    GetOwner()->Destroy();
}

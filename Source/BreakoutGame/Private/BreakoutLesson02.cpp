#include "BreakoutLesson02.h"
#include "Animation/WidgetAnimation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

void UBreakoutClearWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (FloatAnimation) PlayAnimation(FloatAnimation, 0.0f, 3, EUMGSequencePlayMode::Forward, 1.0f, false);
}

void UBreakoutGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ColorAnimation) PlayAnimation(ColorAnimation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, false);
}

ABreakoutGameManager::ABreakoutGameManager()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    ClearWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Breakout/UI/ClearWidget.ClearWidget_C")));
    GameOverWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Breakout/UI/GameOverWidget.GameOverWidget_C")));
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
}

void ABreakoutBlock::BeginPlay()
{
    Super::BeginPlay();
    Manager = ABreakoutGameManager::Find(GetWorld());
    if (Manager.IsValid()) Manager->AddBlockNum();
    else UE_LOG(LogTemp, Error, TEXT("Block requires a GameManager in this level."));
}

void ABreakoutBlock::OnCubeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this) BreakBlock();
}

void ABreakoutBlock::BreakBlock()
{
    if (bBroken) return;
    bBroken = true;
    // Count before destruction and guard against two hit callbacks in the same frame.
    if (Manager.IsValid()) Manager->AddBrokenBlockNum();
    Destroy();
}

UBreakoutBallRulesComponent::UBreakoutBallRulesComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBreakoutBallRulesComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UPrimitiveComponent* Ball = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
    {
        Ball->SetGenerateOverlapEvents(true);
        Ball->OnComponentBeginOverlap.AddDynamic(this, &UBreakoutBallRulesComponent::OnBallOverlap);
    }
}

void UBreakoutBallRulesComponent::OnBallOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (bMissHandled || !OtherComp || !OtherComp->ComponentHasTag(TEXT("MissArea"))) return;
    bMissHandled = true;
    if (ABreakoutGameManager* Manager = ABreakoutGameManager::Find(GetWorld())) Manager->MissCount();
    // The slides explicitly retain ball destruction even after clear.
    GetOwner()->Destroy();
}

#include "BreakoutPresentation.h"
#include "BreakoutLesson02.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UBreakoutScoreOverlay::RebuildWidget()
{
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24, 20)
        [SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
            .ColorAndOpacity(FLinearColor::White)
            .ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(FLinearColor::Black)
            .Text_Lambda([this]() {
                const ABreakoutGameManager* Manager = ABreakoutGameManager::Find(GetWorld());
                return Manager ? Manager->GetStatusText() : FText::GetEmpty();
            })]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(16, 20)
        [SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
            .ColorAndOpacity(FLinearColor(1.0f, 0.9f, 0.45f))
            .ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(FLinearColor::Black)
            .Text_Lambda([this]() {
                const ABreakoutGameManager* Manager = ABreakoutGameManager::Find(GetWorld());
                return Manager ? Manager->GetActionHint() : FText::GetEmpty();
            })];
}

ABreakoutBlockBurst::ABreakoutBlockBurst()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Breakout/BlockMaterial.BlockMaterial"));
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shard%d"), Index));
        Shard->SetupAttachment(RootComponent);
        if (Mesh.Succeeded()) Shard->SetStaticMesh(Mesh.Object);
        if (Material.Succeeded()) Shard->SetMaterial(0, Material.Object);
        Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shard->SetGenerateOverlapEvents(false);
        Shard->SetCastShadow(false);
        Shard->SetRelativeScale3D(FVector(0.24f));
        Shards.Add(Shard);
    }
    InitialLifeSpan = 0.45f;
}

void ABreakoutBlockBurst::SetColor(FLinearColor Color)
{
    for (UStaticMeshComponent* Shard : Shards)
        if (UMaterialInstanceDynamic* Material = Shard->CreateDynamicMaterialInstance(0))
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
}

void ABreakoutBlockBurst::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const float Scale = 0.24f * FMath::Max(0.0f, 1.0f - Age / 0.45f);
    for (int32 Index = 0; Index < Shards.Num(); ++Index)
    {
        const float Angle = Index * UE_TWO_PI / Shards.Num();
        Shards[Index]->SetRelativeLocation(FVector(-70, FMath::Cos(Angle) * Age * 650,
            FMath::Sin(Angle) * Age * 650 - 350 * Age * Age));
        Shards[Index]->SetRelativeScale3D(FVector(Scale));
        Shards[Index]->SetRelativeRotation(FRotator(Age * 180, Index * 30, Age * 240));
    }
}

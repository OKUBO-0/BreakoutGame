#include "BreakoutLesson06.h"
#include "BreakoutLesson02.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABreakoutAddBallItem::ABreakoutAddBallItem()
{
    PrimaryActorTick.bCanEverTick = true;
    Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
    SetRootComponent(Sphere);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> White(TEXT("/Game/Breakout/M_Ball.M_Ball"));
    if (Mesh.Succeeded()) Sphere->SetStaticMesh(Mesh.Object);
    if (White.Succeeded()) Sphere->SetMaterial(0,White.Object);
    Sphere->SetCollisionProfileName(TEXT("Item"));
    Sphere->SetGenerateOverlapEvents(true);
    Sphere->ComponentTags.Add(TEXT("Item"));
    Sphere->OnComponentBeginOverlap.AddDynamic(this,&ABreakoutAddBallItem::OnSphereOverlap);
    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->SetupAttachment(Sphere);
    TextRender->SetRelativeLocation(FVector(-51,0,0));
    TextRender->SetRelativeRotation(FRotator(0,180,0));
    TextRender->SetHorizontalAlignment(EHTA_Center);
    TextRender->SetVerticalAlignment(EVRTA_TextCenter);
    TextRender->SetTextRenderColor(FColor::Black);
    TextRender->SetWorldSize(150);
    TextRender->SetText(FText::FromString(TEXT("A")));
    TextRender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABreakoutAddBallItem::BeginPlay()
{
    Super::BeginPlay();
    Sphere->SetCollisionProfileName(TEXT("Item"));
}

void ABreakoutAddBallItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bHandled) AddActorWorldOffset(FVector(0,0,-FMath::Max(0.0f,Speed)*DeltaSeconds),true);
}

void ABreakoutAddBallItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bHandled || !OtherComp) return;
    if (OtherComp->ComponentHasTag(TEXT("MissArea")))
    {
        bHandled = true;
        Destroy();
    }
    else if (OtherComp->ComponentHasTag(TEXT("Player")))
    {
        bHandled = true;
        if (ABreakoutGameManager* Manager = ABreakoutGameManager::Find(GetWorld())) Manager->GenerateBall();
        Destroy();
    }
}

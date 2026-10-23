#include "BreakoutLesson07.h"
#include "BreakoutLesson04.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"

ABreakoutTitleGameMode::ABreakoutTitleGameMode()
{
    DefaultPawnClass = nullptr;
}

void ABreakoutTitleGameMode::StartPlay()
{
    // Entering the title starts a fresh run, not a carry-over from Level2.
    if (UBreakoutGameInstance* GI = Cast<UBreakoutGameInstance>(GetGameInstance()))
    {
        GI->LeftBallNum = -1;
        GI->RunScore = 0;
    }
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this,0))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
        PC->FlushPressedKeys();
        for (TActorIterator<ACameraActor> It(GetWorld());It;++It) { PC->SetViewTarget(*It); break; }
    }
    Super::StartPlay();
}

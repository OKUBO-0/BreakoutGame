#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BreakoutLesson07.generated.h"

// The title has its own mode: never spawn the gameplay paddle here.
UCLASS()
class BREAKOUTGAME_API ABreakoutTitleGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ABreakoutTitleGameMode();
    virtual void StartPlay() override;
};

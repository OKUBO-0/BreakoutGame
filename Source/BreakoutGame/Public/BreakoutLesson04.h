#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BreakoutLesson04.generated.h"

UCLASS(Blueprintable)
class BREAKOUTGAME_API UBreakoutGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    // -1 is unset; zero is a valid reserve count.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball") int32 LeftBallNum = -1;
    UFUNCTION(BlueprintPure, Category="Ball") bool IsValidBallNum() const { return LeftBallNum >= 0; }
};

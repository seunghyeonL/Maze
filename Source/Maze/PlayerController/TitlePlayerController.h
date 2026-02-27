#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

class UTitleWidget;
class UMatchWidget;
class ULobbyWidget;
class UUserWidget;

UCLASS()
class MAZE_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UTitleWidget> TitleWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UMatchWidget> MatchWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	UUserWidget* ActiveWidget = nullptr;

private:
	void RefreshUI();
	void ClearActiveWidget();
	void SetupUIInput(UUserWidget* Widget);
	void SetupGameInput();
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	UFUNCTION() void HandleScreenChanged(EUIFlowScreen NewScreen);
};

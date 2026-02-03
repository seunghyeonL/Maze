#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIFlowSubsystem.generated.h"

UENUM(BlueprintType)
enum class EUIFlowScreen : uint8
{
	Title UMETA(DisplayName="Title"),
	Match UMETA(DisplayName="Match"),
	Lobby UMETA(DisplayName="Lobby"),
};

UCLASS()
class MAZE_API UUIFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="UIFlow")	
	EUIFlowScreen GetScreen() const { return Screen; }

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenTitle();

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenMatch();

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenLobby(bool bHost);

	UFUNCTION(BlueprintPure, Category="UIFlow")
	bool IsLobbyHost() const { return bLobbyHost; }

private:
	UPROPERTY()
	EUIFlowScreen Screen = EUIFlowScreen::Title;

	UPROPERTY()
	bool bLobbyHost = false;
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSubsystem/SOSManager.h"
#include "MatchWidget.generated.h"

class UButton;
class UListView;
class USOSManager;
class UUIFlowSubsystem;
class ULobbySearchResultItem;

UCLASS()
class MAZE_API UMatchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* CreateLobbyButton;

	UPROPERTY(meta = (BindWidget))
	UButton* FindLobbyButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ExitToTitleButton;

	UPROPERTY(meta = (BindWidget))
	UListView* LobbyList;

	UPROPERTY()
	USOSManager* SOSManager = nullptr;

	UPROPERTY()
	UUIFlowSubsystem* UIFlowSubsystem = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULobbySearchResultItem>> LobbyItems;

	UFUNCTION()
	void HandleCreateLobbyClicked();

	UFUNCTION()
	void HandleFindLobbyClicked();

	UFUNCTION()
	void HandleExitToTitleClicked();

	UFUNCTION()
	void HandleLobbyItemClicked(UObject* Item);

	UFUNCTION()
	void HandleSessionCreated(bool bSuccess);

	UFUNCTION()
	void HandleSessionsFound(bool bSuccess, const TArray<FFoundSessionInfo>& Results);

	UFUNCTION()
	void HandleSessionJoined(bool bSuccess);

private:
	void CacheSubsystems();
};

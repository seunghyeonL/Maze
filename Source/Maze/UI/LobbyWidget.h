#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "LobbyWidget.generated.h"

class UButton;
class UComboBoxString;
class UListView;
class USOSManager;
class UUIFlowSubsystem;
class ULobbyPlayerEntryItem;
class AMazeLobbyPlayerState;

UCLASS()
class MAZE_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* GameStartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ReadyButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ExitToMatchingButton;

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* MazeSizeComboBox;

	UPROPERTY(meta = (BindWidget))
	UListView* PlayerList;

	UPROPERTY()
	USOSManager* SOSManager = nullptr;

	UPROPERTY()
	UUIFlowSubsystem* UIFlowSubsystem = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULobbyPlayerEntryItem>> PlayerItems;

	UPROPERTY()
	TArray<TWeakObjectPtr<AMazeLobbyPlayerState>> BoundPlayerStates;

	UFUNCTION()
	void HandleGameStartClicked();

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleExitToMatchingClicked();

	UFUNCTION()
	void HandleReadyChanged(AMazeLobbyPlayerState* PlayerState, bool bIsReady);

private:
	void CacheSubsystems();
	void UpdateRoleVisibility();
	bool IsLobbyHost() const;
	void RefreshPlayerList();
	void BindPlayerStateReady(AMazeLobbyPlayerState* PlayerState);
	void UnbindPlayerStates();

	FTimerHandle RefreshTimerHandle;
};

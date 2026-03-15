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
class ULoadingOverlayWidget;
class UCommonModalWidget;
class AMazeLobbyGameState;

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

	/** 로딩 오버레이 (BP에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	ULoadingOverlayWidget* LoadingOverlay;

	/** 알림 모달 (BP에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	UCommonModalWidget* AlertModal;

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

	UFUNCTION()
	void HandleMazeSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleMazeSizeChanged(int32 NewMazeSize);

	UFUNCTION()
	void HandlePlayerListChanged();

private:
	void CacheSubsystems();
	void UpdateRoleVisibility();
	bool IsLobbyHost() const;
	void RefreshPlayerList();
	void BindPlayerStateReady(AMazeLobbyPlayerState* PlayerState);
	void UnbindPlayerStates();
	void ShowLoading(const FText& Message);
	void HideLoading();

	/** 알림 모달 헬퍼 */
	UFUNCTION()
	void ShowAlert(const FText& Title, const FText& Message);

	FTimerHandle GameStartTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AMazeLobbyGameState> BoundGameState;

	UFUNCTION()
	void HandleGameStarted();

	void BindGameStateMazeSize();
	void UnbindGameState();
};

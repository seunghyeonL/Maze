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
class ULoadingOverlayWidget;
class UCommonModalWidget;

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

	/** 로딩 표시 헬퍼 */
	void ShowLoading(const FText& Message);
	void HideLoading();

	/** 알림 모달 헬퍼 */
	UFUNCTION()
	void ShowAlert(const FText& Title, const FText& Message);

	/** Pending error 체크 (지연 호출용) */
	void CheckPendingError();

	FTimerHandle PendingErrorTimerHandle;
	
	int32 MaxPlayerNum = 4;
};

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenChangedBP, EUIFlowScreen, NewScreen);

UCLASS()
class MAZE_API UUIFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	mutable FOnScreenChangedBP OnScreenChanged;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="UIFlow")	
	EUIFlowScreen GetScreen() const { return Screen; }

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenTitle();

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenMatch();

	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetScreenLobby(bool bHost);

	/** ClientTravel 직전 호출 — 화면 상태만 설정하고 OnScreenChanged 브로드캐스트를 하지 않음.
	 *  새 World의 TitlePlayerController::BeginPlay → RefreshUI()가 상태를 읽어 위젯을 생성함. */
	void SetScreenLobbyForTravel(bool bHost);

	UFUNCTION(BlueprintPure, Category="UIFlow")
	bool IsLobbyHost() const { return bLobbyHost; }

	// ---- Pending Error (네트워크 실패 등에서 사용) ----
	
	/** 다음 화면 전환 시 표시할 에러 메시지 설정 */
	UFUNCTION(BlueprintCallable, Category="UIFlow")
	void SetPendingError(const FText& ErrorMessage);

	/** 대기 중인 에러 메시지가 있는지 확인 */
	UFUNCTION(BlueprintPure, Category="UIFlow")
	bool HasPendingError() const { return !PendingErrorMessage.IsEmpty(); }

	/** 대기 중인 에러 메시지 가져오기 (가져온 후 클리어됨) */
	UFUNCTION(BlueprintCallable, Category="UIFlow")
	FText ConsumePendingError();

private:
	UPROPERTY()
	EUIFlowScreen Screen = EUIFlowScreen::Title;

	UPROPERTY()
	bool bLobbyHost = false;

	UPROPERTY()
	FText PendingErrorMessage;
};

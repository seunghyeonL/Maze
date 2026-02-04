#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonModalWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalCancelled);

/**
 * 공통 모달 위젯
 * 성공/실패 알림, 확인/취소 요청에 사용
 */
UCLASS()
class MAZE_API UCommonModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 확인 버튼만 있는 알림 모달 표시 */
	UFUNCTION(BlueprintCallable, Category="UI|Modal")
	void ShowAlert(const FText& Title, const FText& Message);

	/** 확인/취소 버튼이 있는 확인 모달 표시 */
	UFUNCTION(BlueprintCallable, Category="UI|Modal")
	void ShowConfirm(const FText& Title, const FText& Message);

	/** 모달 닫기 */
	UFUNCTION(BlueprintCallable, Category="UI|Modal")
	void Close();

	/** 현재 표시 중인지 확인 */
	UFUNCTION(BlueprintPure, Category="UI|Modal")
	bool IsShowing() const { return bIsShowing; }

	/** 확인 버튼 클릭 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category="UI|Modal")
	FOnModalConfirmed OnConfirmed;

	/** 취소 버튼 클릭 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category="UI|Modal")
	FOnModalCancelled OnCancelled;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MessageText;

	UPROPERTY(meta = (BindWidget))
	UButton* ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CancelButton;

private:
	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	bool bIsShowing = false;
};

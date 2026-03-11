#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingOverlayWidget.generated.h"

class UTextBlock;
class UCircularThrobber;

/**
 * 전체 화면 로딩 오버레이 위젯
 * 비동기 작업 중 화면을 덮어 입력을 차단하고 로딩 상태를 표시
 */
UCLASS()
class MAZE_API ULoadingOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 로딩 메시지를 설정하고 표시 */
	UFUNCTION(BlueprintCallable, Category="UI|Loading")
	void Show(const FText& Message);

	/** 오버레이 숨기기 */
	UFUNCTION(BlueprintCallable, Category="UI|Loading")
	void Hide();

	/** 현재 표시 중인지 확인 */
	UFUNCTION(BlueprintPure, Category="UI|Loading")
	bool IsShowing() const { return bIsShowing; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LoadingText;

	UPROPERTY(meta = (BindWidget))
	UCircularThrobber* LoadingThrobber;

private:
	bool bIsShowing = false;
};

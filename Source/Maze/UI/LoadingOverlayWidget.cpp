#include "LoadingOverlayWidget.h"

#include "Components/TextBlock.h"
#include "Components/Throbber.h"

void ULoadingOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 기본적으로 숨김 상태로 시작
	SetVisibility(ESlateVisibility::Collapsed);
	bIsShowing = false;
}

void ULoadingOverlayWidget::Show(const FText& Message)
{
	if (LoadingText)
	{
		LoadingText->SetText(Message);
	}

	SetVisibility(ESlateVisibility::Visible);
	bIsShowing = true;
}

void ULoadingOverlayWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
	bIsShowing = false;
}

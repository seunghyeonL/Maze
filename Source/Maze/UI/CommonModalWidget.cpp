#include "CommonModalWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"

void UCommonModalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 기본적으로 숨김 상태로 시작
	SetVisibility(ESlateVisibility::Collapsed);
	bIsShowing = false;

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UCommonModalWidget::HandleConfirmClicked);
		ConfirmButton->OnClicked.AddDynamic(this, &UCommonModalWidget::HandleConfirmClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UCommonModalWidget::HandleCancelClicked);
		CancelButton->OnClicked.AddDynamic(this, &UCommonModalWidget::HandleCancelClicked);
	}
}

void UCommonModalWidget::NativeDestruct()
{
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UCommonModalWidget::HandleConfirmClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UCommonModalWidget::HandleCancelClicked);
	}

	Super::NativeDestruct();
}

void UCommonModalWidget::ShowAlert(const FText& Title, const FText& Message)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}

	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	// 알림 모달: 취소 버튼 숨김
	if (CancelButton)
	{
		CancelButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Visible);
	bIsShowing = true;
}

void UCommonModalWidget::ShowConfirm(const FText& Title, const FText& Message)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}

	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	// 확인 모달: 취소 버튼 표시
	if (CancelButton)
	{
		CancelButton->SetVisibility(ESlateVisibility::Visible);
	}

	SetVisibility(ESlateVisibility::Visible);
	bIsShowing = true;
}

void UCommonModalWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	bIsShowing = false;

	// 델리게이트 초기화 (다음 사용을 위해)
	OnConfirmed.Clear();
	OnCancelled.Clear();
}

void UCommonModalWidget::HandleConfirmClicked()
{
	OnConfirmed.Broadcast();
	Close();
}

void UCommonModalWidget::HandleCancelClicked()
{
	OnCancelled.Broadcast();
	Close();
}

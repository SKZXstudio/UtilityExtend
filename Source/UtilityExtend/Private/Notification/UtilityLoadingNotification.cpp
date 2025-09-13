#include "Notification/UtilityLoadingNotification.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/Engine.h"

UUtilityLoadingNotification::UUtilityLoadingNotification()
	: bIsCompleted(false)
{
}

bool UUtilityLoadingNotification::CreateNotification(const FString& Title, const FString& Text, const TArray<FString>& ButtonTexts, bool bShowProgressBar)
{
	// 如果已经有活跃的通知，先关闭它
	if (NotificationPtr.IsValid())
	{
		CloseNotification();
	}

	// 缓存按钮文本
	CachedButtonTexts = ButtonTexts;
	bIsCompleted = false;

	// 创建通知信息结构
	FNotificationInfo Info(FText::FromString(Text));
	Info.bFireAndForget = false; // 不自动消失
	Info.bUseLargeFont = false;
	Info.bUseThrobber = bShowProgressBar; // 显示加载动画
	Info.bUseSuccessFailIcons = true;

	// 设置标题（如果提供）
	if (!Title.IsEmpty())
	{
		Info.Text = FText::FromString(FString::Printf(TEXT("%s\n%s"), *Title, *Text));
	}

	// 创建按钮 - 使用简单的回调方式
	if (ButtonTexts.Num() > 0)
	{
		// 暂时不实现自定义按钮，使用默认的超链接样式按钮
		// 这可以在后续版本中扩展
		Info.HyperlinkText = FText::FromString(ButtonTexts[0]);
		Info.Hyperlink = FSimpleDelegate::CreateUObject(this, &UUtilityLoadingNotification::HandleDefaultButtonClick);
	}

	// 创建通知
	NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

	if (NotificationPtr.IsValid())
	{
		// 设置初始状态为 Pending（加载中）
		NotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		return true;
	}

	return false;
}

void UUtilityLoadingNotification::SetSuccess(const FString& NewText)
{
	if (!NotificationPtr.IsValid() || bIsCompleted)
	{
		return;
	}

	// 更新文本（如果提供）
	if (!NewText.IsEmpty())
	{
		NotificationPtr.Pin()->SetText(FText::FromString(NewText));
	}

	// 设置成功状态
	NotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Success);
	NotificationPtr.Pin()->ExpireAndFadeout();

	bIsCompleted = true;
	HandleNotificationCompleted(true);
}

void UUtilityLoadingNotification::SetError(const FString& NewText)
{
	if (!NotificationPtr.IsValid() || bIsCompleted)
	{
		return;
	}

	// 更新文本（如果提供）
	if (!NewText.IsEmpty())
	{
		NotificationPtr.Pin()->SetText(FText::FromString(NewText));
	}

	// 设置失败状态
	NotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Fail);
	NotificationPtr.Pin()->ExpireAndFadeout();

	bIsCompleted = true;
	HandleNotificationCompleted(false);
}

void UUtilityLoadingNotification::UpdateProgress(float Progress)
{
	if (!NotificationPtr.IsValid())
	{
		return;
	}

	// 限制进度值在 0.0 - 1.0 之间
	Progress = FMath::Clamp(Progress, 0.0f, 1.0f);
	
	// 注意：Slate 通知系统可能不直接支持进度条更新
	// 这里可以根据需要扩展实现
}

void UUtilityLoadingNotification::UpdateText(const FString& NewText)
{
	if (!NotificationPtr.IsValid())
	{
		return;
	}

	NotificationPtr.Pin()->SetText(FText::FromString(NewText));
}

void UUtilityLoadingNotification::CloseNotification()
{
	if (NotificationPtr.IsValid())
	{
		NotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_None);
		NotificationPtr.Pin()->ExpireAndFadeout();
		NotificationPtr.Reset();
	}
}

bool UUtilityLoadingNotification::IsNotificationActive() const
{
	return NotificationPtr.IsValid() && !bIsCompleted;
}

void UUtilityLoadingNotification::HandleButtonClick(int32 ButtonIndex, const FString& ButtonText)
{
	// 广播按钮点击事件
	OnButtonClicked.Broadcast(ButtonIndex, ButtonText);
}

void UUtilityLoadingNotification::HandleDefaultButtonClick()
{
	// 处理默认按钮点击（第一个按钮）
	if (CachedButtonTexts.Num() > 0)
	{
		HandleButtonClick(0, CachedButtonTexts[0]);
	}
}

void UUtilityLoadingNotification::HandleNotificationCompleted(bool bSuccess)
{
	// 广播完成事件
	OnCompleted.Broadcast(bSuccess);
}

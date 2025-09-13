#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UtilityLoadingNotification.generated.h"

// 委托声明 - 按钮点击事件（使用不同名称避免冲突）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadingNotificationButtonClicked, int32, ButtonIndex, const FString&, ButtonText);

// 委托声明 - 通知完成事件（成功/失败）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingNotificationCompleted, bool, bSuccess);

/**
 * 加载通知对象类
 * 提供更优雅的事件绑定方式，无需实现接口
 */
UCLASS(BlueprintType, Blueprintable)
class UTILITYEXTEND_API UUtilityLoadingNotification : public UObject
{
	GENERATED_BODY()

public:
	UUtilityLoadingNotification();

	// 委托属性 - 可在蓝图中绑定
	UPROPERTY(BlueprintAssignable, Category = "Notification Events")
	FOnLoadingNotificationButtonClicked OnButtonClicked;

	UPROPERTY(BlueprintAssignable, Category = "Notification Events")
	FOnLoadingNotificationCompleted OnCompleted;

	// 新的委托名称（与蓝图库中使用的名称一致）
	UPROPERTY(BlueprintAssignable, Category = "Notification Events")
	FOnLoadingNotificationButtonClicked OnLoadingNotificationButtonClicked;

	// 通知基本信息
	UPROPERTY(BlueprintReadOnly, Category = "Notification Info")
	FString NotificationId;

	UPROPERTY(BlueprintReadOnly, Category = "Notification Info")
	FString Message;

	/**
	 * 创建加载通知
	 * @param Title 通知标题
	 * @param Text 通知内容
	 * @param ButtonTexts 按钮文本数组
	 * @param bShowProgressBar 是否显示进度条
	 * @return 是否创建成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	bool CreateNotification(const FString& Title, const FString& Text, const TArray<FString>& ButtonTexts, bool bShowProgressBar = true);

	/**
	 * 更新通知状态为成功
	 * @param NewText 新的文本内容
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	void SetSuccess(const FString& NewText = TEXT(""));

	/**
	 * 更新通知状态为失败
	 * @param NewText 新的文本内容
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	void SetError(const FString& NewText = TEXT(""));

	/**
	 * 更新进度条进度
	 * @param Progress 进度值 (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	void UpdateProgress(float Progress);

	/**
	 * 更新通知文本
	 * @param NewText 新的文本内容
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	void UpdateText(const FString& NewText);

	/**
	 * 关闭通知
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification")
	void CloseNotification();

	/**
	 * 检查通知是否仍然活跃
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility Notification", BlueprintPure)
	bool IsNotificationActive() const;

protected:
	// 内部按钮点击处理函数
	void HandleButtonClick(int32 ButtonIndex, const FString& ButtonText);

	// 默认按钮点击处理函数（超链接方式）
	void HandleDefaultButtonClick();

	// 通知完成处理函数
	void HandleNotificationCompleted(bool bSuccess);

private:
	// 通知项的弱引用
	TWeakPtr<SNotificationItem> NotificationPtr;

	// 按钮文本数组（用于回调时获取按钮文本）
	TArray<FString> CachedButtonTexts;

	// 是否已经完成
	bool bIsCompleted;

public:
	// 通知项的共享指针（用于蓝图库访问）
	TSharedPtr<SNotificationItem> NotificationItem;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UtilityExtendBPLibrary.generated.h"

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UUtilityExtendBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 跟踪创建的通知
    static TArray<TSharedPtr<SNotificationItem>> CreatedNotifications;

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Show Editor Notification", Keywords = "调用编辑器通知 显示通知"), Category = "UtilityExtend")
    static void ShowEditorNotification(const FString& Message, float Duration = 5.0f, bool bAutoExpire = true, bool bUseThrobber = false, bool bUseSuccessIcon = false, bool bUseFailIcon = false);

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove All Editor Notifications", Keywords = "移除通知 清除通知"), Category = "UtilityExtend")
    static void RemoveAllEditorNotifications();

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Restart Editor", Keywords = "重启编辑器 重新启动"), Category = "UtilityExtend")
    static void RestartEditor();

    // 外部软件调用相关函数
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Launch External Application", 
        Keywords = "启动外部程序 运行exe 调用外部软件",
        ToolTip = "启动外部可执行文件，支持命令行参数和工作目录设置",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "启动成功") bool LaunchExternalApplication(
        UPARAM(DisplayName = "可执行文件路径") const FString& ExecutablePath, 
        UPARAM(DisplayName = "命令行参数") const FString& Arguments = TEXT(""), 
        UPARAM(DisplayName = "工作目录") const FString& WorkingDirectory = TEXT(""), 
        UPARAM(DisplayName = "分离启动") bool bLaunchDetached = true, 
        UPARAM(DisplayName = "隐藏启动") bool bLaunchHidden = false, 
        UPARAM(DisplayName = "最小化启动") bool bLaunchMinimized = false, 
        UPARAM(DisplayName = "最大化启动") bool bLaunchMaximized = false, 
        UPARAM(DisplayName = "正常启动") bool bLaunchNormal = false
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Launch External Application With Process Info", 
        Keywords = "启动外部程序详细信息 运行exe详细信息",
        ToolTip = "启动外部程序并返回详细信息，包括进程ID和错误信息",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "启动成功") bool LaunchExternalApplicationWithInfo(
        UPARAM(DisplayName = "可执行文件路径") const FString& ExecutablePath, 
        UPARAM(DisplayName = "命令行参数") const FString& Arguments, 
        UPARAM(DisplayName = "工作目录") const FString& WorkingDirectory, 
        UPARAM(DisplayName = "分离启动") bool bLaunchDetached, 
        UPARAM(DisplayName = "隐藏启动") bool bLaunchHidden, 
        UPARAM(DisplayName = "最小化启动") bool bLaunchMinimized, 
        UPARAM(DisplayName = "最大化启动") bool bLaunchMaximized, 
        UPARAM(DisplayName = "正常启动") bool bLaunchNormal, 
        UPARAM(DisplayName = "输出进程ID") FString& OutProcessID, 
        UPARAM(DisplayName = "输出错误信息") FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Is External Application Running", 
        Keywords = "检查外部程序是否运行 检查进程",
        ToolTip = "检查指定名称的程序是否正在运行",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "是否运行") bool IsExternalApplicationRunning(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Terminate External Application", 
        Keywords = "终止外部程序 结束进程",
        ToolTip = "强制终止指定的外部程序",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "终止成功") bool TerminateExternalApplication(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Get All Running Processes", 
        Keywords = "获取所有运行进程 列出进程",
        ToolTip = "获取系统中所有正在运行的进程列表",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "进程列表") TArray<FString> GetAllRunningProcesses();

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Wait For External Application", 
        Keywords = "等待外部程序 等待进程",
        ToolTip = "等待指定的外部程序启动，支持超时设置",
        Category = "UtilityExtend|ExternalApp"
    ))
    static UPARAM(DisplayName = "等待成功") bool WaitForExternalApplication(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName, 
        UPARAM(DisplayName = "超时时间") float TimeoutSeconds = 30.0f
    );
};

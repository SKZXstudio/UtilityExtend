// Copyright Epic Games, Inc. All Rights Reserved.

#include "Blueprint/UtilityExtendBPLibrary.h"
#include "Notification/UtilityLoadingNotification.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Editor/EditorEngine.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformMisc.h"
#include "UnrealEdMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFile.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Engine/Engine.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Framework/Docking/TabManager.h"

// Windows特定头文件
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

// 定义Windows常量（如果未定义）
#ifndef PROCESS_DETACHED
#define PROCESS_DETACHED 0x00000001
#endif

#ifndef CREATE_MINIMIZED
#define CREATE_MINIMIZED 0x00000001
#endif

#ifndef CREATE_MAXIMIZED
#define CREATE_MAXIMIZED 0x00000002
#endif

#ifndef CREATE_NO_WINDOW
#define CREATE_NO_WINDOW 0x08000000
#endif

#ifndef CREATE_NEW_CONSOLE
#define CREATE_NEW_CONSOLE 0x00000010
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
#endif

// 静态成员变量定义
TArray<TSharedPtr<SNotificationItem>> UUtilityExtendBPLibrary::CreatedNotifications;
FOnNotificationButtonClicked UUtilityExtendBPLibrary::OnNotificationButtonClicked;
TMap<FString, TSharedPtr<SNotificationItem>> UUtilityExtendBPLibrary::NotificationMap;
TArray<TWeakObjectPtr<UObject>> UUtilityExtendBPLibrary::NotificationEventReceivers;

// 创建常规通知函数（不带按钮）
FString UUtilityExtendBPLibrary::CreateEditorNotification(
    const FString& Message,
    EEditorNotificationType NotificationType,
    float Duration,
    bool bAutoExpire)
{
    // 检查Slate系统是否可用
    if (!FSlateApplication::IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("Slate系统未初始化，无法显示通知"));
        return FString();
    }

    // 生成唯一的通知ID
    FString NotificationId = FGuid::NewGuid().ToString();

    // 创建通知信息
    FNotificationInfo Info(FText::FromString(Message));
    
    // 确定完成状态
    SNotificationItem::ECompletionState CompletionState = SNotificationItem::CS_Pending;
    
    // 根据通知类型设置属性
    switch (NotificationType)
    {
    case EEditorNotificationType::Success:
        Info.bUseSuccessFailIcons = true;
        CompletionState = SNotificationItem::CS_Success;
        break;
    case EEditorNotificationType::Error:
        Info.bUseSuccessFailIcons = true;
        CompletionState = SNotificationItem::CS_Fail;
        break;
    case EEditorNotificationType::Default:
    default:
        // 默认通知不使用特殊图标和动画
        CompletionState = SNotificationItem::CS_None;
        break;
    }

    // 设置持续时间和自动过期
    Info.FadeInDuration = 0.1f;
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = bAutoExpire ? Duration : 0.0f;
    Info.bUseLargeFont = false;
    Info.bFireAndForget = bAutoExpire; // 如果自动过期，则设置FireAndForget
    Info.bAllowThrottleWhenFrameRateIsLow = false;

    // 常规通知不支持按钮

    // 显示通知
    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    
    if (NotificationItem.IsValid())
    {
        // 添加到跟踪数组
        CreatedNotifications.Add(NotificationItem);
        
        // 添加到ID映射表
        NotificationMap.Add(NotificationId, NotificationItem);
        
        // 设置完成状态
        NotificationItem->SetCompletionState(CompletionState);
    }

    return NotificationId;
}

// 创建带加载动画的通知函数（支持按钮）
FString UUtilityExtendBPLibrary::CreateEditorLoadingNotification(
    const FString& Message,
    UUtilityLoadingNotification*& OutNotificationObject,
    bool bShowButton,
    const FString& ButtonText,
    const FString& ButtonTooltip)
{
    // 检查Slate系统是否可用
    if (!FSlateApplication::IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("Slate系统未初始化，无法显示通知"));
        OutNotificationObject = nullptr;
        return FString();
    }

    // 生成唯一的通知ID
    FString NotificationId = FGuid::NewGuid().ToString();

    // 创建通知对象
    OutNotificationObject = NewObject<UUtilityLoadingNotification>();
    if (!OutNotificationObject)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建 UUtilityLoadingNotification 对象"));
        return FString();
    }

    // 设置通知对象的基本信息
    OutNotificationObject->NotificationId = NotificationId;
    OutNotificationObject->Message = Message;

    // 创建通知信息
    FNotificationInfo Info(FText::FromString(Message));
    
    // 设置加载动画样式
    Info.bUseThrobber = true;
    SNotificationItem::ECompletionState CompletionState = SNotificationItem::CS_Pending;

    // 设置持续时间和自动过期（加载通知默认不会自动过期）
    Info.FadeInDuration = 0.1f;
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f; // 不自动过期
    Info.bUseLargeFont = false;
    Info.bFireAndForget = false;
    Info.bAllowThrottleWhenFrameRateIsLow = false;

    // 如果显示按钮，添加按钮
    if (bShowButton && !ButtonText.IsEmpty())
    {
        // 创建按钮回调函数，使用弱引用避免悬空指针
        TWeakObjectPtr<UUtilityLoadingNotification> WeakNotificationObject = OutNotificationObject;
        FString CapturedButtonText = ButtonText;
        
        auto ButtonCallback = [WeakNotificationObject, CapturedButtonText]()
        {
            UE_LOG(LogTemp, Warning, TEXT("Button clicked with text: %s"), *CapturedButtonText);
            
            if (WeakNotificationObject.IsValid())
            {
                UUtilityLoadingNotification* NotificationObject = WeakNotificationObject.Get();
                if (NotificationObject && NotificationObject->OnButtonClicked.IsBound())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Broadcasting button click event"));
                    NotificationObject->OnButtonClicked.Broadcast(0, CapturedButtonText);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Notification object is valid but delegate is not bound"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Notification object is no longer valid"));
            }
        };

        // 添加按钮到通知
        Info.ButtonDetails.Add(FNotificationButtonInfo(
            FText::FromString(ButtonText),
            FText::FromString(ButtonTooltip.IsEmpty() ? ButtonText : ButtonTooltip),
            FSimpleDelegate::CreateLambda(ButtonCallback)
        ));
    }

    // 显示通知
    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    
    if (NotificationItem.IsValid())
    {
        // 将通知项保存到对象中
        OutNotificationObject->NotificationItem = NotificationItem;
        
        // 添加到跟踪数组
        CreatedNotifications.Add(NotificationItem);
        
        // 添加到ID映射表
        NotificationMap.Add(NotificationId, NotificationItem);
        
        // 设置完成状态
        NotificationItem->SetCompletionState(CompletionState);
    }

    return NotificationId;
}

// 创建复杂通知
UUtilityLoadingNotification* UUtilityExtendBPLibrary::CreateComplexNotification(const FString& Title, const FString& Text, const TArray<FString>& ButtonTexts, bool bShowProgressBar)
{
    // 创建通知对象实例
    UUtilityLoadingNotification* NotificationObject = NewObject<UUtilityLoadingNotification>();
    
    if (NotificationObject)
    {
        // 创建通知
        bool bSuccess = NotificationObject->CreateNotification(Title, Text, ButtonTexts, bShowProgressBar);
        
        if (bSuccess)
        {
            return NotificationObject;
    }
    else
    {
            // 创建失败，清理对象
            NotificationObject->MarkAsGarbage();
            return nullptr;
        }
    }
    
    return nullptr;
}

// 统一的清除通知函数
bool UUtilityExtendBPLibrary::RemoveEditorNotification(const FString& NotificationId, bool bRemoveAll)
{
    // 如果指定了清除全部或者NotificationId为空
    if (bRemoveAll || NotificationId.IsEmpty())
    {
        // 移除所有创建的通知
        for (TSharedPtr<SNotificationItem>& NotificationItem : CreatedNotifications)
        {
            if (NotificationItem.IsValid())
            {
                NotificationItem->ExpireAndFadeout();
            }
        }
        
        CreatedNotifications.Empty();
        NotificationMap.Empty();
        return true;
    }
    
    // 按ID移除指定通知
    if (NotificationMap.Contains(NotificationId))
    {
        TSharedPtr<SNotificationItem> NotificationItem = NotificationMap[NotificationId];
        if (NotificationItem.IsValid())
        {
            NotificationItem->ExpireAndFadeout();
            NotificationMap.Remove(NotificationId);
            CreatedNotifications.Remove(NotificationItem);
        return true;
    }
        NotificationMap.Remove(NotificationId);
    }
        return false;
    }

// ------------------------------------编辑器操作相关函数------------------------------------
// 重启引擎
void UUtilityExtendBPLibrary::RestartEditor()
{
    // 检查编辑器是否可用
    if (GEditor)
    {
        // 显示重启确认对话框
        FText Title = FText::FromString(TEXT("重启编辑器"));
        FText Message = FText::FromString(TEXT("确定要重启编辑器吗？这将关闭当前编辑器并重新启动。"));
        
        EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message, Title);
        
        if (Result == EAppReturnType::Yes)
        {
            // 使用UE引擎标准的重启编辑器方法，禁用项目切换提示
            FUnrealEdMisc::Get().RestartEditor(false);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Editor not available, cannot restart"));
    }
}

// 运行编辑器工具控件
bool UUtilityExtendBPLibrary::RunUtilityWidget(UEditorUtilityWidgetBlueprint* WidgetBlueprint, FString& OutTabId)
{
    OutTabId = FString();
    
    // 检查是否在编辑器环境中
    if (!GEditor)
    {
        UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: Not in editor environment"));
        return false;
    }

    // 检查Widget蓝图是否有效
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: WidgetBlueprint is null"));
        return false;
    }
    
    // 获取EditorUtilitySubsystem
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    if (!EditorUtilitySubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: Failed to get EditorUtilitySubsystem"));
    return false;
    }
    
    try
    {
        UE_LOG(LogTemp, Log, TEXT("RunUtilityWidget: Attempting to spawn widget blueprint: %s"), 
               *WidgetBlueprint->GetName());
        
        // 使用SpawnAndRegisterTabAndGetID方法运行Widget，可以设置自定义标签页名称
        FName TabId;
        UEditorUtilityWidget* SpawnedWidget = EditorUtilitySubsystem->SpawnAndRegisterTabAndGetID(
            WidgetBlueprint, 
            TabId
        );
        
        if (!SpawnedWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: Failed to spawn widget"));
        return false;
    }

        // 标签页创建成功
        
        // 成功创建Widget
        OutTabId = TabId.ToString();
        UE_LOG(LogTemp, Log, TEXT("RunUtilityWidget: Successfully spawned widget: %s with TabId: %s"), 
               *WidgetBlueprint->GetName(), *OutTabId);
                        return true;
                    }
    catch (const std::exception& e)
                    {
        UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: Exception occurred: %s"), ANSI_TO_TCHAR(e.what()));
                        return false;
                    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RunUtilityWidget: Unknown exception occurred"));
        return false;
    }
}

// 关闭编辑器工具控件
bool UUtilityExtendBPLibrary::CloseUtilityWidgetTab(const FString& TabId)
{
    // 检查是否在编辑器环境中
    if (!GEditor)
    {
        UE_LOG(LogTemp, Error, TEXT("CloseUtilityWidgetTab: Not in editor environment"));
    return false;
    }
    
    // 检查TabId是否有效
    if (TabId.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CloseUtilityWidgetTab: TabId is empty"));
        return false;
    }
    
    // 获取EditorUtilitySubsystem
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    if (!EditorUtilitySubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("CloseUtilityWidgetTab: Failed to get EditorUtilitySubsystem"));
        return false;
    }
    
    try
    {
        FName TabName = FName(*TabId);
        UE_LOG(LogTemp, Log, TEXT("CloseUtilityWidgetTab: Attempting to close tab with ID: %s"), *TabId);
        
        // 尝试关闭指定的标签页
        bool bClosed = EditorUtilitySubsystem->CloseTabByID(TabName);
        
        if (bClosed)
        {
            UE_LOG(LogTemp, Log, TEXT("CloseUtilityWidgetTab: Successfully closed tab with ID: %s"), *TabId);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CloseUtilityWidgetTab: Failed to close tab with ID: %s (tab may not exist)"), *TabId);
        }
        
        return bClosed;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("CloseUtilityWidgetTab: Exception occurred: %s"), ANSI_TO_TCHAR(e.what()));
        return false;
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("CloseUtilityWidgetTab: Unknown exception occurred"));
        return false;
    }
}

// ------------------------------------文件读写相关函数------------------------------------
// 文件读写相关函数实现
bool UUtilityExtendBPLibrary::ReadTextFile(const FString& FilePath, FString& OutContent, FString& OutErrorMessage)
{
    // 清空输出参数
    OutContent.Empty();
    OutErrorMessage.Empty();
    
    // 检查文件路径是否为空
    if (FilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("文件路径不能为空");
        UE_LOG(LogTemp, Error, TEXT("ReadTextFile: File path is empty"));
        return false;
    }
    
    // 处理相对路径和绝对路径
    FString FullPath = FilePath;
    if (!FPaths::IsRelative(FilePath))
    {
        // 绝对路径，直接使用
        FullPath = FilePath;
    }
    else
    {
        // 相对路径，转换为项目相对路径
        FullPath = FPaths::ProjectDir() + FilePath;
    }
    
    // 规范化路径
    FPaths::NormalizeFilename(FullPath);
    
    // 检查文件是否存在
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.FileExists(*FullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("文件不存在: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("ReadTextFile: File does not exist: %s"), *FullPath);
        return false;
    }
    
    // 读取文件内容
    if (!FFileHelper::LoadFileToString(OutContent, *FullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("读取文件失败: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("ReadTextFile: Failed to load file: %s"), *FullPath);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ReadTextFile: Successfully read file: %s (%d characters)"), *FullPath, OutContent.Len());
    return true;
}

bool UUtilityExtendBPLibrary::WriteTextFile(const FString& FilePath, const FString& Content, FString& OutErrorMessage, bool bOverwrite, bool bCreateDirectories)
{
    // 清空输出参数
    OutErrorMessage.Empty();
    
    // 检查文件路径是否为空
    if (FilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("文件路径不能为空");
        UE_LOG(LogTemp, Error, TEXT("WriteTextFile: File path is empty"));
        return false;
    }
    
    // 处理相对路径和绝对路径
    FString FullPath = FilePath;
    if (!FPaths::IsRelative(FilePath))
    {
        // 绝对路径，直接使用
        FullPath = FilePath;
    }
    else
    {
        // 相对路径，转换为项目相对路径
        FullPath = FPaths::ProjectDir() + FilePath;
    }
    
    // 规范化路径
    FPaths::NormalizeFilename(FullPath);
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // 检查文件是否存在，如果存在且不允许覆盖，则返回错误
    if (PlatformFile.FileExists(*FullPath) && !bOverwrite)
    {
        OutErrorMessage = FString::Printf(TEXT("文件已存在且不允许覆盖: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("WriteTextFile: File exists and overwrite is disabled: %s"), *FullPath);
        return false;
    }
    
    // 如果需要创建目录，先创建目录
    if (bCreateDirectories)
    {
        FString Directory = FPaths::GetPath(FullPath);
        if (!Directory.IsEmpty() && !PlatformFile.DirectoryExists(*Directory))
        {
            if (!PlatformFile.CreateDirectoryTree(*Directory))
            {
                OutErrorMessage = FString::Printf(TEXT("创建目录失败: %s"), *Directory);
                UE_LOG(LogTemp, Error, TEXT("WriteTextFile: Failed to create directory: %s"), *Directory);
                return false;
            }
        }
    }
    
    // 写入文件内容
    if (!FFileHelper::SaveStringToFile(Content, *FullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("写入文件失败: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("WriteTextFile: Failed to save file: %s"), *FullPath);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("WriteTextFile: Successfully wrote file: %s (%d characters)"), *FullPath, Content.Len());
    return true;
}

bool UUtilityExtendBPLibrary::CheckFileExists(const FString& FilePath)
{
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CheckFileExists: File path is empty"));
        return false;
    }
    
    // 处理相对路径和绝对路径
    FString FullPath = FilePath;
    if (!FPaths::IsRelative(FilePath))
    {
        FullPath = FilePath;
    }
    else
    {
        FullPath = FPaths::ProjectDir() + FilePath;
    }
    
    // 规范化路径
    FPaths::NormalizeFilename(FullPath);
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileExists(*FullPath);
}

int64 UUtilityExtendBPLibrary::GetFileSize(const FString& FilePath)
{
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GetFileSize: File path is empty"));
        return -1;
    }
    
    // 处理相对路径和绝对路径
    FString FullPath = FilePath;
    if (!FPaths::IsRelative(FilePath))
    {
        FullPath = FilePath;
    }
    else
    {
        FullPath = FPaths::ProjectDir() + FilePath;
    }
    
    // 规范化路径
    FPaths::NormalizeFilename(FullPath);
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (!PlatformFile.FileExists(*FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("GetFileSize: File does not exist: %s"), *FullPath);
        return -1;
    }
    
    return PlatformFile.FileSize(*FullPath);
}

bool UUtilityExtendBPLibrary::DeleteFile(const FString& FilePath, FString& OutErrorMessage)
{
    // 清空输出参数
    OutErrorMessage.Empty();
    
    if (FilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("文件路径不能为空");
        UE_LOG(LogTemp, Error, TEXT("DeleteFile: File path is empty"));
        return false;
    }
    
    // 处理相对路径和绝对路径
    FString FullPath = FilePath;
    if (!FPaths::IsRelative(FilePath))
    {
        FullPath = FilePath;
    }
    else
    {
        FullPath = FPaths::ProjectDir() + FilePath;
    }
    
    // 规范化路径
    FPaths::NormalizeFilename(FullPath);
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (!PlatformFile.FileExists(*FullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("文件不存在: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("DeleteFile: File does not exist: %s"), *FullPath);
        return false;
    }
    
    if (!PlatformFile.DeleteFile(*FullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("删除文件失败: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("DeleteFile: Failed to delete file: %s"), *FullPath);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DeleteFile: Successfully deleted file: %s"), *FullPath);
    return true;
}

bool UUtilityExtendBPLibrary::CopyFile(const FString& SourceFilePath, const FString& DestFilePath, FString& OutErrorMessage, bool bOverwrite, bool bCreateDirectories)
{
    // 清空输出参数
    OutErrorMessage.Empty();
    
    // 检查源文件路径是否为空
    if (SourceFilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("源文件路径不能为空");
        UE_LOG(LogTemp, Error, TEXT("CopyFile: Source file path is empty"));
        return false;
    }
    
    // 检查目标文件路径是否为空
    if (DestFilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("目标文件路径不能为空");
        UE_LOG(LogTemp, Error, TEXT("CopyFile: Destination file path is empty"));
        return false;
    }
    
    // 获取平台文件管理器
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // 处理源文件路径
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Original Source Path: %s"), *SourceFilePath);
    UE_LOG(LogTemp, Log, TEXT("CopyFile: IsRelative(SourceFilePath): %s"), FPaths::IsRelative(SourceFilePath) ? TEXT("true") : TEXT("false"));
    
    FString SourceFullPath;
    if (FPaths::IsRelative(SourceFilePath))
    {
        SourceFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), SourceFilePath);
        UE_LOG(LogTemp, Log, TEXT("CopyFile: Converted relative source path to: %s"), *SourceFullPath);
    }
    else
    {
        SourceFullPath = SourceFilePath;
        UE_LOG(LogTemp, Log, TEXT("CopyFile: Using absolute source path: %s"), *SourceFullPath);
    }
    FPaths::NormalizeFilename(SourceFullPath);
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Normalized source path: %s"), *SourceFullPath);
    
    // 处理目标文件路径
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Original Dest Path: %s"), *DestFilePath);
    UE_LOG(LogTemp, Log, TEXT("CopyFile: IsRelative(DestFilePath): %s"), FPaths::IsRelative(DestFilePath) ? TEXT("true") : TEXT("false"));
    
    FString DestFullPath;
    if (FPaths::IsRelative(DestFilePath))
    {
        DestFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), DestFilePath);
        UE_LOG(LogTemp, Log, TEXT("CopyFile: Converted relative dest path to: %s"), *DestFullPath);
    }
    else
    {
        DestFullPath = DestFilePath;
        UE_LOG(LogTemp, Log, TEXT("CopyFile: Using absolute dest path: %s"), *DestFullPath);
    }
    FPaths::NormalizeFilename(DestFullPath);
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Normalized dest path: %s"), *DestFullPath);
    
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Final Source: %s -> Final Destination: %s"), *SourceFullPath, *DestFullPath);
    
    // 额外的路径验证
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Project Directory: %s"), *FPaths::ProjectDir());
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Source exists check: %s"), FPlatformFileManager::Get().GetPlatformFile().FileExists(*SourceFullPath) ? TEXT("true") : TEXT("false"));
    
    // 检查源文件是否存在
    if (!PlatformFile.FileExists(*SourceFullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("源文件不存在: %s"), *SourceFullPath);
        UE_LOG(LogTemp, Error, TEXT("CopyFile: Source file does not exist: %s"), *SourceFullPath);
        return false;
    }
    
    // 检查目标文件是否已存在
    if (!bOverwrite && PlatformFile.FileExists(*DestFullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("目标文件已存在且不允许覆盖: %s"), *DestFullPath);
        UE_LOG(LogTemp, Warning, TEXT("CopyFile: Destination file exists and overwrite is disabled: %s"), *DestFullPath);
        return false;
    }
    
    // 如果需要创建目录，则创建目标文件所在的目录
    if (bCreateDirectories)
    {
        FString DestDirectory = FPaths::GetPath(DestFullPath);
        if (!DestDirectory.IsEmpty() && !PlatformFile.DirectoryExists(*DestDirectory))
        {
            if (!PlatformFile.CreateDirectoryTree(*DestDirectory))
            {
                OutErrorMessage = FString::Printf(TEXT("无法创建目标目录: %s"), *DestDirectory);
                UE_LOG(LogTemp, Error, TEXT("CopyFile: Failed to create destination directory: %s"), *DestDirectory);
                return false;
            }
            UE_LOG(LogTemp, Log, TEXT("CopyFile: Created destination directory: %s"), *DestDirectory);
        }
    }
    
    // 执行文件复制
    if (!PlatformFile.CopyFile(*DestFullPath, *SourceFullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("复制文件失败: %s -> %s"), *SourceFullPath, *DestFullPath);
        UE_LOG(LogTemp, Error, TEXT("CopyFile: Failed to copy file: %s -> %s"), *SourceFullPath, *DestFullPath);
        return false;
    }
    
    // 验证复制结果
    if (!PlatformFile.FileExists(*DestFullPath))
    {
        OutErrorMessage = FString::Printf(TEXT("复制后目标文件不存在: %s"), *DestFullPath);
        UE_LOG(LogTemp, Error, TEXT("CopyFile: Destination file does not exist after copy: %s"), *DestFullPath);
        return false;
    }
    
    // 获取源文件和目标文件大小进行验证
    int64 SourceSize = PlatformFile.FileSize(*SourceFullPath);
    int64 DestSize = PlatformFile.FileSize(*DestFullPath);
    
    if (SourceSize != DestSize)
    {
        OutErrorMessage = FString::Printf(TEXT("复制后文件大小不匹配: 源文件 %lld 字节，目标文件 %lld 字节"), SourceSize, DestSize);
        UE_LOG(LogTemp, Error, TEXT("CopyFile: File size mismatch after copy: Source %lld bytes, Destination %lld bytes"), SourceSize, DestSize);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("CopyFile: Successfully copied file: %s -> %s (%lld bytes)"), *SourceFullPath, *DestFullPath, SourceSize);
    return true;
}

FString UUtilityExtendBPLibrary::GetUtilityExtendPluginDirectory()
{
    // 获取插件管理器
    IPluginManager& PluginManager = IPluginManager::Get();
    
    // 查找UtilityExtend插件
    TSharedPtr<IPlugin> Plugin = PluginManager.FindPlugin(TEXT("UtilityExtend"));
    
    if (!Plugin.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("GetUtilityExtendPluginDirectory: UtilityExtend plugin not found"));
        return FString();
    }
    
    // 获取插件的基础目录
    FString PluginBaseDir = Plugin->GetBaseDir();
    
    // 转换为绝对路径并标准化
    FString PluginDirectory = FPaths::ConvertRelativePathToFull(PluginBaseDir);
    FPaths::NormalizeDirectoryName(PluginDirectory);
    
    UE_LOG(LogTemp, Log, TEXT("GetUtilityExtendPluginDirectory: Plugin directory: %s"), *PluginDirectory);
    
    return PluginDirectory;
}

TArray<FString> UUtilityExtendBPLibrary::OpenFileDialog(
    const FString& DialogTitle,
    const FString& DefaultPath,
    const FString& FileTypeFilter,
    bool bAllowMultipleSelection)
{
    TArray<FString> SelectedFiles;
    
    // 获取桌面平台模块
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        UE_LOG(LogTemp, Error, TEXT("OpenFileDialog: Failed to get desktop platform"));
        return SelectedFiles;
    }
    
    // 获取主窗口句柄
    const void* ParentWindowHandle = nullptr;
    if (GEngine && GEngine->GameViewport)
    {
        ParentWindowHandle = GEngine->GameViewport->GetWindow().Get()->GetNativeWindow()->GetOSWindowHandle();
    }
    
    // 设置默认路径，如果为空则使用项目目录
    FString StartDirectory = DefaultPath;
    if (StartDirectory.IsEmpty())
    {
        StartDirectory = FPaths::ProjectDir();
    }
    
    // 确保路径存在
    if (!FPaths::DirectoryExists(StartDirectory))
    {
        StartDirectory = FPaths::ProjectDir();
    }
    
    // 调用文件对话框
    bool bSuccess = false;
    if (bAllowMultipleSelection)
    {
        bSuccess = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            DialogTitle,
            StartDirectory,
            TEXT(""),
            FileTypeFilter,
            EFileDialogFlags::Multiple,
            SelectedFiles
        );
    }
    else
    {
        bSuccess = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            DialogTitle,
            StartDirectory,
            TEXT(""),
            FileTypeFilter,
            EFileDialogFlags::None,
            SelectedFiles
        );
    }
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: Successfully selected %d file(s)"), SelectedFiles.Num());
        
        // 处理返回的路径，将相对路径转换为绝对路径
        for (int32 i = 0; i < SelectedFiles.Num(); i++)
        {
            FString& FilePath = SelectedFiles[i];
            UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: Original selected file %d: %s"), i, *FilePath);
            
            // 如果是相对路径，转换为绝对路径
            if (FPaths::IsRelative(FilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: File %d is relative, converting to absolute"), i);
                
                // 对于以../开始的路径，需要基于项目目录来解析
                FString AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
                
                // 标准化路径
                FPaths::NormalizeFilename(AbsolutePath);
                
                UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: Converted file %d to absolute path: %s"), i, *AbsolutePath);
                
                // 验证文件是否存在
                if (FPaths::FileExists(AbsolutePath))
                {
                    FilePath = AbsolutePath;
                    UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: File %d exists, path updated"), i);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("OpenFileDialog: Converted path for file %d does not exist: %s"), i, *AbsolutePath);
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: File %d is already absolute: %s"), i, *FilePath);
            }
            
            UE_LOG(LogTemp, Log, TEXT("OpenFileDialog: Final selected file %d: %s"), i, *FilePath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("OpenFileDialog: User cancelled or dialog failed"));
    }
    
    return SelectedFiles;
}




// ------------------------------------实验性功能------------------------------------
// 外部软件调用相关函数实现
bool UUtilityExtendBPLibrary::LaunchExternalApplication(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchMinimized, bool bLaunchMaximized, bool bLaunchNormal)
{
    // 检查可执行文件是否存在
    if (!FPaths::FileExists(ExecutablePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Executable file not found: %s"), *ExecutablePath);
        return false;
    }

    // 确定工作目录
    FString FinalWorkingDir = WorkingDirectory;
    if (FinalWorkingDir.IsEmpty())
    {
        FinalWorkingDir = FPaths::GetPath(ExecutablePath);
    }

    // 构建启动参数
    uint32 ProcessFlags = 0;
    if (bLaunchDetached)
    {
        ProcessFlags |= PROCESS_DETACHED;
    }
    if (bLaunchHidden)
    {
        ProcessFlags |= CREATE_NO_WINDOW;
    }
    if (bLaunchMinimized)
    {
        ProcessFlags |= CREATE_MINIMIZED;
    }
    if (bLaunchMaximized)
    {
        ProcessFlags |= CREATE_MAXIMIZED;
    }
    if (bLaunchNormal)
    {
        ProcessFlags |= CREATE_NEW_CONSOLE;
    }

    // 启动进程 - 使用ShellExecute来正确处理工作目录
#if PLATFORM_WINDOWS
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = TEXT("open");
    sei.lpFile = *ExecutablePath;
    sei.lpParameters = Arguments.IsEmpty() ? NULL : *Arguments;
    sei.lpDirectory = FinalWorkingDir.IsEmpty() ? NULL : *FinalWorkingDir;
    sei.nShow = SW_SHOW;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    
    if (ShellExecuteEx(&sei))
    {
        if (sei.hProcess)
        {
            CloseHandle(sei.hProcess);
        }
        UE_LOG(LogTemp, Log, TEXT("Successfully launched external application: %s with working directory: %s"), *ExecutablePath, *FinalWorkingDir);
        return true;
    }
    else
    {
        DWORD ErrorCode = GetLastError();
        UE_LOG(LogTemp, Error, TEXT("Failed to launch external application: %s. Error code: %u"), *ExecutablePath, ErrorCode);
        return false;
    }
#else
    // 非Windows平台的简单实现
    FString Command = FString::Printf(TEXT("start \"\" \"%s\""), *ExecutablePath);
    if (!Arguments.IsEmpty())
    {
        Command = FString::Printf(TEXT("start \"\" \"%s\" %s"), *ExecutablePath, *Arguments);
    }
    
    int32 Result = system(TCHAR_TO_UTF8(*Command));
    
    if (Result == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully launched external application: %s"), *ExecutablePath);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to launch external application: %s"), *ExecutablePath);
        return false;
    }
#endif
}

bool UUtilityExtendBPLibrary::LaunchExternalApplicationWithInfo(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchMinimized, bool bLaunchMaximized, bool bLaunchNormal, FString& OutProcessID, FString& OutErrorMessage)
{
    // 检查可执行文件是否存在
    if (!FPaths::FileExists(ExecutablePath))
    {
        OutErrorMessage = TEXT("可执行文件不存在");
        OutProcessID = TEXT("");
        return false;
    }
    
    // 确定工作目录
    FString FinalWorkingDir = WorkingDirectory;
    if (FinalWorkingDir.IsEmpty())
    {
        FinalWorkingDir = FPaths::GetPath(ExecutablePath);
    }

    // 构建启动参数
    uint32 ProcessFlags = 0;
    if (bLaunchDetached)
    {
        ProcessFlags |= PROCESS_DETACHED;
    }
    if (bLaunchHidden)
    {
        ProcessFlags |= CREATE_NO_WINDOW;
    }
    if (bLaunchMinimized)
    {
        ProcessFlags |= CREATE_MINIMIZED;
    }
    if (bLaunchMaximized)
    {
        ProcessFlags |= CREATE_MAXIMIZED;
    }
    if (bLaunchNormal)
    {
        ProcessFlags |= CREATE_NEW_CONSOLE;
    }

    // 启动进程 - 使用ShellExecute来正确处理工作目录
#if PLATFORM_WINDOWS
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = TEXT("open");
    sei.lpFile = *ExecutablePath;
    sei.lpParameters = Arguments.IsEmpty() ? NULL : *Arguments;
    sei.lpDirectory = FinalWorkingDir.IsEmpty() ? NULL : *FinalWorkingDir;
    sei.nShow = SW_SHOW;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    
    if (ShellExecuteEx(&sei))
    {
        if (sei.hProcess)
        {
            // 获取进程ID
            DWORD ProcessId = GetProcessId(sei.hProcess);
            OutProcessID = FString::Printf(TEXT("%u"), ProcessId);
            CloseHandle(sei.hProcess);
        }
        else
        {
            OutProcessID = TEXT("0");
        }
        OutErrorMessage = TEXT("");
        UE_LOG(LogTemp, Log, TEXT("Successfully launched external application: %s with working directory: %s"), *ExecutablePath, *FinalWorkingDir);
        return true;
    }
    else
    {
        DWORD ErrorCode = GetLastError();
        OutProcessID = TEXT("");
        OutErrorMessage = FString::Printf(TEXT("启动失败，错误代码: %u"), ErrorCode);
        UE_LOG(LogTemp, Error, TEXT("Failed to launch external application: %s. Error code: %u"), *ExecutablePath, ErrorCode);
        return false;
    }
#else
    // 非Windows平台的简单实现
    FString Command = FString::Printf(TEXT("start \"\" \"%s\""), *ExecutablePath);
    if (!Arguments.IsEmpty())
    {
        Command = FString::Printf(TEXT("start \"\" \"%s\" %s"), *ExecutablePath, *Arguments);
    }
    
    int32 Result = system(TCHAR_TO_UTF8(*Command));
    
    if (Result == 0)
    {
        // 获取进程ID（简化实现）
        OutProcessID = TEXT("0"); // 使用system命令无法直接获取PID
        OutErrorMessage = TEXT("");
        UE_LOG(LogTemp, Log, TEXT("Successfully launched external application: %s"), *ExecutablePath);
        return true;
    }
    else
    {
        OutProcessID = TEXT("");
        OutErrorMessage = TEXT("启动进程失败");
        UE_LOG(LogTemp, Error, TEXT("Failed to launch external application: %s"), *ExecutablePath);
        return false;
    }
#endif
}

bool UUtilityExtendBPLibrary::IsExternalApplicationRunning(const FString& ProcessName)
{
#if PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            FString CurrentProcessName = FString(pe32.szExeFile);
            if (CurrentProcessName.Contains(ProcessName))
            {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return false;
#else
    // 非Windows平台的简单实现
    TArray<FString> ProcessList = GetAllRunningProcesses();
    for (const FString& Process : ProcessList)
    {
        if (Process.Contains(ProcessName))
        {
            return true;
        }
    }
        return false;
#endif
}

bool UUtilityExtendBPLibrary::TerminateExternalApplication(const FString& ProcessName)
{
#if PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            FString CurrentProcessName = FString(pe32.szExeFile);
            if (CurrentProcessName.Contains(ProcessName))
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL)
                {
                    BOOL bResult = TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    CloseHandle(hSnapshot);
                    
                    if (bResult)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Successfully terminated process: %s (PID: %u)"), *ProcessName, pe32.th32ProcessID);
                        return true;
        }
        else
        {
                        UE_LOG(LogTemp, Error, TEXT("Failed to terminate process: %s (PID: %u)"), *ProcessName, pe32.th32ProcessID);
                        return false;
                    }
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    UE_LOG(LogTemp, Warning, TEXT("Process not found: %s"), *ProcessName);
    return false;
#else
    // 非Windows平台的简单实现
    UE_LOG(LogTemp, Warning, TEXT("TerminateExternalApplication called on non-Windows platform. This function requires proper implementation with system APIs."));
    return false;
#endif
}

TArray<FString> UUtilityExtendBPLibrary::GetAllRunningProcesses()
{
    TArray<FString> ProcessList;
    
#if PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return ProcessList;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            FString ProcessName = FString(pe32.szExeFile);
            if (!ProcessName.IsEmpty())
            {
                ProcessList.Add(ProcessName);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
#else
    // 非Windows平台的简单实现
    UE_LOG(LogTemp, Warning, TEXT("GetAllRunningProcesses called on non-Windows platform. This function requires proper implementation with system APIs."));
#endif
    
    return ProcessList;
}

bool UUtilityExtendBPLibrary::WaitForExternalApplication(const FString& ProcessName, float TimeoutSeconds)
{
    // 等待外部应用程序启动
    float ElapsedTime = 0.0f;
    const float CheckInterval = 0.1f; // 每0.1秒检查一次
    
    while (ElapsedTime < TimeoutSeconds)
    {
        if (IsExternalApplicationRunning(ProcessName))
        {
            UE_LOG(LogTemp, Log, TEXT("External application %s is now running"), *ProcessName);
            return true;
        }
        
        // 等待一小段时间
        FPlatformProcess::Sleep(CheckInterval);
        ElapsedTime += CheckInterval;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Timeout waiting for external application: %s"), *ProcessName);
    return false;
}






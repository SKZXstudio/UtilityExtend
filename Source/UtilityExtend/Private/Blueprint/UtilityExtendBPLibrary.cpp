// Copyright Epic Games, Inc. All Rights Reserved.

#include "Blueprint/UtilityExtendBPLibrary.h"
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
#include "Async/Async.h"

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

void UUtilityExtendBPLibrary::ShowEditorNotification(const FString& Message, float Duration, bool bAutoExpire, bool bUseThrobber, bool bUseSuccessIcon, bool bUseFailIcon)
{
    // 检查Slate系统是否可用
    if (!FSlateApplication::IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("Slate system not initialized, cannot show notification"));
        return;
    }

    // 创建通知
    FNotificationInfo Info(FText::FromString(Message));
    Info.bUseLargeFont = false;
    Info.bUseThrobber = bUseThrobber;
    Info.bUseSuccessFailIcons = bUseSuccessIcon || bUseFailIcon;
    Info.FadeInDuration = 0.1f;
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = Duration;

    // 显示通知
    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    
    // 保存通知引用
    if (NotificationItem.IsValid())
    {
        CreatedNotifications.Add(NotificationItem);
        
        // 如果设置了自动过期，启动定时器
        if (bAutoExpire)
        {
            // 使用延迟执行而不是定时器，避免GWorld依赖
            FTimerHandle TimerHandle;
            if (UWorld* World = GWorld)
            {
                FTimerManager& TimerManager = World->GetTimerManager();
                TimerManager.SetTimer(TimerHandle, [NotificationItem]()
                {
                    if (NotificationItem.IsValid())
                    {
                        NotificationItem->ExpireAndFadeout();
                    }
                }, Duration, false);
            }
        }
    }
}

void UUtilityExtendBPLibrary::RemoveAllEditorNotifications()
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
}

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



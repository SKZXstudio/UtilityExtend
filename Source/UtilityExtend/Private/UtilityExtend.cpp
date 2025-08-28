// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtend.h"
#include "UtilityExtendCommands.h"
#include "UtilityExtendSettings.h"
#include "UtilityToolbarManager.h"
#include "UtilityExtendStyle.h"  // 添加样式系统头文件
#include "UtilityExtendIconRegistry.h"  // 添加图标注册系统头文件
#include "Persistent/UtilityExtendPersistentSettings.h"  // 添加持久化设置头文件
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Blueprint/UtilityExtendBPLibrary.h"  // 添加BPLibrary头文件
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FUtilityExtendModule"

void FUtilityExtendModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始启动模块"));
    
    // 初始化图标注册系统 - 必须在样式系统之前初始化
    // 这样可以确保图标信息在样式系统需要时已经可用
    FUtilityExtendIconRegistry::Initialize();
    
    // 初始化持久化设置 - 确保配置在需要时已经可用
    UUtilityExtendPersistentSettings::Initialize();
    UUtilityExtendPersistentSettings* PersistentSettings = UUtilityExtendPersistentSettings::Get();
    if (PersistentSettings)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置初始化成功"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 持久化设置初始化失败"));
    }
    
    // 初始化样式系统 - 必须在其他系统之前初始化
    // 这样可以确保图标资源在需要时已经可用
    FUtilityExtendStyle::Initialize();
    
    // 初始化命令
    FUtilityExtendCommands::Register();

    // 创建工具栏管理器
    ToolbarManager = NewObject<UUtilityToolbarManager>();
    if (ToolbarManager)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 工具栏管理器创建成功"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 工具栏管理器创建失败"));
    }
    
    // 创建命令列表
    PluginCommands = MakeShareable(new FUICommandList);
    
    // 注册工具栏菜单回调，延迟初始化
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUtilityExtendModule::RegisterMenus));
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 模块启动完成"));
}

void FUtilityExtendModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始关闭模块"));
    
    // 关闭持久化设置系统
    UUtilityExtendPersistentSettings::Shutdown();
    
    // 先关闭样式系统，防止其他系统继续访问样式资源
    FUtilityExtendStyle::Shutdown();
    
    // 关闭图标注册系统
    FUtilityExtendIconRegistry::Shutdown();
    
    // 注销工具栏菜单扩展
    if (UToolMenus* ToolMenus = UToolMenus::Get())
    {
        ToolMenus->UnregisterOwner(this);
    }
    
    // 安全地清理工具栏管理器
    if (ToolbarManager)
    {
        // 在关闭时不要刷新工具栏，避免访问已失效的UI系统
        // ToolbarManager->RefreshToolbar(); // 注释掉这行
        
        // 简单地将指针设置为nullptr，让垃圾回收器自然处理
        ToolbarManager = nullptr;
        
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 工具栏管理器已清理"));
    }

    // 注销命令
    FUtilityExtendCommands::Unregister();
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 模块已关闭"));
}

void FUtilityExtendModule::RegisterMenus()
{
    // 注册菜单扩展器
    FToolMenuOwnerScoped OwnerScoped(this);
    
    // 延迟初始化工具栏管理器，确保ToolMenus系统已完全初始化
    if (ToolbarManager)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始初始化工具栏"));
        ToolbarManager->Initialize();
    }
    
    // 在UE5.6中，RegisterOwner已被移除，使用OwnerScoped自动管理
    // UToolMenus::Get()->RegisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUtilityExtendModule, UtilityExtend)

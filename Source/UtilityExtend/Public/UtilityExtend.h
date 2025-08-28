// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "UtilityToolbarManager.h"

struct FToolMenuOwnerScoped;

/**
 * UtilityExtend插件模块类
 */
class FUtilityExtendModule : public IModuleInterface
{
public:
    /** 模块启动时调用 */
    virtual void StartupModule() override;
    
    /** 模块关闭时调用 */
    virtual void ShutdownModule() override;

private:
    /** 注册菜单 */
    void RegisterMenus();

    /** 工具栏管理器实例 */
    UPROPERTY()
    UUtilityToolbarManager* ToolbarManager;
    
    // 插件命令列表
    TSharedPtr<FUICommandList> PluginCommands;
};

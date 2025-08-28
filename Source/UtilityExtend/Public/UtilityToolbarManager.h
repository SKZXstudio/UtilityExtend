// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UtilityExtendSettings.h"
#include "Persistent/UtilityExtendPersistentSettings.h"
#include "UtilityToolbarManager.generated.h"

class UUtilityExtendTopBarButtonScript;
struct FToolMenuSection;
struct FToolbarButtonConfig;

/**
 * 工具栏管理器类
 * 负责根据设置创建和管理顶部工具栏按钮
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "UtilityExtend Toolbar Manager"))
class UTILITYEXTEND_API UUtilityToolbarManager : public UObject
{
    GENERATED_BODY()

public:
    UUtilityToolbarManager();
    virtual ~UUtilityToolbarManager();

    /** 初始化工具栏管理器 */
    void Initialize();

    /** 创建单个按钮 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    void CreateSingleButton(const FToolbarButtonConfig& ButtonConfig, FToolMenuSection& Section);

    /** 创建下拉按钮 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    void CreateDropdownButton(const FToolbarButtonConfig& ButtonConfig, FToolMenuSection& Section);

    /** 根据类名创建按钮实例 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    UUtilityExtendTopBarButtonScript* CreateButtonInstance(const FString& ClassName);

    /** 根据软类引用创建按钮实例 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    UUtilityExtendTopBarButtonScript* CreateButtonInstanceFromSoftClass(const TSoftClassPtr<UUtilityExtendTopBarButtonScript>& SoftClass) const;

    /** 刷新工具栏 */
    void RefreshToolbar();

    /** 获取合并的按钮配置（项目配置 + 持久化配置） */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    TArray<FToolbarButtonConfig> GetMergedButtonConfigs() const;

    /** 获取持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    TArray<FToolbarButtonConfig> GetPersistentButtonConfigs() const;

    /** 获取项目按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "Toolbar Manager")
    TArray<FToolbarButtonConfig> GetProjectButtonConfigs() const;

private:
    /** 创建工具栏按钮 */
    void CreateToolbarButtons();

    /** 处理按钮点击事件 */
    void OnButtonClicked(const FToolbarButtonConfig& ButtonConfig) const;

    /** 检查按钮是否可以执行 */
    bool CanExecuteButton(const FToolbarButtonConfig& ButtonConfig) const;

    /** 处理下拉项点击事件 */
    void OnDropdownItemClicked(UUtilityExtendTopBarButtonScript* ButtonScript) const;
    
    // 创建下拉菜单
    void CreateDropdownMenu(UToolMenu* Menu, const FToolbarButtonConfig& ButtonConfig) const;

    /** 获取按钮图标 */
    FSlateIcon GetButtonIcon(const FToolbarButtonConfig& ButtonConfig);

    /** 获取按钮工具提示 */
    FString GetButtonTooltip(const FToolbarButtonConfig& ButtonConfig);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/Engine.h"
#include "UtilityExtendSettings.h"
#include "Interfaces/IPluginManager.h"
#include "UtilityExtendPersistentSettings.generated.h"

/**
 * UtilityExtend插件的持久化配置设置
 * 这些配置只从插件内部的配置文件中读取，真正实现跨项目持久化
 * 不会在项目Config目录中创建文件，确保配置随插件一起分发
 * 这是一个纯数据类，只负责从插件配置文件读取数据
 */
UCLASS(Config=UtilityExtendPersistent, DefaultConfig, BlueprintType, meta = (DisplayName = "UtilityExtend 持久化设置"))
class UTILITYEXTEND_API UUtilityExtendPersistentSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UUtilityExtendPersistentSettings();
    virtual ~UUtilityExtendPersistentSettings();

    /** 获取持久化配置实例（单例模式） */
    static UUtilityExtendPersistentSettings* Get();

    /** 初始化持久化设置系统 */
    static void Initialize();

    /** 清理持久化设置系统 */
    static void Shutdown();



    /** 获取持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    const TArray<FToolbarButtonConfig>& GetPersistentButtonConfigs() const { return PersistentButtonConfigs; }

    /** 设置持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    void SetPersistentButtonConfigs(const TArray<FToolbarButtonConfig>& NewConfigs);

    /** 添加持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    void AddPersistentButtonConfig(const FToolbarButtonConfig& Config);

    /** 移除持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    void RemovePersistentButtonConfig(int32 Index);

    /** 清空所有持久化按钮配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    void ClearPersistentButtonConfigs();

    /**
     * ===========================================
     * 图标配置区域 - 在此处添加您的图标配置代码
     * ===========================================
     * 
     * 如需添加持久化图标配置，请在此处添加相关函数：
     * 1. 添加图标配置数据结构
     * 2. 实现图标加载和管理功能
     * 3. 在 InitializeDefaultConfig() 中设置默认图标
     * 
     * 示例：
     * void AddPersistentIcon(const FString& IconName, const FString& IconPath);
     * FString GetPersistentIconPath(const FString& IconName) const;
     */

    /** 从插件配置文件加载配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    bool LoadFromPluginConfig();

    /** 保存配置到插件配置文件 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    bool SaveToPluginConfig();

    /** 重新加载配置 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    void ReloadConfig();

    /** 获取持久化图标名称列表（仅供持久化设置使用） */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    TArray<FName> GetPersistentIconNames() const;

    /** 获取所有可用图标名称列表（持久化设置版本 - 仅返回持久化图标） */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    TArray<FName> GetAllAvailableIconNames() const;

    /** 验证图标是否为持久化图标 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    bool IsValidPersistentIcon(const FName& IconName) const;

    /** 根据友好名称获取图标技术名称 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    FName GetIconNameFromDisplayName(const FString& DisplayName) const;

    /** 根据技术名称获取图标友好名称 */
    UFUNCTION(BlueprintCallable, Category = "UtilityExtend|Persistent")
    FString GetIconDisplayNameFromName(const FName& IconName) const;

protected:
    /** 持久化按钮配置数组 - 从插件配置文件中读取 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Button Configs", 
               meta = (DisplayName = "持久化按钮配置", 
                      ToolTip = "从插件配置文件中读取的持久化工具栏按钮配置。注意：这些设置仅用于显示，不会保存到项目配置文件。"))
    TArray<FToolbarButtonConfig> PersistentButtonConfigs;

    /** 插件配置文件路径 */
    FString PluginConfigPath;

    /** 延迟保存定时器句柄 */
    FTimerHandle DelayedSaveTimerHandle;

    /** 保存延迟时间（秒） */
    static constexpr float SaveDelayTime = 0.5f;

public:
    // UDeveloperSettings 接口
    virtual FName GetCategoryName() const override;
    virtual void PostInitProperties() override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:


    /** 解析JSON配置文件 */
    bool ParseJsonConfigFile();

    /** 获取插件根目录路径 */
    FString GetPluginDirectory() const;
    
    /** 从配置字符串解析按钮配置 */
    bool ParseButtonConfigFromString(const FString& ConfigString, FToolbarButtonConfig& OutConfig);
    
    /** 从字符串解析下拉项配置 */
    bool ParseDropdownItemsFromString(const FString& DropdownString, TArray<FToolbarDropdownItem>& OutItems);
    
    /** 解析单个下拉项 */
    bool ParseSingleDropdownItem(const FString& ItemString, FToolbarDropdownItem& OutItem);
    
    /** 解析顶层参数（处理嵌套括号） */
    void ParseTopLevelParameters(const FString& ParameterString, TArray<FString>& OutParameters);
    
    /** 延迟保存配置（避免频繁保存导致的数据丢失） */
    void ScheduleDelayedSave();
    
    /** 验证配置数据的完整性 */
    bool ValidateConfigData() const;
};

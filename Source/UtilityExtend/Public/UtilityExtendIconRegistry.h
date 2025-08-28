// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "UtilityExtendSettings.h"

/**
 * UtilityExtend插件的SVG图标注册管理类
 * 负责统一管理插件的所有SVG图标资源，包括插件内置图标和项目配置图标
 * 
 * 注意：图标配置仅通过配置文件编辑，不再提供UI界面设置
 */
class FUtilityExtendIconRegistry
{
public:
    /** 初始化图标注册系统 */
    static void Initialize();

    /** 关闭图标注册系统 */
    static void Shutdown();



    /** 获取所有可用的图标名称列表 */
    static TArray<FName> GetAllAvailableIconNames();

    /** 获取所有可用的图标信息 */
    static TArray<FToolbarIconInfo> GetAllAvailableIconInfos();

    /** 根据友好名称获取技术名称 */
    static FName GetIconNameFromDisplayName(const FString& DisplayName);

    /** 根据技术名称获取友好名称 */
    static FString GetIconDisplayNameFromName(const FName& IconName);

    /** 检查图标名称是否有效 */
    static bool IsValidIconName(const FName& IconName);

    /** 获取默认按钮图标名称 */
    static FName GetDefaultButtonIconName();

    /** 获取下拉按钮图标名称 */
    static FName GetDropdownButtonIconName();



private:
    /** 创建统一图标集 */
    static void CreateIconSet(TArray<FToolbarIconInfo>& IconInfos);

private:
    /** 图标注册系统是否已初始化 */
    static bool bIsInitialized;

    /** 所有可用图标信息缓存 */
    static TArray<FToolbarIconInfo> CachedIconInfos;

    /** 标准图标尺寸 */
    static const FVector2D Icon16x16;
    static const FVector2D Icon20x20;
    static const FVector2D Icon24x24;
};

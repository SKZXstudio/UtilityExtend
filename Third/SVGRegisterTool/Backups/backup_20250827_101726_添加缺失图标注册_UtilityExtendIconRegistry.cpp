// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtendIconRegistry.h"
#include "UtilityExtendSettings.h"
#include "Styling/SlateStyleMacros.h"
#include "Misc/MessageDialog.h"
#include "Styling/SlateStyle.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"

// 静态成员初始化
bool FUtilityExtendIconRegistry::bIsInitialized = false;
TArray<FToolbarIconInfo> FUtilityExtendIconRegistry::CachedIconInfos;

// 标准图标尺寸
const FVector2D FUtilityExtendIconRegistry::Icon16x16(16.0f, 16.0f);
const FVector2D FUtilityExtendIconRegistry::Icon20x20(20.0f, 20.0f);
const FVector2D FUtilityExtendIconRegistry::Icon24x24(24.0f, 24.0f);

void FUtilityExtendIconRegistry::Initialize()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始初始化图标注册系统"));
        
        // 创建统一的图标信息缓存
        CreateIconSet(CachedIconInfos);
        
        bIsInitialized = true;
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 图标注册系统初始化完成，共 %d 个图标"), CachedIconInfos.Num());
    }
}

void FUtilityExtendIconRegistry::Shutdown()
{
    if (bIsInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始关闭图标注册系统"));
        
        CachedIconInfos.Empty();
        bIsInitialized = false;
        
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 图标注册系统已关闭"));
    }
}



TArray<FName> FUtilityExtendIconRegistry::GetAllAvailableIconNames()
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    TArray<FName> IconNames;
    for (const FToolbarIconInfo& IconInfo : CachedIconInfos)
    {
        IconNames.Add(FName(*IconInfo.DisplayName));
    }
    
    return IconNames;
}

TArray<FToolbarIconInfo> FUtilityExtendIconRegistry::GetAllAvailableIconInfos()
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    return CachedIconInfos;
}

FName FUtilityExtendIconRegistry::GetIconNameFromDisplayName(const FString& DisplayName)
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    for (const FToolbarIconInfo& IconInfo : CachedIconInfos)
    {
        if (IconInfo.DisplayName.Equals(DisplayName))
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 找到图标名称映射: %s -> %s"), *DisplayName, *IconInfo.IconName.ToString());
            return IconInfo.IconName;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 未找到图标名称映射: %s，使用默认图标"), *DisplayName);
    return GetDefaultButtonIconName();
}

FString FUtilityExtendIconRegistry::GetIconDisplayNameFromName(const FName& IconName)
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    for (const FToolbarIconInfo& IconInfo : CachedIconInfos)
    {
        if (IconInfo.IconName == IconName)
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 找到图标友好名称映射: %s -> %s"), *IconName.ToString(), *IconInfo.DisplayName);
            return IconInfo.DisplayName;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 未找到图标友好名称映射: %s，返回技术名称"), *IconName.ToString());
    return IconName.ToString();
}

bool FUtilityExtendIconRegistry::IsValidIconName(const FName& IconName)
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    for (const FToolbarIconInfo& IconInfo : CachedIconInfos)
    {
        if (IconInfo.IconName == IconName)
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 图标名称有效: %s"), *IconName.ToString());
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无效的图标名称: %s"), *IconName.ToString());
    return false;
}

FName FUtilityExtendIconRegistry::GetDefaultButtonIconName()
{
    static FName DefaultIconName(TEXT("UtilityExtend.BtnIcon"));
    return DefaultIconName;
}

FName FUtilityExtendIconRegistry::GetDropdownButtonIconName()
{
    static FName DropdownIconName(TEXT("UtilityExtend.DropdownButtonIcon"));
    return DropdownIconName;
}



void FUtilityExtendIconRegistry::CreateIconSet(TArray<FToolbarIconInfo>& IconInfos)
{
    // ============================================================================
    // 插件内置图标集 - 仅插件Resources目录下的图标
    // ============================================================================
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始创建插件内置图标集"));
    
    // 基础默认图标
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.BtnIcon"), TEXT("默认图标"), TEXT("通用按钮图标，适用于大多数按钮")));
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.ToolBox"), TEXT("工具箱"), TEXT("工具箱功能图标")));
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.nexbox"), TEXT("NEXBox图标"), TEXT("NEXBOX图标")));
    
    // ============================================================================
    // 注意：所有图标都位于插件的Resources目录中
    // 高级用户可以将自定义SVG图标放置到插件Resources目录并修改此代码进行注册
    // 普通用户只能使用已注册的图标
    // ============================================================================
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 图标集创建完成，共 %d 个图标"), IconInfos.Num());
}





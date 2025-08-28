// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtendIconRegistry.h"
#include "UtilityExtendSettings.h"
#include "Styling/SlateStyleMacros.h"
#include "Misc/MessageDialog.h"
#include "Styling/SlateStyle.h"

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
        
        // 创建图标信息缓存
        CreateBaseIconSet(CachedIconInfos);
        CreateUserCustomIconSet(CachedIconInfos);
        
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

void FUtilityExtendIconRegistry::CreateBaseIconSet(TArray<FToolbarIconInfo>& IconInfos)
{
    // ============================================================================
    // 基础图标集 - 插件内置的核心图标
    // ============================================================================
    
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.BtnIcon"), TEXT("默认图标"), TEXT("通用按钮图标，适用于大多数按钮")));
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.ToolBox"), TEXT("工具箱"), TEXT("工具箱功能图标")));
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 基础图标集创建完成，共 %d 个图标"), IconInfos.Num());
}

void FUtilityExtendIconRegistry::CreateUserCustomIconSet(TArray<FToolbarIconInfo>& IconInfos)
{
    // ============================================================================
    // 用户自定义区域
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.NEXIcon"), TEXT("NEX图标"), TEXT("用户自定义图标")));
    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.Inno"), TEXT("Inno"), TEXT("用户自定义图标")));
    // ============================================================================
    // 在这里继续添加代码

    // ============================================================================
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 用户自定义图标集创建完成，当前总图标数: %d"), IconInfos.Num());
}



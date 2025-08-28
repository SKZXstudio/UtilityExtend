// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtendStyle.h"
#include "UtilityExtend.h"
#include "UtilityExtendIconRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir



TSharedPtr<FSlateStyleSet> FUtilityExtendStyle::StyleInstance = nullptr;

void FUtilityExtendStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 样式系统初始化完成"));
    }
}

void FUtilityExtendStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始关闭样式系统"));
        
        // 在关闭样式系统之前，先检查Slate系统是否仍然有效
        if (FSlateApplication::IsInitialized())
        {
            FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: Slate系统已关闭，跳过样式注销"));
        }
        
        // 确保样式实例是唯一的，然后安全地重置
        if (StyleInstance.IsUnique())
        {
            StyleInstance.Reset();
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 样式系统已安全关闭"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 样式实例仍被其他对象引用，无法安全关闭"));
            // 强制重置，但记录警告
            StyleInstance.Reset();
        }
    }
}

FName FUtilityExtendStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("UtilityExtendStyle"));
    return StyleSetName;
}

FName FUtilityExtendStyle::GetDefaultButtonIconName()
{
    static FName DefaultIconName(TEXT("UtilityExtend.DefaultButtonIcon"));
    return DefaultIconName;
}

FName FUtilityExtendStyle::GetDropdownButtonIconName()
{
    static FName DropdownIconName(TEXT("UtilityExtend.DropdownButtonIcon"));
    return DropdownIconName;
}

TSharedRef< FSlateStyleSet > FUtilityExtendStyle::Create()
{
    TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("UtilityExtendStyle"));
    
    // 设置资源根目录
    if (IPluginManager::Get().FindPlugin("UtilityExtend"))
    {
        Style->SetContentRoot(IPluginManager::Get().FindPlugin("UtilityExtend")->GetBaseDir() / TEXT("Resources"));
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 设置资源根目录: %s"), *Style->GetContentRootDir());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法找到插件管理器"));
    }
    
    // 注册所有可用的图标（统一使用20x20尺寸）
    Style->Set("UtilityExtend.BtnIcon", new IMAGE_BRUSH_SVG(TEXT("BtnIcon"), FVector2D(20.0f, 20.0f)));
    Style->Set("UtilityExtend.ToolBox", new IMAGE_BRUSH_SVG(TEXT("ToolBox"), FVector2D(20.0f, 20.0f)));
    
    // ============================================================================
    // 用户自定义区域
    Style->Set("UtilityExtend.NEXIcon", new IMAGE_BRUSH_SVG(TEXT("NEXIcon"), FVector2D(20.0f, 20.0f)));
    // ============================================================================
    // 在这里继续添加代码

    // ============================================================================
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 样式集创建完成"));
    
    return Style;
}

void FUtilityExtendStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 纹理资源重新加载完成"));
    }
}

const ISlateStyle& FUtilityExtendStyle::Get()
{
    if (!StyleInstance.IsValid())
    {
        Initialize();
    }
    return *StyleInstance;
}

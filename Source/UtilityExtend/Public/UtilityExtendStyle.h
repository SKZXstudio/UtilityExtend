// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"

/**
 * UtilityExtend插件的样式管理类
 * 负责注册和管理插件的SVG图标资源
 */
class FUtilityExtendStyle
{
public:
    /** 初始化样式系统 */
    static void Initialize();

    /** 关闭样式系统 */
    static void Shutdown();

    /** 重新加载纹理资源 */
    static void ReloadTextures();

    /** 获取样式集实例 */
    static const ISlateStyle& Get();

    /** 获取样式集名称 */
    static FName GetStyleSetName();

    /** 获取默认按钮图标名称 */
    static FName GetDefaultButtonIconName();

    /** 获取下拉按钮图标名称 */
    static FName GetDropdownButtonIconName();



private:
    /** 创建样式集 */
    static TSharedRef< class FSlateStyleSet > Create();

private:
    /** 样式集实例 */
    static TSharedPtr< class FSlateStyleSet > StyleInstance;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "UtilityExtendTopBarButtonScript.generated.h"

/**
 * 标准工具栏按钮脚本类
 * 用户可以在蓝图中继承此类来创建自定义按钮功能
 * 继承自UEditorUtilityObject，拥有编辑器工具对象的能力
 * 
 * 使用方法：
 * 1. 在蓝图中继承此类
 * 2. 重写 OnButtonClicked 函数
 * 3. 在项目设置中配置按钮
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "UtilityExtend Top Bar Button Script"))
class UTILITYEXTEND_API UUtilityExtendTopBarButtonScript : public UEditorUtilityObject
{
    GENERATED_BODY()

public:
    UUtilityExtendTopBarButtonScript();

    /**
     * 按钮被点击时调用的函数
     * 在蓝图中重写此函数来实现按钮功能
     * 
     * 示例用法：
     * - 打开窗口
     * - 执行编辑器命令
     * - 运行工具脚本
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Button Actions")
    void OnButtonClicked();
    virtual void OnButtonClicked_Implementation();
};

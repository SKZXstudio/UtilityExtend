// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Blueprint/ButtonScript/UtilityExtendTopBarButtonScript.h"
#include "UtilityExtendSettings.generated.h"

/**
 * 工具栏图标信息结构体
 */
USTRUCT(BlueprintType)
struct FToolbarIconInfo
{
    GENERATED_BODY()

    /** 图标技术名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Info")
    FName IconName;

    /** 图标友好显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Info")
    FString DisplayName;

    /** 图标描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Info")
    FString Description;

    FToolbarIconInfo()
        : IconName(NAME_None)
        , DisplayName(TEXT(""))
        , Description(TEXT(""))
    {
    }

    FToolbarIconInfo(const FName& InIconName, const FString& InDisplayName, const FString& InDescription)
        : IconName(InIconName)
        , DisplayName(InDisplayName)
        , Description(InDescription)
    {
    }
};

/**
 * 工具栏按钮类型枚举
 */
UENUM(BlueprintType)
enum class EToolbarButtonType : uint8
{
    /** 单个按钮 */
    SingleButton UMETA(DisplayName = "单个按钮"),
    /** 下拉按钮 */
    DropdownButton UMETA(DisplayName = "下拉按钮")
};

/**
 * 下拉列表项配置
 */
USTRUCT(BlueprintType)
struct FToolbarDropdownItem
{
    GENERATED_BODY()

    /** 下拉项名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropdown Item")
    FString ItemName;

    /** 绑定的脚本类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropdown Item", meta = (AllowedClasses = "UtilityExtendTopBarButtonScript"))
    TSoftClassPtr<UUtilityExtendTopBarButtonScript> BoundClass;

    FToolbarDropdownItem()
        : ItemName(TEXT(""))
        , BoundClass(nullptr)
    {
    }

    /** 自定义序列化支持，确保BoundClass正确保存和加载 */
    bool Serialize(FArchive& Ar);
    
    /** 配置导出支持 - 确保在项目设置中正确显示和保存 */
    bool ExportTextItem(FString& ValueStr, FToolbarDropdownItem const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
    bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
};

// 告诉UE这个结构体有自定义序列化
template<>
struct TStructOpsTypeTraits<FToolbarDropdownItem> : public TStructOpsTypeTraitsBase2<FToolbarDropdownItem>
{
    enum
    {
        WithSerializer = true,
        WithImportTextItem = true,
        WithExportTextItem = true,
    };
};

/**
 * 工具栏按钮配置
 */
USTRUCT(BlueprintType)
struct FToolbarButtonConfig
{
    GENERATED_BODY()

    /** 按钮名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Config")
    FString ButtonName;

    /** 按钮类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Config")
    EToolbarButtonType ButtonType;

    /** 绑定的脚本类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Config", meta = (AllowedClasses = "UtilityExtendTopBarButtonScript"))
    TSoftClassPtr<UUtilityExtendTopBarButtonScript> BoundClass;

    /** 按钮图标名称（通过配置文件编辑） */
    UPROPERTY(config, BlueprintReadWrite, Category = "Button Config",
    //这一行是让图标可以在编辑器中编辑
    //UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Button Config", 
               meta = (DisplayName = "按钮图标", 
                      ToolTip = "图标名称，需要在配置文件中手动编辑"))
    FName ButtonIconName;

    /** 下拉列表项（仅当ButtonType为DropdownButton时有效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Config", meta = (EditCondition = "ButtonType == EToolbarButtonType::DropdownButton"))
    TArray<FToolbarDropdownItem> DropdownItems;

    /** 是否显示按钮文本标签 */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Button Config", 
               meta = (DisplayName = "显示按钮文本", 
                      ToolTip = "是否在工具栏按钮上显示文本标签"))
    bool bShowButtonText;

    FToolbarButtonConfig()
        : ButtonName(TEXT(""))
        , ButtonType(EToolbarButtonType::SingleButton)
        , BoundClass(nullptr)
        , bShowButtonText(true)
    {
    }
};

/**
 * UtilityExtend插件设置类
 * 在项目设置中可以配置工具栏按钮
 * 
 * 注意：图标设置已移除UI界面，需要通过配置文件手动编辑
 */
UCLASS(config = UtilityExtend, defaultconfig, meta = (DisplayName = "UtilityExtend 设置", Category = "Plugins", ConfigGroupName = "/Script/UtilityExtend.UtilityExtendSettings"))
class UTILITYEXTEND_API UUtilityExtendSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UUtilityExtendSettings();

    /** 工具栏按钮配置数组 */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Toolbar Buttons", 
               meta = (DisplayName = "工具栏按钮配置", 
                      ToolTip = "配置要在顶部工具栏中显示的按钮"))
    TArray<FToolbarButtonConfig> ToolbarButtonConfigs;

    /** 获取设置实例 */
    static UUtilityExtendSettings* Get();

    /** 获取设置类别名称 */
    virtual FName GetCategoryName() const override;

    /** 获取设置显示名称 */
    virtual FText GetDisplayName() const;
    



};

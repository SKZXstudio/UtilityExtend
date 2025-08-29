// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityToolbarManager.h"
#include "Blueprint/ButtonScript/UtilityExtendTopBarButtonScript.h"
#include "UtilityExtendSettings.h"
#include "UtilityExtendCommands.h"
#include "UtilityExtendStyle.h"  // 添加样式系统头文件
#include "UtilityExtendIconRegistry.h"  // 添加图标注册系统头文件
#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "EditorStyleSet.h"
#include "Slate.h"

UUtilityToolbarManager::UUtilityToolbarManager()
{
    // 构造函数
}

UUtilityToolbarManager::~UUtilityToolbarManager()
{
    // 析构函数 - 安全地清理资源
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: ToolbarManager 析构函数被调用"));
    
    // 在析构时不要尝试刷新UI，因为此时UI系统可能已经关闭
    // 只记录日志，让系统自然清理
}

void UUtilityToolbarManager::Initialize()
{
    // 创建工具栏按钮
    CreateToolbarButtons();
}

void UUtilityToolbarManager::CreateToolbarButtons()
{
    // 获取合并的按钮配置（项目配置 + 持久化配置）
    TArray<FToolbarButtonConfig> MergedConfigs = GetMergedButtonConfigs();
    
    if (MergedConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 没有找到任何按钮配置"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 找到 %d 个合并的按钮配置"), MergedConfigs.Num());

    // 检查ToolMenus系统是否有效
    UToolMenus* ToolMenus = UToolMenus::Get();
    if (!ToolMenus)
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: ToolMenus系统不可用"));
        return;
    }

    // 尝试多个可能的工具栏路径
    TArray<FString> ToolbarPaths = {
        "LevelEditor.LevelEditorToolBar.PlayToolBar",
        "LevelEditor.LevelEditorToolBar.MainToolBar",
        "LevelEditor.LevelEditorToolBar"
    };

    UToolMenu* ToolbarMenu = nullptr;
    for (const FString& Path : ToolbarPaths)
    {
        ToolbarMenu = ToolMenus->ExtendMenu(FName(*Path));
        if (ToolbarMenu)
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 成功扩展工具栏: %s"), *Path);
            break;
        }
    }

    if (!ToolbarMenu)
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法扩展任何工具栏菜单"));
        return;
    }

    // 找到或创建PluginTools部分
    FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");

    // 遍历配置创建按钮
    for (const FToolbarButtonConfig& ButtonConfig : MergedConfigs)
    {
        if (ButtonConfig.ButtonName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 跳过空名称的按钮配置"));
            continue;
        }

        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 创建按钮: %s"), *ButtonConfig.ButtonName);
        
        if (ButtonConfig.ButtonType == EToolbarButtonType::SingleButton)
        {
            CreateSingleButton(ButtonConfig, Section);
        }
        else if (ButtonConfig.ButtonType == EToolbarButtonType::DropdownButton)
        {
            CreateDropdownButton(ButtonConfig, Section);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 按钮创建完成"));
}

void UUtilityToolbarManager::CreateSingleButton(const FToolbarButtonConfig& ButtonConfig, FToolMenuSection& Section)
{
    // 创建按钮操作
    FUIAction ButtonAction;
    
    // 使用lambda函数来包装带参数的函数调用
    ButtonAction.ExecuteAction = FExecuteAction::CreateLambda([this, ButtonConfig]()
    {
        this->OnButtonClicked(ButtonConfig);
    });
    
    ButtonAction.CanExecuteAction = FCanExecuteAction::CreateLambda([this, ButtonConfig]()
    {
        return this->CanExecuteButton(ButtonConfig);
    });

    // 创建单个按钮
    FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
        FName(*ButtonConfig.ButtonName),
        FToolUIActionChoice(ButtonAction),
        FText::FromString(ButtonConfig.ButtonName),
        FText::FromString(GetButtonTooltip(ButtonConfig)),
        GetButtonIcon(ButtonConfig)
    );

    // 如果配置了显示按钮文本，则设置样式覆盖
    if (ButtonConfig.bShowButtonText)
    {
        Entry.StyleNameOverride = "CalloutToolbar";
    }

    Section.AddEntry(Entry);
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 创建单个按钮: %s"), *ButtonConfig.ButtonName);
}

void UUtilityToolbarManager::CreateDropdownButton(const FToolbarButtonConfig& ButtonConfig, FToolMenuSection& Section)
{
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始创建下拉按钮: %s, 下拉项数量: %d"), 
           *ButtonConfig.ButtonName, ButtonConfig.DropdownItems.Num());
    
    // 验证下拉项数据
    for (int32 i = 0; i < ButtonConfig.DropdownItems.Num(); ++i)
    {
        const FToolbarDropdownItem& Item = ButtonConfig.DropdownItems[i];
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉按钮[%s]下拉项[%d] - 名称: %s, 类: %s"), 
               *ButtonConfig.ButtonName, i, *Item.ItemName, *Item.BoundClass.ToString());
    }
    
    // 创建下拉按钮
    FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
        FName(*ButtonConfig.ButtonName),
        FToolUIActionChoice(), // 空操作
        FNewToolMenuDelegate::CreateLambda([this, ButtonConfig](UToolMenu* Menu)
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: Lambda中创建下拉菜单: %s, 下拉项数量: %d"), 
                   *ButtonConfig.ButtonName, ButtonConfig.DropdownItems.Num());
            this->CreateDropdownMenu(Menu, ButtonConfig);
        }),
        FText::FromString(ButtonConfig.ButtonName),
        FText::FromString(GetButtonTooltip(ButtonConfig)),
        GetButtonIcon(ButtonConfig)
    );

    // 如果配置了显示按钮文本，则设置样式覆盖
    if (ButtonConfig.bShowButtonText)
    {
        Entry.StyleNameOverride = "CalloutToolbar";
    }

    Section.AddEntry(Entry);
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 创建下拉按钮完成: %s"), *ButtonConfig.ButtonName);
}

UUtilityExtendTopBarButtonScript* UUtilityToolbarManager::CreateButtonInstance(const FString& ClassName)
{
    // 根据类名创建按钮实例
    UClass* Class = LoadClass<UUtilityExtendTopBarButtonScript>(nullptr, *ClassName);
    if (Class)
    {
        return NewObject<UUtilityExtendTopBarButtonScript>(GetTransientPackage(), Class);
    }
    return nullptr;
}

UUtilityExtendTopBarButtonScript* UUtilityToolbarManager::CreateButtonInstanceFromSoftClass(const TSoftClassPtr<UUtilityExtendTopBarButtonScript>& SoftClass) const
{
    // 根据软类引用创建按钮实例
    if (UClass* Class = SoftClass.LoadSynchronous())
    {
        if (Class->IsChildOf<UUtilityExtendTopBarButtonScript>())
        {
            return NewObject<UUtilityExtendTopBarButtonScript>(GetTransientPackage(), Class);
        }
    }
    return nullptr;
}

void UUtilityToolbarManager::RefreshToolbar()
{
    // 刷新工具栏 - 添加安全检查
    if (UToolMenus* ToolMenus = UToolMenus::Get())
    {
        // 检查ToolMenus系统是否仍然有效 - 使用IsValid()检查UObject的有效性
        if (IsValid(ToolMenus))
        {
            ToolMenus->RefreshAllWidgets();
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 工具栏刷新完成"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: ToolMenus对象无效，跳过工具栏刷新"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法获取ToolMenus实例，跳过工具栏刷新"));
    }
}

void UUtilityToolbarManager::OnButtonClicked(const FToolbarButtonConfig& ButtonConfig) const
{
    // 处理按钮点击事件
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 按钮被点击: %s"), *ButtonConfig.ButtonName);
    
    // 创建按钮实例并执行
    if (UUtilityExtendTopBarButtonScript* ButtonScript = CreateButtonInstanceFromSoftClass(ButtonConfig.BoundClass))
    {
        ButtonScript->OnButtonClicked();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法创建按钮实例: %s"), *ButtonConfig.ButtonName);
    }
}

bool UUtilityToolbarManager::CanExecuteButton(const FToolbarButtonConfig& ButtonConfig) const
{
    // 检查按钮是否可以执行
    if (UUtilityExtendTopBarButtonScript* ButtonScript = CreateButtonInstanceFromSoftClass(ButtonConfig.BoundClass))
    {
        // 按钮始终可以执行，除非脚本类无效
        return true;
    }
    return false;
}

void UUtilityToolbarManager::OnDropdownItemClicked(UUtilityExtendTopBarButtonScript* ButtonScript) const
{
    // 处理下拉项点击事件
    if (ButtonScript)
    {
        ButtonScript->OnButtonClicked();
    }
}

void UUtilityToolbarManager::CreateDropdownMenu(UToolMenu* Menu, const FToolbarButtonConfig& ButtonConfig) const
{
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 创建下拉菜单开始: %s, 下拉项数量: %d"), 
           *ButtonConfig.ButtonName, ButtonConfig.DropdownItems.Num());
    
    // 创建下拉菜单部分
    FToolMenuSection& Section = Menu->FindOrAddSection("DropdownItems");
    
    // 添加下拉项
    for (int32 i = 0; i < ButtonConfig.DropdownItems.Num(); ++i)
    {
        const FToolbarDropdownItem& DropdownItem = ButtonConfig.DropdownItems[i];
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 创建下拉项[%d]: 名称='%s', 类='%s'"), 
               i, *DropdownItem.ItemName, *DropdownItem.BoundClass.ToString());
        
        if (DropdownItem.ItemName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 跳过空名称的下拉项[%d]"), i);
            continue;
        }
        
        // 创建下拉项操作
        FUIAction ItemAction;
        ItemAction.ExecuteAction = FExecuteAction::CreateLambda([this, DropdownItem]()
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉项被点击: %s"), *DropdownItem.ItemName);
            if (UUtilityExtendTopBarButtonScript* ButtonScript = CreateButtonInstanceFromSoftClass(DropdownItem.BoundClass))
            {
                this->OnDropdownItemClicked(ButtonScript);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法创建下拉项实例: %s"), *DropdownItem.ItemName);
            }
        });
        
        FToolMenuEntry MenuEntry = FToolMenuEntry::InitMenuEntry(
            FName(*DropdownItem.ItemName),
            FText::FromString(DropdownItem.ItemName),
            FText::FromString(DropdownItem.ItemName),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Placeholder"),
            FToolUIActionChoice(ItemAction),
            EUserInterfaceActionType::Button
        );
        
        Section.AddEntry(MenuEntry);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉项创建成功: %s"), *DropdownItem.ItemName);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉菜单创建完成: %s"), *ButtonConfig.ButtonName);
}

FSlateIcon UUtilityToolbarManager::GetButtonIcon(const FToolbarButtonConfig& ButtonConfig)
{
    // 如果用户指定了图标名称，使用用户选择的图标
    if (!ButtonConfig.ButtonIconName.IsNone())
    {
        // 将友好名称转换为技术名称
        FName TechnicalIconName = FUtilityExtendIconRegistry::GetIconNameFromDisplayName(ButtonConfig.ButtonIconName.ToString());
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 按钮 %s 友好名称: %s, 技术名称: %s"), *ButtonConfig.ButtonName, *ButtonConfig.ButtonIconName.ToString(), *TechnicalIconName.ToString());
        
        // 使用转换后的技术名称从样式系统获取图标
        return FSlateIcon(FUtilityExtendStyle::GetStyleSetName(), TechnicalIconName);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 按钮 %s 未指定图标，使用第一个可用图标"), *ButtonConfig.ButtonName);
    
    // 如果没有指定图标，使用第一个可用的图标
    return FSlateIcon(FUtilityExtendStyle::GetStyleSetName(), TEXT("UtilityExtend.BtnIcon"));
}

FString UUtilityToolbarManager::GetButtonTooltip(const FToolbarButtonConfig& ButtonConfig)
{
    // 返回按钮工具提示
    return ButtonConfig.ButtonName;
}

TArray<FToolbarButtonConfig> UUtilityToolbarManager::GetMergedButtonConfigs() const
{
    TArray<FToolbarButtonConfig> MergedConfigs;
    
    // 首先添加持久化配置（优先级更高）
    UUtilityExtendPersistentSettings* PersistentSettings = UUtilityExtendPersistentSettings::Get();
    if (PersistentSettings)
    {
        const TArray<FToolbarButtonConfig>& PersistentConfigs = PersistentSettings->GetPersistentButtonConfigs();
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 获取到 %d 个持久化按钮配置"), PersistentConfigs.Num());
        
        // 逐个添加配置并验证数据完整性
        for (int32 i = 0; i < PersistentConfigs.Num(); ++i)
        {
            const FToolbarButtonConfig& Config = PersistentConfigs[i];
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化配置[%d] - 名称: %s, 类型: %d, 下拉项数量: %d"), 
                   i, *Config.ButtonName, (int32)Config.ButtonType, Config.DropdownItems.Num());
            
            // 验证下拉项数据
            for (int32 j = 0; j < Config.DropdownItems.Num(); ++j)
            {
                const FToolbarDropdownItem& Item = Config.DropdownItems[j];
                UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化配置[%d]下拉项[%d] - 名称: %s, 类: %s"), 
                       i, j, *Item.ItemName, *Item.BoundClass.ToString());
            }
            
            // 添加到合并配置中
            MergedConfigs.Add(Config);
        }
        
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 添加了 %d 个持久化按钮配置"), PersistentConfigs.Num());
    }
    
    // 然后添加项目配置
    UUtilityExtendSettings* ProjectSettings = UUtilityExtendSettings::Get();
    if (ProjectSettings)
    {
        TArray<FToolbarButtonConfig> ProjectConfigs = ProjectSettings->ToolbarButtonConfigs;
        MergedConfigs.Append(ProjectConfigs);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 添加了 %d 个项目按钮配置"), ProjectConfigs.Num());
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 总共合并了 %d 个按钮配置"), MergedConfigs.Num());
    
    // 验证合并后的配置
    for (int32 i = 0; i < MergedConfigs.Num(); ++i)
    {
        const FToolbarButtonConfig& Config = MergedConfigs[i];
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 合并配置[%d] - 名称: %s, 类型: %d, 下拉项数量: %d"), 
               i, *Config.ButtonName, (int32)Config.ButtonType, Config.DropdownItems.Num());
    }
    
    return MergedConfigs;
}

TArray<FToolbarButtonConfig> UUtilityToolbarManager::GetPersistentButtonConfigs() const
{
    UUtilityExtendPersistentSettings* PersistentSettings = UUtilityExtendPersistentSettings::Get();
    if (PersistentSettings)
    {
        return PersistentSettings->GetPersistentButtonConfigs();
    }
    return TArray<FToolbarButtonConfig>();
}

TArray<FToolbarButtonConfig> UUtilityToolbarManager::GetProjectButtonConfigs() const
{
    UUtilityExtendSettings* ProjectSettings = UUtilityExtendSettings::Get();
    if (ProjectSettings)
    {
        return ProjectSettings->ToolbarButtonConfigs;
    }
    return TArray<FToolbarButtonConfig>();
}

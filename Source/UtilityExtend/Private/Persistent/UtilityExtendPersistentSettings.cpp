// Copyright Epic Games, Inc. All Rights Reserved.

#include "Persistent/UtilityExtendPersistentSettings.h"
#include "UtilityExtendIconRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "TimerManager.h"

UUtilityExtendPersistentSettings::UUtilityExtendPersistentSettings()
{
    // 设置类别名称，使其显示在项目设置的"插件"分类下
    CategoryName = TEXT("Plugins");
    
    // 获取插件配置文件路径
    PluginConfigPath = GetPluginDirectory() / TEXT("Config") / TEXT("DefaultUtilityExtendPersistent.ini");
    
    // 不在构造函数中初始化配置，等待PostInitProperties调用
}

UUtilityExtendPersistentSettings::~UUtilityExtendPersistentSettings()
{
    // 清理延迟保存计时器
    if (GEditor)
    {
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        if (EditorWorld && DelayedSaveTimerHandle.IsValid())
        {
            EditorWorld->GetTimerManager().ClearTimer(DelayedSaveTimerHandle);
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 析构函数中清理延迟保存计时器"));
        }
    }
}

void UUtilityExtendPersistentSettings::Initialize()
{
    // UDeveloperSettings由引擎自动管理，这里确保配置被加载
    UUtilityExtendPersistentSettings* Settings = Get();
    if (Settings)
    {
        Settings->LoadFromPluginConfig();
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置系统初始化完成"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 持久化设置系统初始化失败"));
    }
}

void UUtilityExtendPersistentSettings::Shutdown()
{
    // UDeveloperSettings由引擎管理，无需手动清理
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置系统清理完成"));
}

UUtilityExtendPersistentSettings* UUtilityExtendPersistentSettings::Get()
{
    // 使用UDeveloperSettings的标准获取方法
    return GetMutableDefault<UUtilityExtendPersistentSettings>();
}

FString UUtilityExtendPersistentSettings::GetPluginDirectory() const
{
    // 获取插件管理器
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("UtilityExtend");
    if (Plugin.IsValid())
    {
        return Plugin->GetBaseDir();
    }
    
    UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 无法找到插件目录"));
    return FString();
}

void UUtilityExtendPersistentSettings::SetPersistentButtonConfigs(const TArray<FToolbarButtonConfig>& NewConfigs)
{
    PersistentButtonConfigs = NewConfigs;
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 设置持久化按钮配置，数量: %d"), PersistentButtonConfigs.Num());
}

void UUtilityExtendPersistentSettings::AddPersistentButtonConfig(const FToolbarButtonConfig& Config)
{
    PersistentButtonConfigs.Add(Config);
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 添加持久化按钮配置: %s"), *Config.ButtonName);
}

void UUtilityExtendPersistentSettings::RemovePersistentButtonConfig(int32 Index)
{
    if (PersistentButtonConfigs.IsValidIndex(Index))
    {
        FString ButtonName = PersistentButtonConfigs[Index].ButtonName;
        PersistentButtonConfigs.RemoveAt(Index);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 移除持久化按钮配置: %s"), *ButtonName);
    }
}

void UUtilityExtendPersistentSettings::ClearPersistentButtonConfigs()
{
    PersistentButtonConfigs.Empty();
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 清空所有持久化按钮配置"));
}

bool UUtilityExtendPersistentSettings::LoadFromPluginConfig()
{
    if (PluginConfigPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 插件配置文件路径为空"));
        return false;
    }

    // 检查配置文件是否存在
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PluginConfigPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 插件配置文件不存在: %s"), *PluginConfigPath);
        return false;
    }

    // 解析配置文件
    bool bSuccess = ParseConfigFile();
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 从插件配置文件加载完成，按钮数量: %d"), PersistentButtonConfigs.Num());
    return bSuccess;
}

bool UUtilityExtendPersistentSettings::SaveToPluginConfig()
{
    if (PluginConfigPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 插件配置文件路径为空，无法保存"));
        return false;
    }

    // 验证当前配置数据的完整性
    if (!ValidateConfigData())
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 配置数据验证失败，跳过保存以防止数据损坏"));
        return false;
    }

    // 备份功能已移除 - 直接保存配置文件

    // 创建配置对象
    FConfigFile ConfigFile;
    
    // 如果文件已存在，先读取现有内容
    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*PluginConfigPath))
    {
        ConfigFile.Read(PluginConfigPath);
    }

    // 获取或创建配置部分
    const FString ConfigSectionName = TEXT("/Script/UtilityExtend.UtilityExtendPersistentSettings");
    FConfigSection* ConfigSection = const_cast<FConfigSection*>(ConfigFile.FindOrAddConfigSection(*ConfigSectionName));
    if (!ConfigSection)
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 无法创建配置文件部分"));
        return false;
    }
    
    // 清空现有的按钮配置
    TArray<FName> KeysToRemove;
    for (auto& ConfigPair : *ConfigSection)
    {
        FString KeyString = ConfigPair.Key.ToString();
        if (KeyString.StartsWith(TEXT("PersistentButtonConfigs")) || KeyString == TEXT("+PersistentButtonConfigs"))
        {
            KeysToRemove.Add(ConfigPair.Key);
        }
    }
    for (const FName& Key : KeysToRemove)
    {
        ConfigSection->Remove(Key);
    }

    // 保存按钮配置（使用 + 前缀，这是UE的数组配置格式）
    for (int32 i = 0; i < PersistentButtonConfigs.Num(); ++i)
    {
        const FToolbarButtonConfig& Config = PersistentButtonConfigs[i];
        
        // 获取按钮类型字符串
        FString ButtonTypeString;
        if (Config.ButtonType == EToolbarButtonType::SingleButton)
        {
            ButtonTypeString = TEXT("SingleButton");
        }
        else if (Config.ButtonType == EToolbarButtonType::DropdownButton)
        {
            ButtonTypeString = TEXT("DropdownButton");
        }
        
        // 构建下拉项字符串
        FString DropdownItemsString = TEXT("");
        if (Config.ButtonType == EToolbarButtonType::DropdownButton && Config.DropdownItems.Num() > 0)
        {
            TArray<FString> ItemStrings;
            for (const FToolbarDropdownItem& Item : Config.DropdownItems)
            {
                FString BoundClassString = TEXT("None");
                if (Item.BoundClass.IsValid())
                {
                    BoundClassString = Item.BoundClass.ToString();
                }
                
                FString ItemString = FString::Printf(TEXT("(ItemName=\"%s\",BoundClass=\"%s\")"),
                    *Item.ItemName,
                    *BoundClassString
                );
                ItemStrings.Add(ItemString);
            }
            DropdownItemsString = FString::Printf(TEXT(",DropdownItems=(%s)"), *FString::Join(ItemStrings, TEXT(",")));
        }
        
        // 获取单个按钮的绑定类字符串
        FString SingleButtonBoundClassString = TEXT("None");
        if (Config.BoundClass.IsValid())
        {
            SingleButtonBoundClassString = Config.BoundClass.ToString();
        }
        
        // 构建配置字符串
        FString ConfigValue = FString::Printf(TEXT("(ButtonName=\"%s\",ButtonType=%s,BoundClass=\"%s\",ButtonIconName=\"%s\",bShowButtonText=%s%s)"),
            *Config.ButtonName,
            *ButtonTypeString,
            *SingleButtonBoundClassString,
            *Config.ButtonIconName.ToString(),
            Config.bShowButtonText ? TEXT("true") : TEXT("false"),
            *DropdownItemsString
        );
        
        // 使用 + 前缀添加数组元素
        ConfigSection->AddUnique(TEXT("+PersistentButtonConfigs"), *ConfigValue);
    }

    // 保存文件
    bool bSaved = ConfigFile.Write(PluginConfigPath);
    
    if (bSaved)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 配置已成功保存到插件配置文件: %s"), *PluginConfigPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 保存插件配置文件失败: %s"), *PluginConfigPath);
    }
    
    return bSaved;
}

void UUtilityExtendPersistentSettings::ReloadConfig()
{
    // 重新加载配置
    InitializeDefaultConfig();
    LoadFromPluginConfig();
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化配置重新加载完成"));
}

bool UUtilityExtendPersistentSettings::ParseConfigFile()
{
    // 创建临时配置对象来解析INI文件
    FConfigFile ConfigFile;
    
    // 加载INI文件
    ConfigFile.Read(PluginConfigPath);
    
    // 检查文件是否成功加载（通过检查是否有任何部分）
    if (ConfigFile.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 无法读取插件配置文件或文件为空: %s"), *PluginConfigPath);
        return false;
    }

    // 清空现有配置
    PersistentButtonConfigs.Empty();

    // 获取配置部分
    const FConfigSection* ConfigSection = ConfigFile.FindSection(TEXT("/Script/UtilityExtend.UtilityExtendPersistentSettings"));
    if (!ConfigSection)
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 配置文件中未找到持久化设置部分"));
        return false;
    }

    // 解析按钮配置
    for (auto& ConfigPair : *ConfigSection)
    {
        FString Key = ConfigPair.Key.ToString();
        if (Key == TEXT("+PersistentButtonConfigs") || Key.StartsWith(TEXT("PersistentButtonConfigs")))
        {
            // 解析按钮配置字符串
            FString ConfigValue = ConfigPair.Value.GetValue();
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 解析按钮配置 - Key: %s, Value: %s"), *Key, *ConfigValue);
            
            // 解析配置字符串
            FToolbarButtonConfig ParsedConfig;
            if (ParseButtonConfigFromString(ConfigValue, ParsedConfig))
            {
                PersistentButtonConfigs.Add(ParsedConfig);
                UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 成功解析按钮配置: %s"), *ParsedConfig.ButtonName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 解析按钮配置失败: %s"), *ConfigValue);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 配置文件解析完成，共加载 %d 个按钮配置"), PersistentButtonConfigs.Num());
    return true;
}

void UUtilityExtendPersistentSettings::InitializeDefaultConfig()
{
    // 清空配置数组
    PersistentButtonConfigs.Empty();
    
    // 添加默认的持久化按钮配置
    FToolbarButtonConfig DefaultConfig;
    DefaultConfig.ButtonName = TEXT("插件工具箱");
    DefaultConfig.ButtonType = EToolbarButtonType::DropdownButton;
    DefaultConfig.ButtonIconName = FName("工具箱");
    DefaultConfig.bShowButtonText = true; // 默认显示按钮文本
    
    // 添加下拉项
    FToolbarDropdownItem Item1;
    Item1.ItemName = TEXT("插件信息");
    Item1.BoundClass = nullptr;
    DefaultConfig.DropdownItems.Add(Item1);
    
    FToolbarDropdownItem Item2;
    Item2.ItemName = TEXT("插件设置");
    Item2.BoundClass = nullptr;
    DefaultConfig.DropdownItems.Add(Item2);
    
    PersistentButtonConfigs.Add(DefaultConfig);
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 默认持久化配置初始化完成"));
}

TArray<FName> UUtilityExtendPersistentSettings::GetPersistentIconNames() const
{
    // 获取插件内置图标信息并提取名称
    TArray<FToolbarIconInfo> PluginIcons = FUtilityExtendIconRegistry::GetAllAvailableIconInfos();
    TArray<FName> PluginIconNames;
    
    for (const FToolbarIconInfo& IconInfo : PluginIcons)
    {
        PluginIconNames.Add(IconInfo.IconName);
    }
    
    return PluginIconNames;
}

TArray<FName> UUtilityExtendPersistentSettings::GetAllAvailableIconNames() const
{
    // 获取插件内置图标信息并提取友好显示名称
    TArray<FToolbarIconInfo> PluginIcons = FUtilityExtendIconRegistry::GetAllAvailableIconInfos();
    TArray<FName> PluginIconDisplayNames;
    
    for (const FToolbarIconInfo& IconInfo : PluginIcons)
    {
        // 返回友好显示名称，而不是技术名称
        PluginIconDisplayNames.Add(FName(*IconInfo.DisplayName));
    }
    
    return PluginIconDisplayNames;
}

bool UUtilityExtendPersistentSettings::IsValidPersistentIcon(const FName& IconName) const
{
    // 检查图标是否为有效插件图标
    return FUtilityExtendIconRegistry::IsValidIconName(IconName);
}

FName UUtilityExtendPersistentSettings::GetIconNameFromDisplayName(const FString& DisplayName) const
{
    // 直接调用图标注册系统的转换方法
    return FUtilityExtendIconRegistry::GetIconNameFromDisplayName(DisplayName);
}

FString UUtilityExtendPersistentSettings::GetIconDisplayNameFromName(const FName& IconName) const
{
    // 直接调用图标注册系统的转换方法
    return FUtilityExtendIconRegistry::GetIconDisplayNameFromName(IconName);
}

// UDeveloperSettings 接口实现
FName UUtilityExtendPersistentSettings::GetCategoryName() const
{
    return TEXT("Plugins");
}

void UUtilityExtendPersistentSettings::PostInitProperties()
{
    Super::PostInitProperties();
    
    // 清空现有配置
    PersistentButtonConfigs.Empty();
    
    // 尝试从插件配置文件加载数据
    bool bLoadedFromFile = LoadFromPluginConfig();
    
    // 如果没有从文件加载到数据，使用默认配置
    if (!bLoadedFromFile || PersistentButtonConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 未能从配置文件加载数据，使用默认配置"));
        InitializeDefaultConfig();
    }
    
    // 记录初始化完成
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置初始化完成，配置文件: %s，按钮数量: %d"), 
           *PluginConfigPath, PersistentButtonConfigs.Num());
}

void UUtilityExtendPersistentSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // 不调用Super，防止保存到项目配置文件
    // Super::PostEditChangeProperty(PropertyChangedEvent);
    
    // 当属性被修改时，立即保存到插件配置文件
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置属性已修改: %s，立即保存"), *PropertyName.ToString());
        
        // 立即保存，避免延迟保存导致的数据丢失
        bool bSaveSuccess = SaveToPluginConfig();
        if (bSaveSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 配置保存成功"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 配置保存失败"));
        }
    }
}

bool UUtilityExtendPersistentSettings::ParseButtonConfigFromString(const FString& ConfigString, FToolbarButtonConfig& OutConfig)
{
    // 重置输出配置
    OutConfig = FToolbarButtonConfig();
    
    // 移除最外层括号
    FString CleanString = ConfigString;
    CleanString = CleanString.TrimStartAndEnd();
    
    if (CleanString.StartsWith(TEXT("(")) && CleanString.EndsWith(TEXT(")")))
    {
        CleanString = CleanString.Mid(1, CleanString.Len() - 2);
    }
    
    // 手动解析参数，因为DropdownItems包含嵌套的括号
    TArray<FString> Parameters;
    ParseTopLevelParameters(CleanString, Parameters);
    
    for (const FString& Parameter : Parameters)
    {
        FString Key, Value;
        if (Parameter.Split(TEXT("="), &Key, &Value))
        {
            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();
            
            // 移除值的引号
            Value = Value.Replace(TEXT("\""), TEXT(""));
            
            if (Key == TEXT("ButtonName"))
            {
                OutConfig.ButtonName = Value;
            }
            else if (Key == TEXT("ButtonType"))
            {
                // 解析按钮类型
                if (Value == TEXT("SingleButton") || Value == TEXT("EToolbarButtonType::SingleButton"))
                {
                    OutConfig.ButtonType = EToolbarButtonType::SingleButton;
                }
                else if (Value == TEXT("DropdownButton") || Value == TEXT("EToolbarButtonType::DropdownButton"))
                {
                    OutConfig.ButtonType = EToolbarButtonType::DropdownButton;
                }

                else
                {
                    // 默认为单按钮
                    OutConfig.ButtonType = EToolbarButtonType::SingleButton;
                }
            }
            else if (Key == TEXT("BoundClass"))
            {
                // 解析单个按钮的绑定类
                if (Value != TEXT("None") && !Value.IsEmpty())
                {
                    OutConfig.BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(Value));
                }
            }
            else if (Key == TEXT("ButtonIconName"))
            {
                if (!Value.IsEmpty())
                {
                    OutConfig.ButtonIconName = FName(*Value);
                }
            }
            else if (Key == TEXT("bShowButtonText"))
            {
                // 解析布尔值
                OutConfig.bShowButtonText = (Value.ToLower() == TEXT("true") || Value == TEXT("1"));
            }
            else if (Key == TEXT("DropdownItems"))
            {
                // 解析下拉项
                if (!ParseDropdownItemsFromString(Value, OutConfig.DropdownItems))
                {
                    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 解析下拉项失败: %s"), *Value);
                }
            }
        }
    }
    
    // 检查必要字段是否已设置
    if (OutConfig.ButtonName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 解析按钮配置失败 - 缺少ButtonName"));
        return false;
    }
    
    // 为单个按钮设置默认图标（如果没有设置图标或图标为空）
    if (OutConfig.ButtonType == EToolbarButtonType::SingleButton && 
        (OutConfig.ButtonIconName.IsNone() || OutConfig.ButtonIconName.ToString().IsEmpty()))
    {
        // 设置默认图标的友好显示名称，而不是技术名称
        FName DefaultTechnicalName = FUtilityExtendIconRegistry::GetDefaultButtonIconName();
        FString DefaultDisplayName = FUtilityExtendIconRegistry::GetIconDisplayNameFromName(DefaultTechnicalName);
        OutConfig.ButtonIconName = FName(*DefaultDisplayName);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 单个按钮使用默认图标: %s (技术名: %s)"), *DefaultDisplayName, *DefaultTechnicalName.ToString());
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 成功解析按钮配置 - Name: %s, Type: %d, Icon: %s, ShowText: %s"), 
           *OutConfig.ButtonName, (int32)OutConfig.ButtonType, *OutConfig.ButtonIconName.ToString(),
           OutConfig.bShowButtonText ? TEXT("true") : TEXT("false"));
    
    return true;
}

bool UUtilityExtendPersistentSettings::ParseDropdownItemsFromString(const FString& DropdownString, TArray<FToolbarDropdownItem>& OutItems)
{
    OutItems.Empty();
    
    // 移除外层括号
    FString CleanString = DropdownString;
    CleanString = CleanString.TrimStartAndEnd();
    
    // 如果字符串为空或只有括号，返回true（空的下拉项是有效的）
    if (CleanString.IsEmpty() || CleanString == TEXT("()"))
    {
        return true;
    }
    
    // 移除最外层的括号
    if (CleanString.StartsWith(TEXT("(")) && CleanString.EndsWith(TEXT(")")))
    {
        CleanString = CleanString.Mid(1, CleanString.Len() - 2);
    }
    
    // 解析下拉项 - 格式: (ItemName="...",BoundClass=...),(ItemName="...",BoundClass=...)
    TArray<FString> ItemStrings;
    
    // 使用简单的括号匹配来分割项目
    int32 BracketCount = 0;
    int32 StartIndex = 0;
    
    for (int32 i = 0; i < CleanString.Len(); i++)
    {
        TCHAR CurrentChar = CleanString[i];
        
        if (CurrentChar == TEXT('('))
        {
            BracketCount++;
        }
        else if (CurrentChar == TEXT(')'))
        {
            BracketCount--;
            
            // 当括号匹配完成时，提取一个完整的项目
            if (BracketCount == 0)
            {
                FString ItemString = CleanString.Mid(StartIndex, i - StartIndex + 1);
                ItemStrings.Add(ItemString);
                
                // 寻找下一个项目的开始（跳过逗号）
                for (int32 j = i + 1; j < CleanString.Len(); j++)
                {
                    if (CleanString[j] == TEXT('('))
                    {
                        StartIndex = j;
                        i = j - 1; // -1 because the loop will increment
                        break;
                    }
                }
            }
        }
    }
    
    // 解析每个项目
    for (const FString& ItemString : ItemStrings)
    {
        FToolbarDropdownItem Item;
        if (ParseSingleDropdownItem(ItemString, Item))
        {
            OutItems.Add(Item);
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 成功解析下拉项: %s"), *Item.ItemName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 解析下拉项失败: %s"), *ItemString);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉项解析完成，共解析出 %d 个项目"), OutItems.Num());
    return true;
}

bool UUtilityExtendPersistentSettings::ParseSingleDropdownItem(const FString& ItemString, FToolbarDropdownItem& OutItem)
{
    OutItem = FToolbarDropdownItem();
    
    // 移除外层括号
    FString CleanString = ItemString;
    CleanString = CleanString.TrimStartAndEnd();
    
    if (CleanString.StartsWith(TEXT("(")) && CleanString.EndsWith(TEXT(")")))
    {
        CleanString = CleanString.Mid(1, CleanString.Len() - 2);
    }
    
    // 按逗号分割参数
    TArray<FString> Parameters;
    CleanString.ParseIntoArray(Parameters, TEXT(","), true);
    
    for (const FString& Parameter : Parameters)
    {
        FString Key, Value;
        if (Parameter.Split(TEXT("="), &Key, &Value))
        {
            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();
            
            // 移除值的引号
            Value = Value.Replace(TEXT("\""), TEXT(""));
            
            if (Key == TEXT("ItemName"))
            {
                OutItem.ItemName = Value;
            }
            else if (Key == TEXT("BoundClass"))
            {
                if (Value != TEXT("None") && !Value.IsEmpty())
                {
                    // 尝试解析软类引用
                    OutItem.BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(Value));
                }
            }
        }
    }
    
    // 检查必要字段是否已设置
    if (OutItem.ItemName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 解析下拉项失败 - 缺少ItemName"));
        return false;
    }
    
    return true;
}

void UUtilityExtendPersistentSettings::ParseTopLevelParameters(const FString& ParameterString, TArray<FString>& OutParameters)
{
    OutParameters.Empty();
    
    int32 BracketCount = 0;
    int32 StartIndex = 0;
    
    for (int32 i = 0; i < ParameterString.Len(); i++)
    {
        TCHAR CurrentChar = ParameterString[i];
        
        if (CurrentChar == TEXT('('))
        {
            BracketCount++;
        }
        else if (CurrentChar == TEXT(')'))
        {
            BracketCount--;
        }
        else if (CurrentChar == TEXT(',') && BracketCount == 0)
        {
            // 在顶层遇到逗号，分割参数
            FString Parameter = ParameterString.Mid(StartIndex, i - StartIndex);
            Parameter = Parameter.TrimStartAndEnd();
            if (!Parameter.IsEmpty())
            {
                OutParameters.Add(Parameter);
            }
            StartIndex = i + 1;
        }
    }
    
    // 添加最后一个参数
    if (StartIndex < ParameterString.Len())
    {
        FString Parameter = ParameterString.Mid(StartIndex);
        Parameter = Parameter.TrimStartAndEnd();
        if (!Parameter.IsEmpty())
        {
            OutParameters.Add(Parameter);
        }
    }
}

void UUtilityExtendPersistentSettings::ScheduleDelayedSave()
{
    // 获取编辑器世界，因为这是编辑器插件
    UWorld* EditorWorld = nullptr;
    if (GEditor)
    {
        EditorWorld = GEditor->GetEditorWorldContext().World();
    }
    
    if (EditorWorld && DelayedSaveTimerHandle.IsValid())
    {
        // 如果已经有计时器在运行，清除它
        EditorWorld->GetTimerManager().ClearTimer(DelayedSaveTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 清除现有的延迟保存计时器"));
    }
    
    if (EditorWorld)
    {
        // 设置新的延迟保存计时器
        EditorWorld->GetTimerManager().SetTimer(
            DelayedSaveTimerHandle,
            [this]()
            {
                UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 执行延迟保存配置"));
                bool bSaveSuccess = SaveToPluginConfig();
                if (bSaveSuccess)
                {
                    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 延迟保存配置成功"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 延迟保存配置失败"));
                }
                
                // 清除计时器句柄
                DelayedSaveTimerHandle.Invalidate();
            },
            SaveDelayTime,
            false  // 不循环
        );
        
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 安排延迟保存，延迟时间: %.1f 秒"), SaveDelayTime);
    }
    else
    {
        // 如果无法获取编辑器世界，回退到立即保存
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 无法获取编辑器世界，使用立即保存"));
        SaveToPluginConfig();
    }
}

bool UUtilityExtendPersistentSettings::ValidateConfigData() const
{
    // 验证按钮配置数组 - 使用宽松的验证，允许部分配置为空
    for (int32 i = 0; i < PersistentButtonConfigs.Num(); ++i)
    {
        const FToolbarButtonConfig& Config = PersistentButtonConfigs[i];
        
        // 只记录警告，不阻止保存 - 用户可能正在配置过程中
        if (Config.ButtonName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 配置验证警告 - 按钮 %d 的名称为空，但仍允许保存"), i);
        }
        
        // 如果是下拉按钮，验证下拉项 - 也使用宽松验证
        if (Config.ButtonType == EToolbarButtonType::DropdownButton)
        {
            for (int32 j = 0; j < Config.DropdownItems.Num(); ++j)
            {
                const FToolbarDropdownItem& Item = Config.DropdownItems[j];
                
                // 下拉项名称为空时只记录警告，不阻止保存
                if (Item.ItemName.IsEmpty())
                {
                    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 配置验证警告 - 按钮 '%s' 的下拉项 %d 名称为空"), *Config.ButtonName, j);
                }
                
                // 绑定类无效时也只记录警告
                if (Item.BoundClass.IsValid())
                {
                    FString ClassPath = Item.BoundClass.ToString();
                    if (ClassPath.IsEmpty() || ClassPath == TEXT("None"))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 配置验证警告 - 按钮 '%s' 的下拉项 '%s' 绑定类路径无效"), *Config.ButtonName, *Item.ItemName);
                    }
                }
            }
        }
        else if (Config.ButtonType == EToolbarButtonType::SingleButton)
        {
            // 对于单个按钮，检查绑定的脚本类路径 - 只警告不阻止
            if (Config.BoundClass.IsValid())
            {
                FString ClassPath = Config.BoundClass.ToString();
                if (ClassPath.IsEmpty() || ClassPath == TEXT("None"))
                {
                    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 配置验证警告 - 单个按钮 '%s' 绑定类路径无效"), *Config.ButtonName);
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 配置数据验证完成，共 %d 个按钮配置"), PersistentButtonConfigs.Num());
    return true; // 始终返回true，允许保存所有数据
}


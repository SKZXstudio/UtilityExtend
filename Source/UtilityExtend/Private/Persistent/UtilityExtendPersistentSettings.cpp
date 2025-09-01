// Copyright Epic Games, Inc. All Rights Reserved.

#include "Persistent/UtilityExtendPersistentSettings.h"
#include "UtilityExtendIconRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UUtilityExtendPersistentSettings::UUtilityExtendPersistentSettings()
{
    // 设置类别名称，使其显示在项目设置的"插件"分类下
    CategoryName = TEXT("Plugins");
    
    // 获取插件配置文件路径 - 改为JSON格式
    PluginConfigPath = GetPluginDirectory() / TEXT("Config") / TEXT("DefaultUtilityExtendPersistent.json");
    
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
    UUtilityExtendPersistentSettings* Settings = GetMutableDefault<UUtilityExtendPersistentSettings>();
    
    // 配置为空是正常的，用户可以从空白开始配置
    if (Settings && Settings->PersistentButtonConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化配置为空，这是正常的初始状态"));
    }
    
    return Settings;
}

FString UUtilityExtendPersistentSettings::GetPluginDirectory() const
{
    // 首先尝试通过插件管理器获取
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("UtilityExtend");
    if (Plugin.IsValid())
    {
        FString PluginDir = Plugin->GetBaseDir();
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 通过插件管理器获取到插件目录: %s"), *PluginDir);
        return PluginDir;
    }
    
    // 如果插件管理器失败，使用硬编码的绝对路径作为后备方案
    FString FallbackPath = TEXT("D:/Apps/Epic Games/UE/UE_S/Engine/Plugins/VDK/Editor/UtilityExtend");
    UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 插件管理器获取插件目录失败，使用后备路径: %s"), *FallbackPath);
    
    // 验证后备路径是否存在
    if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FallbackPath))
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 后备路径验证成功"));
        return FallbackPath;
    }
    
    UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 后备路径也不存在，无法找到插件目录"));
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
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始加载插件配置文件"));
    
    if (PluginConfigPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 插件配置文件路径为空，尝试重新获取"));
        PluginConfigPath = GetPluginDirectory() / TEXT("Config") / TEXT("DefaultUtilityExtendPersistent.json");
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 重新设置配置文件路径: %s"), *PluginConfigPath);
    }

    // 检查配置文件是否存在
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PluginConfigPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 插件配置文件不存在: %s"), *PluginConfigPath);
        
        // 尝试检查插件目录是否存在
        FString PluginDir = GetPluginDirectory();
        if (PluginDir.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 插件目录获取失败"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 插件目录: %s"), *PluginDir);
            FString ConfigDir = PluginDir / TEXT("Config");
            if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ConfigDir))
            {
                UE_LOG(LogTemp, Log, TEXT("UtilityExtend: Config目录存在: %s"), *ConfigDir);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UtilityExtend: Config目录不存在: %s"), *ConfigDir);
            }
        }
        
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 找到JSON配置文件，开始解析: %s"), *PluginConfigPath);

    // 解析JSON配置文件
    bool bSuccess = ParseJsonConfigFile();
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 从插件配置文件加载完成，解析结果: %s，按钮数量: %d"), 
           bSuccess ? TEXT("成功") : TEXT("失败"), PersistentButtonConfigs.Num());
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

    // 创建JSON对象
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ButtonConfigsArray;

    // 将按钮配置转换为JSON
    for (const FToolbarButtonConfig& Config : PersistentButtonConfigs)
    {
        TSharedPtr<FJsonObject> ConfigObject = MakeShareable(new FJsonObject);
        
        ConfigObject->SetStringField(TEXT("ButtonName"), Config.ButtonName);
        ConfigObject->SetStringField(TEXT("ButtonType"), Config.ButtonType == EToolbarButtonType::SingleButton ? TEXT("SingleButton") : TEXT("DropdownButton"));
        ConfigObject->SetStringField(TEXT("ButtonIconName"), Config.ButtonIconName.ToString());
        ConfigObject->SetBoolField(TEXT("bShowButtonText"), Config.bShowButtonText);
        
        // 单按钮绑定类 - 🔧 修复保存时被清空的问题
        FString ButtonBoundClassStr;
        if (Config.BoundClass.IsValid())
        {
            ButtonBoundClassStr = Config.BoundClass.ToString();
        }
        else if (Config.BoundClass.IsNull())
        {
            ButtonBoundClassStr = TEXT("None");
        }
        else
        {
            // 未加载状态，保持原路径
            ButtonBoundClassStr = Config.BoundClass.ToSoftObjectPath().ToString();
            if (ButtonBoundClassStr.IsEmpty())
            {
                ButtonBoundClassStr = TEXT("None");
            }
        }
        ConfigObject->SetStringField(TEXT("BoundClass"), ButtonBoundClassStr);
        
        // 下拉项
        if (Config.ButtonType == EToolbarButtonType::DropdownButton)
        {
            TArray<TSharedPtr<FJsonValue>> DropdownItemsArray;
            for (const FToolbarDropdownItem& Item : Config.DropdownItems)
            {
                TSharedPtr<FJsonObject> ItemObject = MakeShareable(new FJsonObject);
                ItemObject->SetStringField(TEXT("ItemName"), Item.ItemName);
                
                // 🔧 修复JSON保存时BoundClass被清空的问题
                FString BoundClassStr;
                if (Item.BoundClass.IsValid())
                {
                    BoundClassStr = Item.BoundClass.ToString();
                }
                else if (Item.BoundClass.IsNull())
                {
                    BoundClassStr = TEXT("None");
                }
                else
                {
                    // 未加载状态，保持原路径
                    BoundClassStr = Item.BoundClass.ToSoftObjectPath().ToString();
                    if (BoundClassStr.IsEmpty())
                    {
                        BoundClassStr = TEXT("None");
                    }
                }
                ItemObject->SetStringField(TEXT("BoundClass"), BoundClassStr);
                
                DropdownItemsArray.Add(MakeShareable(new FJsonValueObject(ItemObject)));
            }
            ConfigObject->SetArrayField(TEXT("DropdownItems"), DropdownItemsArray);
        }
        
        ButtonConfigsArray.Add(MakeShareable(new FJsonValueObject(ConfigObject)));
    }

    RootObject->SetArrayField(TEXT("PersistentButtonConfigs"), ButtonConfigsArray);

    // 序列化JSON到字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // 保存到文件
    bool bSaved = FFileHelper::SaveStringToFile(OutputString, *PluginConfigPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    
    if (bSaved)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化配置已保存到JSON文件: %s"), *PluginConfigPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 无法保存持久化配置到JSON文件: %s"), *PluginConfigPath);
    }
    
    return bSaved;
}

void UUtilityExtendPersistentSettings::ReloadConfig()
{
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 开始重新加载持久化配置"));
    
    // 尝试从JSON文件加载配置
    bool bLoadSuccess = LoadFromPluginConfig();
    
    if (bLoadSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 配置重新加载成功，当前按钮数量: %d"), PersistentButtonConfigs.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: 重新加载配置失败，保持当前配置不变"));
    }
}

bool UUtilityExtendPersistentSettings::ParseJsonConfigFile()
{
    // 读取JSON文件内容
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *PluginConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: 无法读取JSON配置文件: %s"), *PluginConfigPath);
        return false;
    }

    // 解析JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UtilityExtend: JSON配置文件解析失败: %s"), *PluginConfigPath);
        return false;
    }

    // 获取按钮配置数组
    const TArray<TSharedPtr<FJsonValue>>* ButtonConfigsArray;
    if (!JsonObject->TryGetArrayField(TEXT("PersistentButtonConfigs"), ButtonConfigsArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("UtilityExtend: JSON配置文件中未找到PersistentButtonConfigs字段"));
        return false;
    }

    // 先解析到临时数组，确保所有数据都正确后再替换
    TArray<FToolbarButtonConfig> NewConfigs;

    // 解析每个按钮配置
    for (const TSharedPtr<FJsonValue>& Value : *ButtonConfigsArray)
    {
        TSharedPtr<FJsonObject> ConfigObject = Value->AsObject();
        if (!ConfigObject.IsValid())
        {
            continue;
        }

        FToolbarButtonConfig ParsedConfig;
        
        // 解析基本字段
        ParsedConfig.ButtonName = ConfigObject->GetStringField(TEXT("ButtonName"));
        
        FString ButtonTypeString = ConfigObject->GetStringField(TEXT("ButtonType"));
        ParsedConfig.ButtonType = (ButtonTypeString == TEXT("SingleButton")) ? EToolbarButtonType::SingleButton : EToolbarButtonType::DropdownButton;
        
        ParsedConfig.ButtonIconName = FName(*ConfigObject->GetStringField(TEXT("ButtonIconName")));
        ParsedConfig.bShowButtonText = ConfigObject->GetBoolField(TEXT("bShowButtonText"));
        
        // 解析单按钮绑定类
        FString BoundClassString = ConfigObject->GetStringField(TEXT("BoundClass"));
        if (BoundClassString != TEXT("None") && !BoundClassString.IsEmpty())
        {
            ParsedConfig.BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(BoundClassString));
        }
        
        // 解析下拉项
        if (ParsedConfig.ButtonType == EToolbarButtonType::DropdownButton)
        {
            const TArray<TSharedPtr<FJsonValue>>* DropdownItemsArray;
            if (ConfigObject->TryGetArrayField(TEXT("DropdownItems"), DropdownItemsArray))
            {
                for (const TSharedPtr<FJsonValue>& ItemValue : *DropdownItemsArray)
                {
                    TSharedPtr<FJsonObject> ItemObject = ItemValue->AsObject();
                    if (!ItemObject.IsValid())
                    {
                        continue;
                    }

                    FToolbarDropdownItem DropdownItem;
                    DropdownItem.ItemName = ItemObject->GetStringField(TEXT("ItemName"));
                    
                    FString ItemBoundClassString = ItemObject->GetStringField(TEXT("BoundClass"));
                    if (ItemBoundClassString != TEXT("None") && !ItemBoundClassString.IsEmpty())
                    {
                        DropdownItem.BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(ItemBoundClassString));
                    }
                    
                    ParsedConfig.DropdownItems.Add(DropdownItem);
                }
            }
        }
        
        NewConfigs.Add(ParsedConfig);
        UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 成功解析JSON按钮配置: %s"), *ParsedConfig.ButtonName);
    }

    // 只有在完全成功解析后才替换现有配置
    PersistentButtonConfigs = MoveTemp(NewConfigs);
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: JSON配置文件解析完成，共加载 %d 个按钮配置"), PersistentButtonConfigs.Num());
    return true;
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
    
    // 尝试从插件配置文件加载数据
    bool bLoadedFromFile = LoadFromPluginConfig();
    
    // 记录初始化完成
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 持久化设置初始化完成，配置文件: %s，按钮数量: %d，加载成功: %s"), 
           *PluginConfigPath, PersistentButtonConfigs.Num(), bLoadedFromFile ? TEXT("是") : TEXT("否"));
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


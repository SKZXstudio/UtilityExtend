// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtendSettings.h"
#include "UtilityExtendIconRegistry.h"
#include "Engine/Engine.h"
#include "UObject/UnrealType.h"

UUtilityExtendSettings::UUtilityExtendSettings()
{
    CategoryName = TEXT("Plugins");
    
    // 不再使用硬编码的默认配置，让项目可以完全自定义按钮配置
}

UUtilityExtendSettings* UUtilityExtendSettings::Get()
{
    return GetMutableDefault<UUtilityExtendSettings>();
}

FName UUtilityExtendSettings::GetCategoryName() const
{
    return CategoryName;
}

FText UUtilityExtendSettings::GetDisplayName() const
{
    return FText::FromString(TEXT("UtilityExtend"));
}

// ========================= FToolbarDropdownItem 自定义序列化实现 =========================

bool FToolbarDropdownItem::Serialize(FArchive& Ar)
{
    // 让UE自动处理基本序列化
    bool bResult = true;
    
    // 序列化ItemName
    Ar << ItemName;
    
    // 自定义处理BoundClass的序列化 - 修复清空问题
    if (Ar.IsSaving())
    {
        // 保存时，将BoundClass转换为字符串路径
        // 🔧 关键修复：保持原有路径，除非明确设置为None
        FString ClassPath;
        if (BoundClass.IsValid())
        {
            ClassPath = BoundClass.ToString();
        }
        else if (BoundClass.IsNull())
        {
            // 明确设置为null时才保存为None
            ClassPath = TEXT("None");
        }
        else
        {
            // 未加载状态，保持现有路径
            ClassPath = BoundClass.ToSoftObjectPath().ToString();
            if (ClassPath.IsEmpty())
            {
                ClassPath = TEXT("None");
            }
        }
        
        Ar << ClassPath;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化保存下拉项 '%s', BoundClass='%s' (Valid:%s, Null:%s)"), 
               *ItemName, *ClassPath, BoundClass.IsValid() ? TEXT("true") : TEXT("false"),
               BoundClass.IsNull() ? TEXT("true") : TEXT("false"));
    }
    else if (Ar.IsLoading())
    {
        // 加载时，从字符串路径重建BoundClass
        FString ClassPath;
        Ar << ClassPath;
        
        if (ClassPath != TEXT("None") && !ClassPath.IsEmpty())
        {
            BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(ClassPath));
        }
        else
        {
            BoundClass = nullptr;
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化加载下拉项 '%s', BoundClass='%s'"), 
               *ItemName, *ClassPath);
    }
    
    return bResult;
}

bool FToolbarDropdownItem::ExportTextItem(FString& ValueStr, FToolbarDropdownItem const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
    // 构建导出字符串格式: (ItemName="名称",BoundClass="路径")
    // 🔧 修复导出时的BoundClass清空问题
    FString BoundClassStr;
    if (BoundClass.IsValid())
    {
        BoundClassStr = BoundClass.ToString();
    }
    else if (BoundClass.IsNull())
    {
        BoundClassStr = TEXT("None");
    }
    else
    {
        // 未加载状态，保持原路径
        BoundClassStr = BoundClass.ToSoftObjectPath().ToString();
        if (BoundClassStr.IsEmpty())
        {
            BoundClassStr = TEXT("None");
        }
    }
    
    ValueStr = FString::Printf(TEXT("(ItemName=\"%s\",BoundClass=\"%s\")"), 
                              *ItemName, 
                              *BoundClassStr);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 导出下拉项配置: %s (Valid:%s, Null:%s)"), 
           *ValueStr, BoundClass.IsValid() ? TEXT("true") : TEXT("false"),
           BoundClass.IsNull() ? TEXT("true") : TEXT("false"));
    return true;
}

bool FToolbarDropdownItem::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
    // 重置当前值
    ItemName.Empty();
    BoundClass = nullptr;
    
    // 跳过空白字符
    while (FChar::IsWhitespace(*Buffer))
    {
        Buffer++;
    }
    
    // 检查是否以括号开始
    if (*Buffer != TEXT('('))
    {
        if (ErrorText)
        {
            ErrorText->Logf(TEXT("Expected '(' at start of FToolbarDropdownItem"));
        }
        return false;
    }
    Buffer++; // 跳过开括号
    
    // 解析参数
    while (*Buffer && *Buffer != TEXT(')'))
    {
        // 跳过空白和逗号
        while (FChar::IsWhitespace(*Buffer) || *Buffer == TEXT(','))
        {
            Buffer++;
        }
        
        if (*Buffer == TEXT(')'))
        {
            break;
        }
        
        // 解析键值对
        FString Key, Value;
        
        // 读取键名
        while (*Buffer && *Buffer != TEXT('=') && !FChar::IsWhitespace(*Buffer))
        {
            Key.AppendChar(*Buffer);
            Buffer++;
        }
        
        // 跳过空白和等号
        while (FChar::IsWhitespace(*Buffer) || *Buffer == TEXT('='))
        {
            Buffer++;
        }
        
        // 读取值
        if (*Buffer == TEXT('"'))
        {
            Buffer++; // 跳过开引号
            while (*Buffer && *Buffer != TEXT('"'))
            {
                if (*Buffer == TEXT('\\') && *(Buffer + 1) == TEXT('"'))
                {
                    Buffer++; // 跳过转义符
                    Value.AppendChar(TEXT('"'));
                }
                else
                {
                    Value.AppendChar(*Buffer);
                }
                Buffer++;
            }
            if (*Buffer == TEXT('"'))
            {
                Buffer++; // 跳过结束引号
            }
        }
        else
        {
            // 没有引号的值
            while (*Buffer && *Buffer != TEXT(',') && *Buffer != TEXT(')') && !FChar::IsWhitespace(*Buffer))
            {
                Value.AppendChar(*Buffer);
                Buffer++;
            }
        }
        
        // 应用解析的键值对
        if (Key == TEXT("ItemName"))
        {
            ItemName = Value;
        }
        else if (Key == TEXT("BoundClass"))
        {
            if (Value != TEXT("None") && !Value.IsEmpty())
            {
                BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(Value));
            }
        }
    }
    
    // 跳过结束括号
    if (*Buffer == TEXT(')'))
    {
        Buffer++;
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 导入下拉项配置 - ItemName='%s', BoundClass='%s'"), 
           *ItemName, 
           BoundClass.IsValid() ? *BoundClass.ToString() : TEXT("None"));
    
    return true;
}

// ========================= FToolbarButtonConfig 自定义序列化实现 =========================

bool FToolbarButtonConfig::Serialize(FArchive& Ar)
{
    bool bResult = true;
    
    // 序列化基本字段
    Ar << ButtonName;
    
    // 序列化按钮类型（作为uint8）
    uint8 ButtonTypeValue = static_cast<uint8>(ButtonType);
    Ar << ButtonTypeValue;
    if (Ar.IsLoading())
    {
        ButtonType = static_cast<EToolbarButtonType>(ButtonTypeValue);
    }
    
    // 序列化图标名称
    FString IconNameStr = ButtonIconName.ToString();
    Ar << IconNameStr;
    if (Ar.IsLoading())
    {
        ButtonIconName = FName(*IconNameStr);
    }
    
    // 序列化显示文本标志
    Ar << bShowButtonText;
    
    // 自定义处理BoundClass的序列化
    if (Ar.IsSaving())
    {
        FString ClassPath;
        if (BoundClass.IsValid())
        {
            ClassPath = BoundClass.ToString();
        }
        else if (BoundClass.IsNull())
        {
            ClassPath = TEXT("None");
        }
        else
        {
            ClassPath = BoundClass.ToSoftObjectPath().ToString();
            if (ClassPath.IsEmpty())
            {
                ClassPath = TEXT("None");
            }
        }
        Ar << ClassPath;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化保存按钮配置 '%s', BoundClass='%s'"), 
               *ButtonName, *ClassPath);
    }
    else if (Ar.IsLoading())
    {
        FString ClassPath;
        Ar << ClassPath;
        
        if (ClassPath != TEXT("None") && !ClassPath.IsEmpty())
        {
            BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(ClassPath));
        }
        else
        {
            BoundClass = nullptr;
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化加载按钮配置 '%s', BoundClass='%s'"), 
               *ButtonName, *ClassPath);
    }
    
    // 序列化下拉项数组
    int32 DropdownItemsCount = DropdownItems.Num();
    Ar << DropdownItemsCount;
    
    if (Ar.IsLoading())
    {
        DropdownItems.SetNum(DropdownItemsCount);
    }
    
    for (int32 i = 0; i < DropdownItemsCount; ++i)
    {
        DropdownItems[i].Serialize(Ar);
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化按钮配置 '%s' 完成，下拉项数量: %d"), 
           *ButtonName, DropdownItems.Num());
    
    return bResult;
}

bool FToolbarButtonConfig::ExportTextItem(FString& ValueStr, FToolbarButtonConfig const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
    // 构建导出字符串
    FString ButtonTypeStr = (ButtonType == EToolbarButtonType::SingleButton) ? TEXT("SingleButton") : TEXT("DropdownButton");
    
    // 处理BoundClass
    FString BoundClassStr;
    if (BoundClass.IsValid())
    {
        BoundClassStr = BoundClass.ToString();
    }
    else if (BoundClass.IsNull())
    {
        BoundClassStr = TEXT("None");
    }
    else
    {
        BoundClassStr = BoundClass.ToSoftObjectPath().ToString();
        if (BoundClassStr.IsEmpty())
        {
            BoundClassStr = TEXT("None");
        }
    }
    
    // 构建下拉项字符串
    FString DropdownItemsStr;
    if (DropdownItems.Num() > 0)
    {
        TArray<FString> ItemStrings;
        for (const FToolbarDropdownItem& Item : DropdownItems)
        {
            FString ItemStr;
            Item.ExportTextItem(ItemStr, FToolbarDropdownItem(), Parent, PortFlags, ExportRootScope);
            ItemStrings.Add(ItemStr);
        }
        DropdownItemsStr = FString::Join(ItemStrings, TEXT(","));
    }
    
    ValueStr = FString::Printf(TEXT("(ButtonName=\"%s\",ButtonType=%s,BoundClass=\"%s\",ButtonIconName=\"%s\",DropdownItems=(%s),bShowButtonText=%s)"),
                              *ButtonName,
                              *ButtonTypeStr,
                              *BoundClassStr,
                              *ButtonIconName.ToString(),
                              *DropdownItemsStr,
                              bShowButtonText ? TEXT("True") : TEXT("False"));
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 导出按钮配置: %s"), *ValueStr);
    return true;
}

bool FToolbarButtonConfig::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
    // 重置所有值
    ButtonName.Empty();
    ButtonType = EToolbarButtonType::SingleButton;
    BoundClass = nullptr;
    ButtonIconName = NAME_None;
    DropdownItems.Empty();
    bShowButtonText = true;
    
    // 跳过空白字符
    while (FChar::IsWhitespace(*Buffer))
    {
        Buffer++;
    }
    
    // 检查是否以括号开始
    if (*Buffer != TEXT('('))
    {
        if (ErrorText)
        {
            ErrorText->Logf(TEXT("Expected '(' at start of FToolbarButtonConfig"));
        }
        return false;
    }
    Buffer++; // 跳过开括号
    
    // 解析参数
    while (*Buffer && *Buffer != TEXT(')'))
    {
        // 跳过空白和逗号
        while (FChar::IsWhitespace(*Buffer) || *Buffer == TEXT(','))
        {
            Buffer++;
        }
        
        if (*Buffer == TEXT(')'))
        {
            break;
        }
        
        // 解析键值对
        FString Key, Value;
        
        // 读取键名
        while (*Buffer && *Buffer != TEXT('=') && !FChar::IsWhitespace(*Buffer))
        {
            Key.AppendChar(*Buffer);
            Buffer++;
        }
        
        // 跳过空白和等号
        while (FChar::IsWhitespace(*Buffer) || *Buffer == TEXT('='))
        {
            Buffer++;
        }
        
        // 特殊处理DropdownItems（包含嵌套结构）
        if (Key == TEXT("DropdownItems"))
        {
            // 解析DropdownItems
            if (*Buffer == TEXT('('))
            {
                // 找到匹配的括号
                int32 BracketCount = 0;
                const TCHAR* StartPos = Buffer;
                const TCHAR* EndPos = Buffer;
                
                while (*EndPos)
                {
                    if (*EndPos == TEXT('('))
                    {
                        BracketCount++;
                    }
                    else if (*EndPos == TEXT(')'))
                    {
                        BracketCount--;
                        if (BracketCount == 0)
                        {
                            EndPos++; // 包含结束括号
                            break;
                        }
                    }
                    EndPos++;
                }
                
                FString DropdownValue(EndPos - StartPos, StartPos);
                Buffer = EndPos;
                
                // 解析下拉项
                ParseDropdownItemsFromString(DropdownValue);
            }
        }
        else
        {
            // 读取普通值
            if (*Buffer == TEXT('"'))
            {
                Buffer++; // 跳过开引号
                while (*Buffer && *Buffer != TEXT('"'))
                {
                    if (*Buffer == TEXT('\\') && *(Buffer + 1) == TEXT('"'))
                    {
                        Buffer++; // 跳过转义符
                        Value.AppendChar(TEXT('"'));
                    }
                    else
                    {
                        Value.AppendChar(*Buffer);
                    }
                    Buffer++;
                }
                if (*Buffer == TEXT('"'))
                {
                    Buffer++; // 跳过结束引号
                }
            }
            else
            {
                // 没有引号的值
                while (*Buffer && *Buffer != TEXT(',') && *Buffer != TEXT(')') && !FChar::IsWhitespace(*Buffer))
                {
                    Value.AppendChar(*Buffer);
                    Buffer++;
                }
            }
            
            // 应用解析的键值对
            if (Key == TEXT("ButtonName"))
            {
                ButtonName = Value;
            }
            else if (Key == TEXT("ButtonType"))
            {
                if (Value == TEXT("SingleButton"))
                {
                    ButtonType = EToolbarButtonType::SingleButton;
                }
                else if (Value == TEXT("DropdownButton"))
                {
                    ButtonType = EToolbarButtonType::DropdownButton;
                }
            }
            else if (Key == TEXT("BoundClass"))
            {
                if (Value != TEXT("None") && !Value.IsEmpty())
                {
                    BoundClass = TSoftClassPtr<UUtilityExtendTopBarButtonScript>(FSoftObjectPath(Value));
                }
            }
            else if (Key == TEXT("ButtonIconName"))
            {
                if (!Value.IsEmpty())
                {
                    ButtonIconName = FName(*Value);
                }
            }
            else if (Key == TEXT("bShowButtonText"))
            {
                bShowButtonText = (Value.ToLower() == TEXT("true"));
            }
        }
    }
    
    // 跳过结束括号
    if (*Buffer == TEXT(')'))
    {
        Buffer++;
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 导入按钮配置 - Name='%s', Type=%d, DropdownItems=%d"), 
           *ButtonName, (int32)ButtonType, DropdownItems.Num());
    
    return true;
}

void FToolbarButtonConfig::ParseDropdownItemsFromString(const FString& DropdownString)
{
    DropdownItems.Empty();
    
    FString CleanString = DropdownString.TrimStartAndEnd();
    
    // 如果字符串为空或只有括号，返回
    if (CleanString.IsEmpty() || CleanString == TEXT("()"))
    {
        return;
    }
    
    // 移除外层括号
    if (CleanString.StartsWith(TEXT("(")) && CleanString.EndsWith(TEXT(")")))
    {
        CleanString = CleanString.Mid(1, CleanString.Len() - 2);
    }
    
    if (CleanString.IsEmpty())
    {
        return;
    }
    
    // 解析下拉项
    TArray<FString> ItemStrings;
    
    // 使用括号匹配来分割项目
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
            
            if (BracketCount == 0)
            {
                FString ItemString = CleanString.Mid(StartIndex, i - StartIndex + 1);
                ItemStrings.Add(ItemString);
                
                // 寻找下一个项目的开始
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
        const TCHAR* Buffer = *ItemString;
        if (Item.ImportTextItem(Buffer, 0, nullptr, nullptr))
        {
            DropdownItems.Add(Item);
            UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 成功解析下拉项: %s"), *Item.ItemName);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("UtilityExtend: 下拉项解析完成，共解析出 %d 个项目"), DropdownItems.Num());
}









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
    
    // 自定义处理BoundClass的序列化
    if (Ar.IsSaving())
    {
        // 保存时，将BoundClass转换为字符串路径
        FString ClassPath = BoundClass.IsValid() ? BoundClass.ToString() : TEXT("None");
        Ar << ClassPath;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 序列化保存下拉项 '%s', BoundClass='%s'"), 
               *ItemName, *ClassPath);
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
    FString BoundClassStr = BoundClass.IsValid() ? BoundClass.ToString() : TEXT("None");
    
    ValueStr = FString::Printf(TEXT("(ItemName=\"%s\",BoundClass=\"%s\")"), 
                              *ItemName, 
                              *BoundClassStr);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("UtilityExtend: 导出下拉项配置: %s"), *ValueStr);
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









// Copyright Epic Games, Inc. All Rights Reserved.

#include "UtilityExtendCommands.h"

#define LOCTEXT_NAMESPACE "FUtilityExtendModule"

void FUtilityExtendCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "UtilityExtend", "Execute UtilityExtend action", EUserInterfaceActionType::Button, FInputChord());
}

// 注意：动态命令注册功能暂时移除，因为UI_COMMAND宏不支持动态参数
// 如果需要动态命令，建议使用其他方法实现

#undef LOCTEXT_NAMESPACE

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"

class FUtilityExtendCommands : public TCommands<FUtilityExtendCommands>
{
public:

	FUtilityExtendCommands()
		: TCommands<FUtilityExtendCommands>(TEXT("UtilityExtend"), NSLOCTEXT("Contexts", "UtilityExtend", "UtilityExtend Plugin"), NAME_None, FAppStyle::GetAppStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};

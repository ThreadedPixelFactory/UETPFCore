// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

using UnrealBuildTool;

public class SinglePlayerStoryTemplateEditorTarget : TargetRules
{
	public SinglePlayerStoryTemplateEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange( new string[] { "UETPFCore", "GameLauncher", "SinglePlayerStoryTemplate" } );
	}
}

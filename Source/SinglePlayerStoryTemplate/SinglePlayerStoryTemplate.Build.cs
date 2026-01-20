// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

using UnrealBuildTool;

public class SinglePlayerStoryTemplate : ModuleRules
{
	public SinglePlayerStoryTemplate(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine",
			"UETPFCore",           // Our physics/environment foundation
			"Json",                // JSON parsing for SpecPacks
			"JsonUtilities"        // JSON serialization helpers
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"InputCore",
			"EnhancedInput"
		});
	}
}

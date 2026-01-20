// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

using UnrealBuildTool;

public class GameLauncher : ModuleRules
{
	public GameLauncher(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine",
			"UETPFCore",           // Access to shared systems (Sky, Time, etc.)
			"UMG",                 // Widget system
			"Slate",               // Low-level UI
			"SlateCore",           // Slate foundation
			"InputCore",           // Input handling
			"EnhancedInput",       // UE5 enhanced input
			"Niagara"              // For menu particle effects
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"RenderCore",
			"RHI",
			"MoviePlayer"          // For loading screens if needed
		});
	}
}

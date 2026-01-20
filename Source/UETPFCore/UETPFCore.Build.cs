// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

using UnrealBuildTool;

public class UETPFCore : ModuleRules
{
	public UETPFCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine",
			"PhysicsCore",
			"Chaos",
			"DataRegistry",
			"Niagara",
			"NiagaraCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Landscape",
			"RenderCore",
			"RHI",
			"Renderer",
			"UnrealEd"  // Required for UMaterialFactoryNew
		});
	}
}

// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

using UnrealBuildTool;
using System.IO;

public class UETPFCoreSim : ModuleRules
{
	public UETPFCoreSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine",
			"UETPFCore"  // Our core framework
		});

		PrivateDependencyModuleNames.AddRange(new string[] 
		{ 
			// Add private dependencies here
		});

		// SPICE library integration
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "ThirdParty");
		string SpicePath = Path.Combine(ThirdPartyPath, "cspice");
		
		if (Directory.Exists(SpicePath))
		{
			PublicIncludePaths.Add(Path.Combine(SpicePath, "include"));
			
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string LibPath = Path.Combine(SpicePath, "lib", "Win64");
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "cspice.lib"));
				
				// Copy DLL to binaries if exists
				string DllPath = Path.Combine(SpicePath, "bin", "Win64", "cspice.dll");
				if (File.Exists(DllPath))
				{
					RuntimeDependencies.Add("$(BinaryOutputDir)/cspice.dll", DllPath);
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				string LibPath = Path.Combine(SpicePath, "lib", "Linux");
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcspice.a"));
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string LibPath = Path.Combine(SpicePath, "lib", "Mac");
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcspice.a"));
			}
			
			PublicDefinitions.Add("WITH_SPICE=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_SPICE=0");
		}
	}
}

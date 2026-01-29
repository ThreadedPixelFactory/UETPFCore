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
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string IncludePath = Path.Combine(SpicePath, "include", "Win64");
				string LibPath = Path.Combine(SpicePath, "lib", "Win64", "cspice.lib");
				
				if (Directory.Exists(Path.GetDirectoryName(IncludePath)) && File.Exists(LibPath))
				{
					PublicIncludePaths.Add(IncludePath);
					PublicAdditionalLibraries.Add(LibPath);
					PublicIncludePaths.Add(IncludePath);
					PublicAdditionalLibraries.Add(LibPath);
					
					// Copy DLL to binaries if exists
					string DllPath = Path.Combine(SpicePath, "bin", "Win64", "cspice.dll");
					if (File.Exists(DllPath))
					{
						RuntimeDependencies.Add("$(BinaryOutputDir)/cspice.dll", DllPath);
					}
					
					PublicDefinitions.Add("WITH_SPICE=1");
				}
				else
				{
					PublicDefinitions.Add("WITH_SPICE=0");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				string IncludePath = Path.Combine(SpicePath, "include", "Linux");
				string LibPath = Path.Combine(SpicePath, "lib", "Linux", "libcspice.a");
				
				if (Directory.Exists(Path.GetDirectoryName(IncludePath)) && File.Exists(LibPath))
				{
					PublicIncludePaths.Add(IncludePath);
					PublicAdditionalLibraries.Add(LibPath);
					PublicDefinitions.Add("WITH_SPICE=1");
				}
				else
				{
					PublicDefinitions.Add("WITH_SPICE=0");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string IncludePath = Path.Combine(SpicePath, "include", "Mac");
				string LibPath = Path.Combine(SpicePath, "lib", "Mac", "libcspice.a");
				
				if (Directory.Exists(Path.GetDirectoryName(IncludePath)) && File.Exists(LibPath))
				{
					PublicIncludePaths.Add(IncludePath);
					PublicAdditionalLibraries.Add(LibPath);
					PublicDefinitions.Add("WITH_SPICE=1");
				}
				else
				{
					PublicDefinitions.Add("WITH_SPICE=0");
				}
			}
		}
		else
		{
			PublicDefinitions.Add("WITH_SPICE=0");
		}
	}
}

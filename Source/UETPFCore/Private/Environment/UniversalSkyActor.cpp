// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Environment/UniversalSkyActor.h"

#include "SpecTypes.h"
#include "Subsystems/TimeSubsystem.h"
#include "Space/Subsystems/SolarSystemSubsystem.h"
#include "Space/Subsystems/StarCatalogSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Materials/MaterialCreator.h"

AUniversalSkyActor::AUniversalSkyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics; // Tick before physics so camera sees latest state

	// Root component - all other components attach to this
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Directional Light (Sun)
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(Root);
	SunLight->SetVisibility(true);
	SunLight->SetHiddenInGame(false);

	// Sky Atmosphere (atmospheric scattering)
	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(Root);
	SkyAtmosphere->SetVisibility(true);
	SkyAtmosphere->SetHiddenInGame(false);

	// Sky Light (ambient/GI contribution)
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetVisibility(true);
	SkyLight->SetHiddenInGame(false);

	// Volumetric Clouds
	VolumetricCloud = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	VolumetricCloud->SetupAttachment(Root);
	VolumetricCloud->SetVisibility(true);
	VolumetricCloud->SetHiddenInGame(false);

	// Exponential Height Fog
	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(Root);
	HeightFog->SetVisibility(true);
	HeightFog->SetHiddenInGame(false);

	// PostProcess Volume
	// CRITICAL: Do NOT attach to root - unbound volumes (bUnbound=true) must remain unattached
	// to affect the entire scene globally. Attaching them limits their influence to a bounded region.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetVisibility(true);

	// Niagara Starfield
	StarfieldComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("StarfieldComponent"));
	StarfieldComponent->SetupAttachment(Root);
	StarfieldComponent->SetVisibility(true);
	StarfieldComponent->SetHiddenInGame(false);

	// Load default Niagara system for starfield
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> StarfieldSystemFinder(TEXT("/Game/SpecPacks/Space/NS_StarField.NS_StarField"));
	if (StarfieldSystemFinder.Succeeded())
	{
		StarfieldNiagaraSystem = StarfieldSystemFinder.Object;
	}

	ConfigureDefaults();
}

void AUniversalSkyActor::ConfigureDefaults()
{
	/* ================================================================================
	 * DIRECTIONAL LIGHT (SUN) CONFIGURATION
	 * ================================================================================
	 * Mobility MUST be Movable for dynamic lighting with Lumen GI/Reflections.
	 * Stationary or Static mobility will not work correctly with procedural sky updates.
	 * ================================================================================ */
	SunLight->SetMobility(EComponentMobility::Movable);
	
	// CRITICAL: bAffectsWorld MUST be true for light to contribute to Lumen GI
	SunLight->bAffectsWorld = true;
	SunLight->bAtmosphereSunLight = true;
	SunLight->AtmosphereSunLightIndex = 0;
	SunLight->bCastCloudShadows = true;
	SunLight->bCastShadowsOnClouds = true;
	
	// Color temperature for realistic daylight
	SunLight->bUseTemperature = true;
	SunLight->Temperature = 6500.0f; // Neutral daylight white
	
	// Shadow casting required for Lumen dynamic lighting
	SunLight->CastShadows = true;
	SunLight->CastDynamicShadows = true;
	SunLight->bCastVolumetricShadow = true;
	
	// Base intensity (will be scaled by ApplySun based on atmospheric conditions)
	SunLight->Intensity = 50000.0f;
	
	// Ensure component is visible and active
	SunLight->SetVisibility(true);
	SunLight->SetHiddenInGame(false);
	SunLight->SetActive(true);
	
	UE_LOG(LogTemp, Warning, TEXT("☀️ SUN: Mobility=Movable, Intensity=%.0f lux, AffectsWorld=%d, Active=%d"), 
		SunLight->Intensity, SunLight->bAffectsWorld, SunLight->IsActive());

	/* ================================================================================
	 * SKY LIGHT CONFIGURATION
	 * ================================================================================
	 * SkyLight provides ambient lighting by capturing the sky dome.
	 * Real-time capture MUST be enabled for dynamic sky changes to affect lighting.
	 * Movable mobility required for Lumen to update GI from sky changes.
	 * ================================================================================ */
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->Intensity = 1.0f;
	SkyLight->SetVisibility(true);
	SkyLight->SetHiddenInGame(false);
	SkyLight->SetActive(true);
	
	UE_LOG(LogTemp, Warning, TEXT("🌤️ SKYLIGHT: Intensity=%.1f, RealTimeCapture=%d, Active=%d"), 
		SkyLight->Intensity, SkyLight->bRealTimeCapture, SkyLight->IsActive());

	/* ================================================================================
	 * SKY ATMOSPHERE CONFIGURATION
	 * ================================================================================
	 * Handles atmospheric scattering (Rayleigh/Mie) for realistic sky appearance.
	 * Earth-like atmosphere with proper physical dimensions and scattering.
	 * ================================================================================ */
	SkyAtmosphere->SetVisibility(true);
	SkyAtmosphere->SetHiddenInGame(false);
	SkyAtmosphere->SetActive(true);
	
	// Earth-like atmosphere dimensions (km converted to Unreal units)
	SkyAtmosphere->BottomRadius = 6360.0f; // Earth radius at sea level (km)
	SkyAtmosphere->AtmosphereHeight = 100.0f; // Atmosphere thickness (km)
	
	// Rayleigh scattering (blue sky) - default values work well
	SkyAtmosphere->RayleighScatteringScale = 0.0331f;
	SkyAtmosphere->RayleighExponentialDistribution = 8.0f;
	
	// Mie scattering (atmospheric haze/fog)
	SkyAtmosphere->MieScatteringScale = 0.003996f;
	SkyAtmosphere->MieAbsorptionScale = 0.000444f;
	SkyAtmosphere->MieAnisotropy = 0.8f;
	SkyAtmosphere->MieExponentialDistribution = 1.2f;
	
	// Ground albedo (how much light surface reflects back to atmosphere)
	SkyAtmosphere->GroundAlbedo = FColor(77, 77, 77); // 0.3 reflectance (77/255 ~= 0.3)
	
	UE_LOG(LogTemp, Log, TEXT("🌍 ATMOSPHERE: Active=%d, Visible=%d, BottomRadius=%.0f km"), 
		SkyAtmosphere->IsActive(), SkyAtmosphere->IsVisible(), SkyAtmosphere->BottomRadius);

	/* ================================================================================
	 * VOLUMETRIC COMPONENTS (FOG & CLOUDS)
	 * ================================================================================
	 * Mobility Movable required for dynamic fog/cloud changes in response to weather.
	 * NOTE: VolumetricCloud requires a material to be assigned for rendering!
	 * Without a cloud material, the component will be active but invisible.
	 * ================================================================================ */
	HeightFog->SetMobility(EComponentMobility::Movable);
	HeightFog->bEnableVolumetricFog = true;
	HeightFog->FogDensity = 0.002f; // Low density for atmospheric haze
	HeightFog->FogHeightFalloff = 0.2f; // Gradual density decrease with altitude
	HeightFog->DirectionalInscatteringExponent = 4.0f;
	HeightFog->DirectionalInscatteringStartDistance = 10000.0f; // 100m
	HeightFog->SetVisibility(true);
	HeightFog->SetHiddenInGame(false);
	HeightFog->SetActive(true);
	
	VolumetricCloud->SetMobility(EComponentMobility::Movable);
	VolumetricCloud->LayerBottomAltitude = 5.0f; // 5km cloud base
	VolumetricCloud->LayerHeight = 10.0f; // 10km cloud layer thickness
	VolumetricCloud->ViewSampleCountScale = 1.0f; // Full quality rendering
	VolumetricCloud->ShadowViewSampleCountScale = 0.5f; // Half quality for shadows
	VolumetricCloud->SetVisibility(true);
	VolumetricCloud->SetHiddenInGame(false);
	VolumetricCloud->SetActive(true);
	
	UE_LOG(LogTemp, Log, TEXT("🌫️ FOG & CLOUDS: Active=%d/%d, CloudLayer=%.0f-%.0f km"), 
		HeightFog->IsActive(), VolumetricCloud->IsActive(),
		VolumetricCloud->LayerBottomAltitude, VolumetricCloud->LayerBottomAltitude + VolumetricCloud->LayerHeight);

	/* ================================================================================
	 * POST PROCESS VOLUME CONFIGURATION FOR LUMEN
	 * ================================================================================
	 * CRITICAL SETTINGS FOR GLOBAL POST-PROCESS:
	 * 
	 * 1. bUnbound = true: Volume affects entire scene (not spatially limited)
	 * 2. BlendWeight = 1.0f: MUST be > 0 for settings to take effect
	 * 3. NOT attached to RootComponent: Unbound volumes must remain unattached
	 * 4. High Priority: Overrides other post-process volumes
	 * 
	 * This configures exposure and Lumen for space/night sky rendering where
	 * extreme dynamic range (bright stars vs black space) requires manual exposure.
	 * ================================================================================ */
	PostProcess->bUnbound = true; // Global effect
	PostProcess->BlendWeight = 1.0f; // CRITICAL: Must be 1.0 to activate
	PostProcess->Priority = 10.0f; // High priority to override project defaults
	
	// Manual exposure for space scenes (auto-exposure fails with extreme dynamic range)
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = AEM_Manual;
	
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = -2.0f; // Brighten by 2 stops
	
	// Exposure range clamping for Lumen
	PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
	PostProcess->Settings.AutoExposureMinBrightness = 1.0f;
	
	PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
	PostProcess->Settings.AutoExposureMaxBrightness = 2.0f;
	
	// Force Lumen GI and Reflections
	PostProcess->Settings.bOverride_ReflectionMethod = true;
	PostProcess->Settings.ReflectionMethod = EReflectionMethod::Lumen;
	
	PostProcess->Settings.bOverride_DynamicGlobalIlluminationMethod = true;
	PostProcess->Settings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	
	PostProcess->SetActive(true);
	
	UE_LOG(LogTemp, Warning, TEXT("📷 POSTPROCESS: Unbound=%d, BlendWeight=%.1f, Priority=%.1f, Lumen=%d"), 
		PostProcess->bUnbound, PostProcess->BlendWeight, PostProcess->Priority, 
		(int32)PostProcess->Settings.ReflectionMethod);

	/* ================================================================================
	 * NIAGARA STARFIELD CONFIGURATION
	 * ================================================================================ */
	if (StarfieldComponent && StarfieldNiagaraSystem)
	{
		StarfieldComponent->SetAsset(StarfieldNiagaraSystem);
		
		// Prevent Niagara scalability from culling particles
		StarfieldComponent->SetAllowScalability(false);
		StarfieldComponent->SetRenderingEnabled(true);
		
		StarfieldComponent->SetAutoActivate(true);
		StarfieldComponent->bAutoManageAttachment = false;
		StarfieldComponent->SetTickGroup(TG_DuringPhysics);
		
		UE_LOG(LogTemp, Warning, TEXT("⭐ STARFIELD: Configured, SphereRadius=%.0f cm"), StarSphereRadiusCm);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("⭐ STARFIELD: Waiting for NiagaraSystem assignment"));
	}
}

void AUniversalSkyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from TimeSubsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimeSubsystem* TimeSys = GI->GetSubsystem<UTimeSubsystem>())
		{
			TimeSys->OnSimTimeAdvanced.Remove(TimeAdvancedHandle);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AUniversalSkyActor::BeginPlay()
{
	Super::BeginPlay();

	// Load Niagara system if not assigned
	if (!StarfieldNiagaraSystem)
	{
		StarfieldNiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/SpecPacks/Space/NS_StarField.NS_StarField"));
		if (StarfieldNiagaraSystem)
		{
			UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Loaded StarfieldNiagaraSystem from /Game/SpecPacks/Space/NS_StarField"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor: Failed to load StarfieldNiagaraSystem. Ensure it exists at /Game/SpecPacks/Space/NS_StarField"));
		}
	}

	// Assign Niagara system if set (ApplyStarfield will handle data upload)
	if (StarfieldComponent && StarfieldNiagaraSystem)
	{
		StarfieldComponent->SetAsset(StarfieldNiagaraSystem);
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Assigned StarfieldNiagaraSystem to component"));		
		// Create and apply procedural star material
		UMaterial* StarMaterial = UMaterialCreator::CreateStarMaterial(
			TEXT("/Game/Core/Materials"),
			FName("M_StarProcedural"),
			1000.0f // Default brightness multiplier
		);
		
		if (StarMaterial)
		{
			UMaterialCreator::ApplyMaterialToNiagara(StarfieldComponent, StarMaterial);
		}	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor: StarfieldComponent or StarfieldNiagaraSystem not available. Assign StarfieldNiagaraSystem in editor."));
	}

	// Force skylight recapture at runtime to ensure sun is captured
    if (SkyLight)
    {
        SkyLight->RecaptureSky();
        UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Skylight recapture initiated"));
    }

    // TESTING: Set sun to 30-degree elevation for optimal menu scene visibility
    // This overrides solar system calculation until proper time initialization
    if (SunLight)
    {
        // Point sun 30 degrees above horizon for natural illumination
        // Pitch=-30 points downward from 30° above horizon
        // Yaw=45 positions sun from southeast for better shadowing
        FRotator TestSunRot(-30.0f, 45.0f, 0.0f);
        SunLight->SetWorldRotation(TestSunRot);
        
        // Reduced intensity prevents auto-exposure from darkening stars
        // 20k lux = comfortable outdoor daylight, not blinding
        SunLight->SetIntensity(20000.0f);
        
        FRotator SunRot = SunLight->GetComponentRotation();
        UE_LOG(LogTemp, Warning, TEXT("☀️ TEST SUN: Rotation=%s, Intensity=%.0f lux"), 
            *SunRot.ToString(), SunLight->Intensity);
    }

	// Subscribe to TimeSubsystem for event-driven updates
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimeSubsystem* TimeSys = GI->GetSubsystem<UTimeSubsystem>())
		{
			TimeAdvancedHandle = TimeSys->OnSimTimeAdvanced.AddUObject(this, &AUniversalSkyActor::OnTimeAdvanced);
			UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Subscribed to TimeSubsystem events"));
		}
	}

	// Initialize starfield BEFORE applying environment - why?
	ApplyStarfield();
	
	// Apply initial environment state
	ApplyEnvironment(CurrentMedium, CurrentWeather);
}

void AUniversalSkyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Only update starfield rotation in Tick (lightweight)
	// Sun/atmosphere/weather updates are event-driven via OnTimeAdvanced
	if (StarfieldUpdateRateHz <= 0.0f)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UniversalSkyActor: Tick Rotation StarfieldUpdateAccumulator hertz update rate=%.2f"), StarfieldUpdateRateHz);
		UpdateStarfieldRotation();
		return;
	}

	StarfieldUpdateAccumulator += DeltaSeconds;
	const float Period = 1.0f / StarfieldUpdateRateHz;
	if (StarfieldUpdateAccumulator >= Period)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UniversalSkyActor: Tick Rotation StarfieldUpdateAccumulator period=%.2f"), Period);
		StarfieldUpdateAccumulator = 0.0f;
		UpdateStarfieldRotation();
	}
}

void AUniversalSkyActor::OnTimeAdvanced(double NewSimTimeSeconds)
{
	// Throttle: Only update if time actually changed
	static double LastSimTime = -1.0;
	if (FMath::Abs(NewSimTimeSeconds - LastSimTime) < 0.01)
	{
		return; // Skip duplicate calls
	}
	LastSimTime = NewSimTimeSeconds;
	
	// Event-driven update when simulation time changes
	ApplyEnvironment(CurrentMedium, CurrentWeather);
	UE_LOG(LogTemp, Verbose, TEXT("UniversalSkyActor: Applied environment at SimTime=%.2f"), NewSimTimeSeconds);
}

void AUniversalSkyActor::ApplyEnvironment(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	// Cache for auto apply
	CurrentMedium = Medium;
	CurrentWeather = Weather;
	// Throttle spam - only log on actual changes
	static FRuntimeMediumSpec LastMedium;
	static FRuntimeWeatherState LastWeather;
	static bool bFirstRun = true;
	
	if (bFirstRun || 
	    FMath::Abs(LastMedium.Density - Medium.Density) > 0.001f ||
	    FMath::Abs(LastWeather.CloudCover01 - Weather.CloudCover01) > 0.01f)
	{
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Applying environment - Medium Density=%.3f, Pressure=%.1f Pa; Weather CloudCover=%.2f"), 
			Medium.Density, Medium.PressurePa, Weather.CloudCover01);
		LastMedium = Medium;
		LastWeather = Weather;
		bFirstRun = false;
	}

	ApplySun(Medium, Weather);
	ApplyAtmosphere(Medium, Weather);
	ApplyFog(Medium, Weather);
	ApplyClouds(Medium, Weather);
	ApplySkyLight(Medium, Weather);
}

void AUniversalSkyActor::ApplySun(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        // Query SolarSystemSubsystem for authoritative sun/moon directions
        if (USolarSystemSubsystem* SolarSys = GI->GetSubsystem<USolarSystemSubsystem>())
        {
            FSolarSystemState SolarState = SolarSys->GetSolarSystemState();

            // Use solar system's sun direction
            FVector SunDir = SolarState.SunDir_World.GetSafeNormal();

            // DirectionalLight points *from* light toward scene; use -SunDir
            const FRotator SunRot = UKismetMathLibrary::MakeRotFromX(-SunDir);
            SunLight->SetWorldRotation(SunRot);

            // Intensity: use SolarState illuminance or fallback to medium
            const float Irr = FMath::Max(0.0f, Medium.SolarIrradiance_Wm2);
            const float CloudDim = 1.0f - 0.75f * FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);
            const float StormDim = 1.0f - 0.5f * FMath::Clamp(Weather.Storm01, 0.0f, 1.0f);
            const float BaseIntensity = (SolarState.SunIlluminanceLux > 0.0f) ? SolarState.SunIlluminanceLux : (Irr * 10.0f);
            const float FinalIntensity = BaseIntensity * CloudDim * StormDim * SunIntensityScale;

            SunLight->SetIntensity(FinalIntensity);

            // Color temperature (rough proxy)
            const float T = Medium.TemperatureK;
            const float Kelvin = FMath::Clamp(6500.0f - (288.0f - T) * 10.0f, 2500.0f, 9000.0f);
            SunLight->SetTemperature(Kelvin);

            // Log sun state with component diagnostics
            static FVector LastLoggedSunDir = FVector::ZeroVector;
            static int32 SunUpdateCount = 0;
            if (FVector::Dist(SunDir, LastLoggedSunDir) > 0.01f || SunUpdateCount < 3)
            {
                UE_LOG(LogTemp, Warning, TEXT("☀️ SUN DIAGNOSTICS:"));
                UE_LOG(LogTemp, Warning, TEXT("  └─ Direction: %s"), *SunDir.ToCompactString());
                UE_LOG(LogTemp, Warning, TEXT("  └─ Rotation: %s"), *SunRot.ToCompactString());
                UE_LOG(LogTemp, Warning, TEXT("  └─ Intensity: %.0f lux (Base: %.0f, Cloud: %.2f, Storm: %.2f)"), FinalIntensity, BaseIntensity, CloudDim, StormDim);
                UE_LOG(LogTemp, Warning, TEXT("  └─ Temperature: %.0fK"), Kelvin);
                UE_LOG(LogTemp, Warning, TEXT("  └─ Visible: %d, Hidden: %d, CastShadows: %d"), 
                    SunLight->IsVisible(), SunLight->bHiddenInGame, SunLight->CastShadows);
                UE_LOG(LogTemp, Warning, TEXT("  └─ AtmosSunLight: %d, Component Active: %d"), 
                    SunLight->bAtmosphereSunLight, SunLight->IsActive());
                UE_LOG(LogTemp, Warning, TEXT("  └─ World Location: %s"), *SunLight->GetComponentLocation().ToString());
                LastLoggedSunDir = SunDir;
                SunUpdateCount++;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UniversalSkyActor: SolarSystemSubsystem not found"));
        }
    }
    SunLight->bUseTemperature = true;
}

void AUniversalSkyActor::ApplyAtmosphere(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	// If we’re in vacuum, atmosphere should be effectively off.
	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);	
	static int32 AtmosUpdateCount = 0;
	if (AtmosUpdateCount < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("🌍 ATMOSPHERE DIAGNOSTICS:"));
		UE_LOG(LogTemp, Warning, TEXT("  └─ Density: %.3f kg/m³"), Medium.Density);
		UE_LOG(LogTemp, Warning, TEXT("  └─ Pressure: %.1f Pa"), Medium.PressurePa);
		UE_LOG(LogTemp, Warning, TEXT("  └─ Vacuum Mode: %d"), bVacuum);
		UE_LOG(LogTemp, Warning, TEXT("  └─ Component Visible: %d, Active: %d"), SkyAtmosphere->IsVisible(), SkyAtmosphere->IsActive());
		AtmosUpdateCount++;
	}
	// SkyAtmosphere doesn’t have a single “enable” flag; we approximate by scaling density-related settings.
	// Rayleigh scattering ~ density proxy, Mie scattering ~ humidity/aerosols proxy.
	// CRITICAL: Cap atmosphere at 10% for space scenes to prevent sun/star absorption
	const float Density01 = bVacuum ? 0.0f : FMath::Clamp(Medium.Density / 1.225f, 0.0f, 0.1f); // Cap at 10% for space visibility
	const float Humidity01 = FMath::Clamp(Weather.Humidity01, 0.0f, 1.0f);

	// Further reduce scattering strength to prevent zenith path extinction
	const float AtmosStrength = Density01 * 0.3f; // Scale down to 30% of density for minimal absorption

	// These setters exist in UE5. If you hit compile issues due to version differences,
	// we’ll swap to direct property access or remove the calls.
	SkyAtmosphere->SetRayleighScatteringScale(AtmosStrength);
	SkyAtmosphere->SetMieScatteringScale(AtmosStrength * (0.25f + 0.75f * Humidity01));
	SkyAtmosphere->SetMieAbsorptionScale(AtmosStrength * (0.1f + 0.9f * Humidity01));

	// Ozone is planet-specific; keep modest for Earth baseline; later move into planet spec.
	SkyAtmosphere->SetOtherAbsorptionScale(bVacuum ? 0.0f : 1.0f);

	// Log only on significant changes (throttle to avoid spam)
	static float LastLoggedDensity = -1.0f;
	if (FMath::Abs(Medium.Density - LastLoggedDensity) > 0.01f)
	{
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor: Atmosphere updated - Density: %.3f, Pressure: %.1f Pa, AtmosStrength: %.3f"), 
			Medium.Density, Medium.PressurePa, AtmosStrength);
		LastLoggedDensity = Medium.Density;
	}
}

void AUniversalSkyActor::ApplyFog(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);

	// Fog mostly comes from moisture/particles; in vacuum it should be off.
	const float Fog01 = bVacuum ? 0.0f : FMath::Clamp(Weather.Fog01, 0.0f, 1.0f);
	const float Cloud01 = bVacuum ? 0.0f : FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);
	const float Storm01 = bVacuum ? 0.0f : FMath::Clamp(Weather.Storm01, 0.0f, 1.0f);

	// ExponentialHeightFog uses FogDensity in “engine units” (small values).
	const float TargetFogDensity =
		(0.001f * Fog01 + 0.0005f * Cloud01 + 0.0015f * Storm01) * FogIntensityScale;

	HeightFog->SetFogDensity(TargetFogDensity);

	// Volumetric fog scattering intensity scales perceived thickness
	const float VolumetricScattering = FMath::Lerp(0.2f, 2.0f, Fog01);
	HeightFog->SetVolumetricFogScatteringDistribution(0.2f);
	// “Intensity” proxy: extinction is the closest lever for “how much fog affects lighting”
    HeightFog->VolumetricFogExtinctionScale = VolumetricScattering; // try 0.1–10

    // Optional: keep these stable defaults
    HeightFog->VolumetricFogAlbedo = FColor::White;                 // or FLinearColor(1,1,1)
    HeightFog->VolumetricFogScatteringDistribution = 0.2f;          // 0 = isotropic, higher = forward scattering
}

void AUniversalSkyActor::ApplyClouds(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);

	// In vacuum there should be no clouds.
	const float Cloud01 = bVacuum ? 0.0f : FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);

	// VolumetricCloud density control is usually material-driven.
	// If you’re using the default engine volumetric cloud material, you can drive parameters via MID later.
	// For now we scale the component’s layer settings where available.
	VolumetricCloud->SetVisibility(!bVacuum);

	// Component has limited runtime knobs; treat this as a placeholder until you wire a MID.
	VolumetricCloud->LayerBottomAltitude = FMath::Lerp(50.0f, 80.0f, Cloud01); // km - High altitude clouds for space layer viewing
	VolumetricCloud->LayerHeight = FMath::Lerp(0.2f, 1.2f, Cloud01) * CloudDensityScale; // km
}

void AUniversalSkyActor::ApplySkyLight(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);
	const float Cloud01 = FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);

	// In vacuum you can still have skylight if you have stars/spacebox; keep it low by default.
	const float Base = bVacuum ? 0.05f : 1.0f;

	// Cloud cover reduces ambient a bit
	const float CloudDim = 1.0f - 0.5f * Cloud01;

	SkyLight->SetIntensity(Base * CloudDim * SkyLightIntensityScale);

	// Optionally force a recapture occasionally; realtime capture already does this.
	SkyLight->RecaptureSky();  // avoid spamming
}

void AUniversalSkyActor::ApplyStarfield()
{
	// Guard rails: validate component and system
	if (!StarfieldComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarfieldComponent is null"));
		return;
	}

	if (!StarfieldNiagaraSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarfieldNiagaraSystem not assigned. Please assign in editor."));
		return;
	}

	// Ensure system asset is assigned (idempotent)
	if (StarfieldComponent->GetAsset() != StarfieldNiagaraSystem)
	{
		StarfieldComponent->SetAsset(StarfieldNiagaraSystem);
		bStarfieldInitialized = false; // Force re-push if asset changed
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Assigned StarfieldNiagaraSystem"));
	}

	// CRITICAL: Activate component BEFORE setting parameters
	// Parameters don't exist until the system is instantiated
	if (!StarfieldComponent->IsActive())
	{
		StarfieldComponent->Activate(true);
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Starfield component activated"));
	}

	// Get game instance and star catalog subsystem
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - GameInstance is null"));
		return;
	}

	UStarCatalogSubsystem* StarSys = GI->GetSubsystem<UStarCatalogSubsystem>();
	if (!StarSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarCatalogSubsystem not available"));
		return;
	}

	// Ensure star data is loaded (idempotent - won't reload if already loaded)
	if (!StarSys->EnsureLoaded())
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - Failed to load star catalog"));
		return;
	}

	const TArray<FStarRecord>& Stars = StarSys->GetStars();
	const int32 TotalStarCount = Stars.Num();
	if (TotalStarCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - No stars available from catalog"));
		// Set empty arrays to clear starfield
		StarfieldComponent->SetVariableInt(FName(TEXT("User.StarCount")), 0);
		return;
	}

	// Build filtered arrays with magnitude culling
	// Only rebuild if not yet initialized or if configuration changed
	if (!bStarfieldInitialized || CachedStarCount != TotalStarCount)
	{
		/* ================================================================================
		 * STARFIELD DATA PIPELINE: C++ → NIAGARA
		 * ================================================================================
		 * We generate 3 parallel arrays for Niagara consumption:
		 *
		 * 1. StarPositions (TArray<FVector>) → User.StarPositions (Niagara Array Position)
		 *    - Absolute positions in local space (normalized dir × StarSphereRadius)
		 *    - Used by particle spawn to place sprites on celestial sphere
		 *
		 * 2. StarMagnitudes (TArray<float>) → User.StarMagnitudes (Niagara Array Float)
		 *    - Astronomical apparent magnitude (lower = brighter; Sun = -26.7, dimmest naked eye = +6.0)
		 *    - Used to drive sprite brightness/alpha (NOT size)
		 *    - Requires remapping: mag → brightness via inverse exponential
		 *
		 * 3. StarColors (TArray<FLinearColor>) → User.StarColors (Niagara Array Color)
		 *    - Processed RGB colors derived from B-V index (blue stars ~-0.3, Sun ~+0.65, red giants ~+2.0)
		 *    - Used for sprite color by spectral class (O, B, A, F, G, K, M types)
		 * ================================================================================ */

		TArray<FVector> StarPositions;       // Absolute positions on sphere
		TArray<float> StarMagnitudes;        // Apparent magnitude for brightness
		TArray<FLinearColor> StarColors;     // Processed RGB colors from B-V index
		
		// Reserve max possible size (all stars)
		StarPositions.Reserve(TotalStarCount);
		StarMagnitudes.Reserve(TotalStarCount);
		StarColors.Reserve(TotalStarCount);

		// Filter and build arrays
		int32 CulledCount = 0;
		for (const FStarRecord& Star : Stars)
		{
			// Apply magnitude culling (dimmer stars have higher magnitude)
			if (Star.Mag > MaxVisibleMagnitude)
			{
				CulledCount++;
				continue;
			}

			// Convert FVector3f to FVector and normalize
			FVector Dir = FVector(Star.DirEquatorial.X, Star.DirEquatorial.Y, Star.DirEquatorial.Z);
			FVector NormalizedDir = Dir.GetSafeNormal();
			if (NormalizedDir.IsNearlyZero())
			{
				NormalizedDir = FVector::ForwardVector; // Fallback to avoid zero vectors
				UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::ApplyStarfield - Near-zero star direction detected, using fallback"));
			}

			// Scale normalized direction by sphere radius to get absolute position
			FVector Position = NormalizedDir * StarSphereRadiusCm;

			// Convert B-V color index to RGB color
			FLinearColor StarColor = BVIndexToColor(Star.CI);

			// Add to arrays (indices must stay aligned!)
			StarPositions.Add(Position);
			StarMagnitudes.Add(Star.Mag);
			StarColors.Add(StarColor);
		}

		const int32 VisibleStarCount = StarPositions.Num();

		// Production-ready logging
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Filtered %d/%d stars (culled %d dimmer than mag %.1f)"),
			VisibleStarCount, TotalStarCount, CulledCount, MaxVisibleMagnitude);

		// Push arrays to Niagara using UE 5.7 Data Interface Array Function Library
		if (VisibleStarCount > 0)
		{
			// CRITICAL: Use SetNiagaraArrayPosition for spatial data (not SetNiagaraArrayVector)
			// NOTE: Niagara converts User.ParameterName to User_ParameterName automatically
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(StarfieldComponent, FName(TEXT("User_StarPositions")), StarPositions);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(StarfieldComponent, FName(TEXT("User_StarMagnitudes")), StarMagnitudes);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(StarfieldComponent, FName(TEXT("User_StarColors")), StarColors);

			UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Pushed arrays to Niagara: Positions=%d, Magnitudes=%d, Colors=%d"),
				StarPositions.Num(), StarMagnitudes.Num(), StarColors.Num());
			
			// Debug: Log first 3 star data with RGB colors
			for (int32 i = 0; i < FMath::Min(3, StarPositions.Num()); i++)
			{
				UE_LOG(LogTemp, Log, TEXT("  Star[%d]: Pos=(%.1f, %.1f, %.1f), Mag=%.2f, Color=(R:%.2f G:%.2f B:%.2f)"),
					i, StarPositions[i].X, StarPositions[i].Y, StarPositions[i].Z, 
					StarMagnitudes[i], StarColors[i].R, StarColors[i].G, StarColors[i].B);
			}
		}

		// Set scalar user parameters to match filtered arrays
		// NOTE: Niagara converts dots to underscores in parameter names
		StarfieldComponent->SetVariableInt(FName(TEXT("User_StarCount")), VisibleStarCount);
		StarfieldComponent->SetVariableFloat(FName(TEXT("User_StarSphereRadius")), StarSphereRadiusCm);

		// Mark as initialized
		bStarfieldInitialized = true;
		CachedStarCount = TotalStarCount;

		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Starfield initialized successfully. VisibleStars=%d, SphereRadius=%.0f cm"),
			VisibleStarCount, StarSphereRadiusCm);
		
		// DIAGNOSTIC: Log Niagara parameter values to verify data reception
		bool bStarCountValid = false;
		bool bSphereRadiusValid = false;
		int32 VerifyStarCount = StarfieldComponent->GetVariableInt(FName(TEXT("User_StarCount")), bStarCountValid);
		float VerifySphereRadius = StarfieldComponent->GetVariableFloat(FName(TEXT("User_StarSphereRadius")), bSphereRadiusValid);
		
		UE_LOG(LogTemp, Warning, TEXT("🔍 NIAGARA DIAGNOSTICS:"));
		UE_LOG(LogTemp, Warning, TEXT("  └─ User_StarCount read back: %d (valid: %d, expected: %d)"), VerifyStarCount, bStarCountValid, VisibleStarCount);
		UE_LOG(LogTemp, Warning, TEXT("  └─ User_StarSphereRadius read back: %.1f (valid: %d, expected: %.1f)"), VerifySphereRadius, bSphereRadiusValid, StarSphereRadiusCm);
		UE_LOG(LogTemp, Warning, TEXT("  └─ Component Location: %s"), *StarfieldComponent->GetComponentLocation().ToString());
		UE_LOG(LogTemp, Warning, TEXT("  └─ Component Visibility: %d"), StarfieldComponent->IsVisible());
		UE_LOG(LogTemp, Warning, TEXT("  └─ Component World Scale: %s"), *StarfieldComponent->GetComponentScale().ToString());
		
		// Force component to acknowledge changes
		StarfieldComponent->ReinitializeSystem();
		UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Reinitialized Niagara system"));
		
		// CRITICAL: Force skylight recapture after initializing starfield and sun
		// This updates Lumen GI with procedural sky data
		if (SkyLight)
		{
			SkyLight->RecaptureSky();
			UE_LOG(LogTemp, Warning, TEXT("🌤️ SKYLIGHT RECAPTURED after starfield initialization"));
		}
	}

	// Verify component state
	UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::ApplyStarfield - Component Active=%d, Asset=%s"),
		StarfieldComponent->IsActive(),
		StarfieldComponent->GetAsset() ? *StarfieldComponent->GetAsset()->GetName() : TEXT("NULL"));

	// Update rotation once during initialization
	UpdateStarfieldRotation();

	/* ================================================================================
	 * NIAGARA WIRING TABLE: Photorealistic Starfield Setup
	 * ================================================================================
	 * This table describes how to configure your Niagara System (NS_StarField)
	 * to achieve physically accurate, real-time starfield rendering.
	 *
	 * REQUIRED USER PARAMETERS (exposed for C++ binding):
	 * ---------------------------------------------------
	 * Name                      Type              Purpose
	 * User.StarPositions        Position Array    Absolute positions on celestial sphere (cm)
	 * User.StarMagnitudes       Float Array       Apparent magnitude (lower = brighter)
	 * User.StarColorIndices     Float Array       B-V color index for spectral classification
	 * User.StarCount            Int               Number of visible stars (after culling)
	 * User.StarSphereRadius     Float             Sphere radius in cm (default 1mkm = 1000000)
	 * User.RotationAngle        Float             GMST rotation in degrees (updated per frame)
	 * -------------------------------------------------------------------------------
	 *
	 * ┌─────────────────────────────────────────────────────────────────────────────┐
	 * │ PERFORMANCE NOTES                                                           │
	 * └─────────────────────────────────────────────────────────────────────────────┘
	 * • For 5000+ stars, use GPU Compute Sim (GPUComputeSim) for better performance
	 * • Enable Niagara Culling by distance (cull stars beyond camera far plane)
	 * • Consider LOD: reduce StarCount based on camera distance or performance budget
	 * • Use material complexity view to ensure star material is lightweight
	 * • Profile with "stat Niagara" and "stat GPU" to measure overhead
	 *
	 * ┌─────────────────────────────────────────────────────────────────────────────┐
	 * │ SIDEREAL TIME ROTATION NOTES                                                │
	 * └─────────────────────────────────────────────────────────────────────────────┘
	 * • C++ updates User.RotationAngle every frame based on GMST (Greenwich Mean
	 *   Sidereal Time) from SolarSystemSubsystem
	 * • GMST tracks Earth's rotation relative to distant stars (23h 56m 4s period)
	 * • Rotation is applied in Update stack around celestial north pole (Z-axis)
	 * • Ensure actor's Local Space = FALSE so rotation is in world coordinates
	 * • If sky dome also rotates, ensure consistent rotation center/axis
	 * • For accuracy, verify celestial pole alignment with your world's +Z axis
	 * • Alternative: Use Emitter.Age × (360° / 86164s) for constant rotation without
	 *   depending on subsystem (less accurate but simpler for prototyping)
	 *
	 * ================================================================================ */
}

void AUniversalSkyActor::UpdateStarfieldRotation()
{
	if (!StarfieldComponent || !StarfieldComponent->IsActive())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	// Update rotation based on GMST (lightweight, called at StarfieldUpdateRateHz)
	if (USolarSystemSubsystem* SolarSys = GI->GetSubsystem<USolarSystemSubsystem>())
	{
		const double GMST = SolarSys->GetGMSTAngleRad();
		const float GMSTDegrees = FMath::RadiansToDegrees(static_cast<float>(GMST));
		// NOTE: Niagara converts dots to underscores in parameter names
		StarfieldComponent->SetVariableFloat(FName(TEXT("User_RotationAngle")), GMSTDegrees);
	}
}

FLinearColor AUniversalSkyActor::BVIndexToColor(float BV) const
{
	/* ================================================================================
	 * B-V COLOR INDEX TO RGB CONVERSION
	 * ================================================================================
	 * Physically accurate stellar colors based on B-V color index.
	 * B-V represents the difference between blue and visual magnitude.
	 * Temperature correlation: BV = -0.3 (~30000K) to BV = 2.0 (~3000K)
	 *
	 * Spectral Classifications (Harvard System):
	 * O-type: BV < -0.20  (Hot blue stars: Rigel, Zeta Puppis)
	 * B-type: BV < 0.00   (Blue-white stars: Spica, Achernar)
	 * A-type: BV < 0.30   (White stars: Vega, Sirius)
	 * F-type: BV < 0.60   (Yellow-white stars: Procyon, Canopus)
	 * G-type: BV < 0.80   (Yellow stars: Sun, Alpha Centauri A)
	 * K-type: BV < 1.20   (Orange stars: Arcturus, Aldebaran)
	 * M-type: BV >= 1.20  (Red stars: Betelgeuse, Antares)
	 *
	 * RGB values derived from blackbody radiation curves and atmospheric effects.
	 * Alpha = 1.0 for all stars (emissive material handles brightness via magnitude).
	 * ================================================================================ */
	
	if (BV < -0.20f)  // O-type: Hot blue stars
		return FLinearColor(0.61f, 0.73f, 1.00f, 1.0f);
	
	if (BV < 0.00f)   // B-type: Blue-white stars
		return FLinearColor(0.78f, 0.87f, 1.00f, 1.0f);
	
	if (BV < 0.30f)   // A-type: White stars
		return FLinearColor(0.96f, 0.97f, 1.00f, 1.0f);
	
	if (BV < 0.60f)   // F-type: Yellow-white stars
		return FLinearColor(1.00f, 0.98f, 0.92f, 1.0f);
	
	if (BV < 0.80f)   // G-type: Yellow stars (like our Sun)
		return FLinearColor(1.00f, 0.93f, 0.74f, 1.0f);
	
	if (BV < 1.20f)   // K-type: Orange stars
		return FLinearColor(1.00f, 0.82f, 0.56f, 1.0f);
	
	// M-type: Red stars (BV >= 1.20)
	return FLinearColor(1.00f, 0.65f, 0.38f, 1.0f);
}

void AUniversalSkyActor::SetStarfieldBounds(float BoundsRadiusCm)
{
	if (!StarfieldComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniversalSkyActor::SetStarfieldBounds - StarfieldComponent is null"));
		return;
	}

	// Set bounds using FBoxSphereBounds to prevent frustum culling
	// For LWC (Large World Coordinates) with 8km x 21km island and interplanetary travel,
	// we need generous bounds to ensure starfield is always visible
	// Starfield acts as a skybox - disable distance culling so it's always rendered
	StarfieldComponent->SetCullDistance(0.0f); // 0 = never cull
	
	UE_LOG(LogTemp, Log, TEXT("UniversalSkyActor::SetStarfieldBounds - Disabled culling for starfield (cull distance = 0)"));
}



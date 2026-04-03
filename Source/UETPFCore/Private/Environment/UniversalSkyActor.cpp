// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Environment/UniversalSkyActor.h"
#include "Log.h"

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

// Actor includes for reference-based pattern
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Materials/MaterialCreator.h"

/* ================================================================================
 * CONSTRUCTOR: MINIMAL SETUP
 * ================================================================================
 * Manager pattern: Only create owned components (Root, Starfield).
 * Atmospheric actors are referenced, not owned - they must be placed separately.
 * ================================================================================ */
AUniversalSkyActor::AUniversalSkyActor()
{
	// MINIMAL TICK ARCHITECTURE: Tick only checks dirty flags, does no work if clean
	// This avoids timer queuing issues while keeping game thread work minimal
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f; // 10 Hz max - not per-frame

	// Root component - starfield attaches to this
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Niagara Starfield - the ONLY owned component for atmospheric rendering
	// Starfield is purely visual and doesn't interact with UE5's atmospheric pipeline
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

	// Configure starfield component
	if (StarfieldComponent && StarfieldNiagaraSystem)
	{
		StarfieldComponent->SetAsset(StarfieldNiagaraSystem);

		// Prevent Niagara scalability from culling particles
		StarfieldComponent->SetAllowScalability(false);
		StarfieldComponent->SetRenderingEnabled(true);

		StarfieldComponent->SetAutoActivate(true);
		StarfieldComponent->bAutoManageAttachment = false;
		StarfieldComponent->SetTickGroup(TG_DuringPhysics);

		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Starfield configured (SphereRadius=%.0f cm)"), StarSphereRadiusCm);
	}
	else
	{
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Starfield waiting for NiagaraSystem assignment"));
	}

	UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Constructed as MANAGER (no owned atmospheric components)"));
	UE_LOG(LogUETPFCore, Log, TEXT("  └─ Assign actor references in Details panel after placing actors in level"));
}

void AUniversalSkyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// No timers to clean up - using tick-based architecture
	Super::EndPlay(EndPlayReason);
}

/* ================================================================================
 * BEGINPLAY: SUBSYSTEM SUBSCRIPTION
 * ================================================================================
 * Subscribe to TimeSubsystem for automatic updates on time changes.
 * Validate actor references and log warnings for missing assignments.
 * ================================================================================ */
void AUniversalSkyActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::BeginPlay - Medium(Density=%.3f, Pressure=%.1f Pa), Weather(Cloud=%.2f, Humidity=%.2f)"),
        CurrentMedium.Density, CurrentMedium.PressurePa, CurrentWeather.CloudCover01, CurrentWeather.Humidity01);

    // Phase 1: Validate required references
    if (!ValidateRequiredReferences())
    {
        UE_LOG(LogUETPFCore, Error, TEXT("[ERROR] UniversalSkyActor: Missing required actor references"));
        UE_LOG(LogUETPFCore, Error, TEXT("   SETUP: 1) Place SkyAtmosphere, DirectionalLight, SkyLight in level"));
        UE_LOG(LogUETPFCore, Error, TEXT("          2) Assign references in UniversalSkyActor Details panel"));
        return;
    }

    // Phase 2: Initialize all atmospheric components and set linkage properties
    InitializeAtmosphericComponents();

    // Phase 3: Subscribe to subsystems
    SubscribeToSubsystems();

    // Phase 4: Apply initial environment state (MUST happen before render state registration)
    ApplyEnvironment(CurrentMedium, CurrentWeather);

    // Phase 5: Initialize starfield with star catalog data
    ApplyStarfield();

    // Phase 6: Register components with rendering thread AFTER properties are set
    RegisterComponentsWithRenderer();
    
    // Phase 7: Mark SkyLight for deferred recapture (non-blocking)
    // SetCaptureIsDirty schedules recapture for next frame instead of blocking immediately.
    // With Lumen: Recapture primarily affects non-Lumen fallback paths
    // Without Lumen: Provides ambient GI baseline after all components configured
    if (USkyLightComponent* SkyComp = GetSkyLightComponent())
    {
        SkyComp->SetCaptureIsDirty();
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: SkyLight marked dirty for deferred recapture"));
    }
}

/* ================================================================================
 * INITIALIZATION PHASE METHODS
 * ================================================================================
 * These methods are called from BeginPlay in a specific order:
 * 1. ValidateRequiredReferences - Check required actors assigned
 * 2. InitializeAtmosphericComponents - Activate and configure components
 * 3. RegisterComponentsWithRenderer - Mark render state dirty
 * 4. SubscribeToSubsystems - Subscribe to time updates
 * ================================================================================ */

bool AUniversalSkyActor::ValidateRequiredReferences()
{
    bool bValid = true;

    // Check required references
    if (!SunLightActor)
    {
        UE_LOG(LogUETPFCore, Error, TEXT("[ERROR] UniversalSkyActor: Missing SunLightActor reference"));
        bValid = false;
    }

    if (!SkyAtmosphereActor)
    {
        UE_LOG(LogUETPFCore, Error, TEXT("[ERROR] UniversalSkyActor: Missing SkyAtmosphereActor reference"));
        bValid = false;
    }

    if (!SkyLightActor)
    {
        UE_LOG(LogUETPFCore, Error, TEXT("[ERROR] UniversalSkyActor: Missing SkyLightActor reference"));
        bValid = false;
    }

    // Log info about optional references
    if (!VolumetricCloudActor)
    {
        UE_LOG(LogUETPFCore, Log, TEXT("[INFO] UniversalSkyActor: VolumetricCloudActor not assigned (optional)"));
    }

    if (!HeightFogActor)
    {
        UE_LOG(LogUETPFCore, Log, TEXT("[INFO] UniversalSkyActor: HeightFogActor not assigned (optional)"));
    }

    if (!PostProcessVolume)
    {
        UE_LOG(LogUETPFCore, Log, TEXT("[INFO] UniversalSkyActor: PostProcessVolume not assigned (optional)"));
    }

    if (bValid)
    {
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: All required actor references assigned"));
    }

    return bValid;
}

void AUniversalSkyActor::InitializeAtmosphericComponents()
{
    // Initialize DirectionalLight component
    if (UDirectionalLightComponent* SunComp = GetSunLightComponent())
    {
        if (!SunComp->IsActive())
        {
            SunComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated DirectionalLight component"));
        }

        // CRITICAL: Set atmosphere sun light properties for SkyAtmosphere linkage
        SunComp->bAtmosphereSunLight = true;
        SunComp->AtmosphereSunLightIndex = 0;
        SunComp->SetMobility(EComponentMobility::Movable);
        
        // CRITICAL: These must be set for Lumen GI to work properly
        SunComp->bAffectsWorld = true;
        SunComp->CastShadows = true;
        SunComp->CastDynamicShadows = true;
        SunComp->bCastVolumetricShadow = true;
        SunComp->bCastCloudShadows = true;
        SunComp->bCastShadowsOnClouds = true;
        
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: DirectionalLight configured (AtmosSunLight=true, Index=0, Movable, AffectsWorld=true)"));

        // Component properties will be set by ApplySun()
    }

    // Initialize SkyAtmosphere component
    if (USkyAtmosphereComponent* AtmosComp = GetSkyAtmosphereComponent())
    {
        if (!AtmosComp->IsActive())
        {
            AtmosComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated SkyAtmosphere component"));
        }

        // MANAGER PATTERN: Do NOT overwrite placed actor's settings
        // The placed SkyAtmosphere actor has correct settings from the editor
        // We only ensure it's active and visible - ApplyAtmosphere handles dynamic updates
        
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: SkyAtmosphere validated (BottomRadius=%.0f, AtmosHeight=%.0f)"),
            AtmosComp->BottomRadius, AtmosComp->AtmosphereHeight);
        // Component properties will be set by ApplyAtmosphere()
    }

    // Initialize SkyLight component
    if (USkyLightComponent* SkyComp = GetSkyLightComponent())
    {
        if (!SkyComp->IsActive())
        {
            SkyComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated SkyLight component"));
        }

        // ARCHITECTURE: No real-time capture, no manual RecaptureSky
        // - With Lumen: Lumen traces through SkyAtmosphere directly for sky GI
        // - Without Lumen: SkyLight provides static ambient (set once, no recapture)
        //
        // RecaptureSky is a synchronous GPU flush that blocks the game thread.
        // Lumen handles dynamic sky GI via raytracing - no capture needed.
        // For non-Lumen, initial capture happens automatically when component activates.
        SkyComp->bRealTimeCapture = false;
        SkyComp->SourceType = ESkyLightSourceType::SLS_CapturedScene;
        SkyComp->SetMobility(EComponentMobility::Movable);

        // PERFORMANCE: Skip lower hemisphere capture - ground GI handled by Lumen/lightmaps
        // This halves the SkyLight capture cost when recapture does occur
        SkyComp->bLowerHemisphereIsBlack = true;

        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: SkyLight configured (RealTimeCapture=false, LowerHemisphereBlack=true, Lumen handles sky GI)"));

        // Component properties will be set by ApplySkyLight()
    }

    // Initialize VolumetricCloud component (optional)
    if (UVolumetricCloudComponent* CloudComp = GetVolumetricCloudComponent())
    {
        if (!CloudComp->IsActive())
        {
            CloudComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated VolumetricCloud component"));
        }
        // Component properties will be set by ApplyClouds()
    }

    // Initialize ExponentialHeightFog component (optional)
    if (UExponentialHeightFogComponent* FogComp = GetHeightFogComponent())
    {
        if (!FogComp->IsActive())
        {
            FogComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated HeightFog component"));
        }
        // Component properties will be set by ApplyFog()
    }

    // Initialize PostProcess component (optional)
    if (UPostProcessComponent* PPComp = GetPostProcessComponent())
    {
        if (!PPComp->IsActive())
        {
            PPComp->SetActive(true);
            UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Activated PostProcess component"));
        }

        // CRITICAL: Set to Unbound to affect entire scene
        PPComp->bUnbound = true;
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: PostProcess set to Unbound (affects entire scene)"));

        // Component properties will be set by ApplyPostProcess()
    }
}

void AUniversalSkyActor::RegisterComponentsWithRenderer()
{
    // Validate world consistency - this check is critical even in shipping
    UWorld* MyWorld = GetWorld();
    if (SunLightActor && SkyAtmosphereActor)
    {
        UWorld* SunActorWorld = SunLightActor->GetWorld();
        UWorld* AtmosActorWorld = SkyAtmosphereActor->GetWorld();

        if (MyWorld != SunActorWorld || MyWorld != AtmosActorWorld)
        {
            UE_LOG(LogUETPFCore, Error, TEXT("UniversalSkyActor: World mismatch detected - atmospheric rendering will fail"));
            UE_LOG(LogUETPFCore, Error, TEXT("  Manager=%s, SunLight=%s, SkyAtmos=%s"),
                MyWorld ? *MyWorld->GetName() : TEXT("NULL"),
                SunActorWorld ? *SunActorWorld->GetName() : TEXT("NULL"),
                AtmosActorWorld ? *AtmosActorWorld->GetName() : TEXT("NULL"));
        }
    }

#if !UE_BUILD_SHIPPING
    // Development diagnostics - verify component registration and linkage
    UE_LOG(LogUETPFCore, Verbose, TEXT("UniversalSkyActor: Scene diagnostics (World=%s, PIE=%d)"),
        MyWorld ? *MyWorld->GetName() : TEXT("NULL"),
        MyWorld ? MyWorld->IsPlayInEditor() : 0);

    if (UDirectionalLightComponent* SunComp = GetSunLightComponent())
    {
        UE_LOG(LogUETPFCore, Verbose, TEXT("  DirectionalLight: Registered=%d, AtmosSunLight=%d, Index=%d"),
            SunComp->IsRegistered(), SunComp->bAtmosphereSunLight, SunComp->AtmosphereSunLightIndex);
    }

    if (USkyAtmosphereComponent* AtmosComp = GetSkyAtmosphereComponent())
    {
        UE_LOG(LogUETPFCore, Verbose, TEXT("  SkyAtmosphere: Registered=%d, Active=%d"),
            AtmosComp->IsRegistered(), AtmosComp->IsActive());
    }

    if (USkyLightComponent* SkyComp = GetSkyLightComponent())
    {
        UE_LOG(LogUETPFCore, Verbose, TEXT("  SkyLight: Registered=%d, RealTimeCapture=%d"),
            SkyComp->IsRegistered(), SkyComp->bRealTimeCapture);
    }
#endif
}

void AUniversalSkyActor::SubscribeToSubsystems()
{
    // MINIMAL TICK ARCHITECTURE: No timers, no subscriptions
    // Tick() checks dirty flags and samples subsystems when needed
    
    UGameInstance* GI = GetGameInstance();
    if (!GI) 
    { 
        UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor: GameInstance not available"));
        return; 
    }

    // Verify TimeSubsystem exists (we'll sample it in Tick)
    if (UTimeSubsystem* TimeSub = GI->GetSubsystem<UTimeSubsystem>())
    {
        UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: TimeSubsystem available for sampling"));
    }
    else
    {
        UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor: TimeSubsystem not available"));
    }
    
    UE_LOG(LogUETPFCore, Log, TEXT("[OK] UniversalSkyActor: Tick-based architecture configured (TickInterval=%.2fs)"),
        PrimaryActorTick.TickInterval);
}

/* ================================================================================
 * COMPONENT GETTER METHODS
 * ================================================================================
 * Safe accessors that extract components from referenced actors.
 * Return nullptr if actor not assigned or component not found.
 * ================================================================================ */

UDirectionalLightComponent* AUniversalSkyActor::GetSunLightComponent() const
{
	if (!SunLightActor)
	{
		return nullptr;
	}
	return Cast<UDirectionalLightComponent>(SunLightActor->GetLightComponent());
}

USkyAtmosphereComponent* AUniversalSkyActor::GetSkyAtmosphereComponent() const
{
	if (!SkyAtmosphereActor)
	{
		return nullptr;
	}
	return SkyAtmosphereActor->GetComponentByClass<USkyAtmosphereComponent>();
}

USkyLightComponent* AUniversalSkyActor::GetSkyLightComponent() const
{
	if (!SkyLightActor)
	{
		return nullptr;
	}
	return SkyLightActor->GetLightComponent();
}

UVolumetricCloudComponent* AUniversalSkyActor::GetVolumetricCloudComponent() const
{
	if (!VolumetricCloudActor)
	{
		return nullptr;
	}
	return VolumetricCloudActor->GetComponentByClass<UVolumetricCloudComponent>();
}

UExponentialHeightFogComponent* AUniversalSkyActor::GetHeightFogComponent() const
{
	if (!HeightFogActor)
	{
		return nullptr;
	}
	return HeightFogActor->FindComponentByClass<UExponentialHeightFogComponent>();
}

UPostProcessComponent* AUniversalSkyActor::GetPostProcessComponent() const
{
	if (!PostProcessVolume)
	{
		return nullptr;
	}
	return PostProcessVolume->GetComponentByClass<UPostProcessComponent>();
}

/* ================================================================================
 * TICK - MINIMAL DIRTY FLAG ARCHITECTURE
 * ================================================================================
 * Tick runs at 10Hz (TickInterval=0.1s), NOT per-frame.
 * Only samples time and applies environment if dirty flags are set.
 * No heavy work happens here - just flag checks and lightweight component updates.
 * 
 * ELIMINATED ALL BLOCKING OPERATIONS:
 * - No RecaptureSky (Lumen handles sky GI via raytracing)
 * - No RecreateRenderState (let deferred update handle it)
 * - No timer queues (timers accumulate during frame stalls)
 * ================================================================================ */

void AUniversalSkyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Sample current simulation time - mark sun dirty if time changed significantly
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimeSubsystem* TimeSub = GI->GetSubsystem<UTimeSubsystem>())
		{
			const double CurrentSimTime = TimeSub->GetSimTimeSeconds();
			// Sun moves ~0.25° per minute, update if time changed by >1 second
			if (FMath::Abs(CurrentSimTime - LastSampledSimTime) > 1.0)
			{
				bSunDirty = true;
				LastSampledSimTime = CurrentSimTime;
			}
		}
	}

	// Only apply environment if something is dirty
	if (bSunDirty || bAtmosphereDirty || bFogDirty || bCloudsDirty || bSkyLightDirty || bPostProcessDirty)
	{
		ApplyEnvironment(CurrentMedium, CurrentWeather);
	}

	// Update starfield rotation (very cheap - just sets a component rotation)
	UpdateStarfieldRotation();
}

void AUniversalSkyActor::ApplyEnvironment(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	// Cache for auto apply
	CurrentMedium = Medium;
	CurrentWeather = Weather;

	// Detect changes and set dirty flags appropriately
	const bool bMediumChanged = FMath::Abs(LastAppliedMedium.Density - Medium.Density) > 0.001f;
	const bool bWeatherChanged = FMath::Abs(LastAppliedWeather.CloudCover01 - Weather.CloudCover01) > 0.01f ||
	                             FMath::Abs(LastAppliedWeather.Fog01 - Weather.Fog01) > 0.01f;
	
	if (bMediumChanged)
	{
		bAtmosphereDirty = true;
		bFogDirty = true;
		bCloudsDirty = true;
		bSkyLightDirty = true;
		bPostProcessDirty = true;
	}
	
	if (bWeatherChanged)
	{
		bFogDirty = true;
		bCloudsDirty = true;
		bSkyLightDirty = true;
		bPostProcessDirty = true;
	}

	// Throttle spam - only log on actual changes
	if (bFirstEnvironmentApply || bMediumChanged || bWeatherChanged)
	{
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Applying environment - Medium Density=%.3f, Pressure=%.1f Pa; Weather CloudCover=%.2f"),
			Medium.Density, Medium.PressurePa, Weather.CloudCover01);
		LastAppliedMedium = Medium;
		LastAppliedWeather = Weather;
		bFirstEnvironmentApply = false;
	}

	// Only call Apply methods if their dirty flags are set
	// Each Apply method checks its own dirty flag and clears it after update
	if (bSunDirty) { ApplySun(Medium, Weather); }
	if (bAtmosphereDirty) { ApplyAtmosphere(Medium, Weather); }
	if (bFogDirty) { ApplyFog(Medium, Weather); }
	if (bCloudsDirty) { ApplyClouds(Medium, Weather); }
	if (bSkyLightDirty) { ApplySkyLight(Medium, Weather); }
	if (bPostProcessDirty) { ApplyPostProcess(Medium, Weather); }
}

void AUniversalSkyActor::ApplySun(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	if (!bSunDirty) { return; } // Skip if nothing changed
	
	UDirectionalLightComponent* SunLight = GetSunLightComponent();
	if (!SunLight) { return; } // Silently skip if not assigned

	if (UGameInstance* GI = GetGameInstance())
	{
		// Query SolarSystemSubsystem for authoritative sun/moon directions
		if (USolarSystemSubsystem* SolarSys = GI->GetSubsystem<USolarSystemSubsystem>())
		{
			FSolarSystemState SolarState = SolarSys->GetSolarSystemState();

			// Use sun direction from subsystem (ECI frame for now, transform TBD)
			FVector SunDir = SolarState.SunDir_World.GetSafeNormal();
			
			// Validate sun direction - if zero or invalid, use reasonable default
			if (SunDir.IsNearlyZero() || !SunDir.IsNormalized())
			{
				UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor: Invalid sun direction from subsystem, using default"));
				SunDir = FVector(0.5f, 0.5f, 0.707f).GetSafeNormal();  // ~45° elevation fallback
			}

			// Calculate intensity
			const float Irr = FMath::Max(0.0f, Medium.SolarIrradiance_Wm2);
			const float CloudDim = 1.0f - 0.75f * FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);
			const float StormDim = 1.0f - 0.5f * FMath::Clamp(Weather.Storm01, 0.0f, 1.0f);
			const float BaseIntensity = (SolarState.SunIlluminanceLux > 0.0f) ? SolarState.SunIlluminanceLux : (Irr * 10.0f);
			const float FinalIntensity = BaseIntensity * CloudDim * StormDim * SunIntensityScale;

			// Check if values actually changed (threshold-based to avoid floating point noise)
			const float DirDelta = FVector::Dist(SunDir, CachedSunDirection);
			const float IntensityDelta = FMath::Abs(FinalIntensity - CachedSunIntensity);
			
			// Only update component if values changed significantly
			if (DirDelta > 0.001f || IntensityDelta > 1.0f || CachedSunIntensity < 0.0f)
			{
				// DirectionalLight points *from* light toward scene; use -SunDir
				const FRotator SunRot = UKismetMathLibrary::MakeRotFromX(-SunDir);
				SunLight->SetWorldRotation(SunRot);
				SunLight->SetIntensity(FinalIntensity);

				// Color temperature (rough proxy)
				const float T = Medium.TemperatureK;
				const float Kelvin = FMath::Clamp(6500.0f - (288.0f - T) * 10.0f, 2500.0f, 9000.0f);
				SunLight->SetTemperature(Kelvin);
				SunLight->bUseTemperature = true;

				// Update cache
				CachedSunDirection = SunDir;
				CachedSunIntensity = FinalIntensity;

#if !UE_BUILD_SHIPPING
				// Development diagnostics: Log sun state for first 3 updates
				if (SunDiagnosticCount < 3)
				{
					UE_LOG(LogUETPFCore, Verbose, TEXT("Sun: Dir=%s, Intensity=%.0f lux, Temp=%.0fK"),
						*SunDir.ToCompactString(), FinalIntensity, Kelvin);
					SunDiagnosticCount++;
				}
#endif
			}
			
			// Mark clean after processing
			bSunDirty = false;
		}
		else
		{
			UE_LOG(LogUETPFCore, Error, TEXT("UniversalSkyActor: SolarSystemSubsystem not found"));
		}
	}
}

void AUniversalSkyActor::ApplyAtmosphere(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{	USkyAtmosphereComponent* SkyAtmosphere = GetSkyAtmosphereComponent();
	if (!SkyAtmosphere) { return; } // Silently skip if not assigned
	// If we’re in vacuum, atmosphere should be effectively off.
	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);	
	
#if !UE_BUILD_SHIPPING
	// Development diagnostics: Log atmosphere state for first 3 updates
	if (AtmosDiagnosticCount < 3)
	{
		UE_LOG(LogUETPFCore, Verbose, TEXT("Atmosphere: Density=%.3f kg/m³, Pressure=%.1f Pa, Vacuum=%d"),
			Medium.Density, Medium.PressurePa, bVacuum);
		AtmosDiagnosticCount++;
	}
#endif
	// SkyAtmosphere doesn't have a single "enable" flag; we approximate by scaling density-related settings.
	// Rayleigh scattering ~ density proxy, Mie scattering ~ humidity/aerosols proxy.
	// For Earth density (1.225), use full strength. For thinner atmospheres, scale down.
	// For denser atmospheres (>Earth), cap to prevent over-saturation.
	const float Density01 = bVacuum ? 0.0f : FMath::Clamp(Medium.Density / 1.225f, 0.0f, 2.0f);
	const float Humidity01 = FMath::Clamp(Weather.Humidity01, 0.0f, 1.0f);

	// Earth atmosphere (Density01 = 1.0) should render with full UE5 default strength
	// Atmosphere strength scales linearly with density up to 2x Earth density
	const float AtmosStrength = Density01;

	// These setters exist in UE5. If you hit compile issues due to version differences,
	// we'll swap to direct property access or remove the calls.
	const float RayleighScale = AtmosStrength;
	const float MieScale = AtmosStrength * (0.25f + 0.75f * Humidity01);
	const float MieAbsorption = AtmosStrength * (0.1f + 0.9f * Humidity01);

	SkyAtmosphere->SetRayleighScatteringScale(RayleighScale);
	SkyAtmosphere->SetMieScatteringScale(MieScale);
	SkyAtmosphere->SetMieAbsorptionScale(MieAbsorption);

	// Ozone is planet-specific; keep modest for Earth baseline; later move into planet spec.
	SkyAtmosphere->SetOtherAbsorptionScale(bVacuum ? 0.0f : 1.0f);

#if !UE_BUILD_SHIPPING
	// One-time development diagnostic: Verify scattering configuration
	if (!bScatteringLogged)
	{
		UE_LOG(LogUETPFCore, Verbose, TEXT("Atmosphere scattering: Rayleigh=%.3f, Mie=%.3f, Absorption=%.3f"),
			RayleighScale, MieScale, MieAbsorption);
		bScatteringLogged = true;
	}
#endif

	// Log only on significant changes (uses member variable, resets per PIE)
	if (FMath::Abs(Medium.Density - LastLoggedDensity) > 0.01f)
	{
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Atmosphere updated - Density: %.3f, Pressure: %.1f Pa, AtmosStrength: %.3f"),
			Medium.Density, Medium.PressurePa, AtmosStrength);
		LastLoggedDensity = Medium.Density;
	}
	
	bAtmosphereDirty = false;
}

void AUniversalSkyActor::ApplyFog(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{	UExponentialHeightFogComponent* HeightFog = GetHeightFogComponent();
	if (!HeightFog) { return; } // Silently skip if not assigned
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
    
    bFogDirty = false;
}

void AUniversalSkyActor::ApplyClouds(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{	UVolumetricCloudComponent* VolumetricCloud = GetVolumetricCloudComponent();
	if (!VolumetricCloud) { return; } // Silently skip if not assigned
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
	
	bCloudsDirty = false;
}

void AUniversalSkyActor::ApplySkyLight(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	USkyLightComponent* SkyLight = GetSkyLightComponent();
	if (!SkyLight) { return; } // Silently skip if not assigned

	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);
	const float Cloud01 = FMath::Clamp(Weather.CloudCover01, 0.0f, 1.0f);

	// SkyLight intensity should be significant for proper ambient lighting
	// In atmosphere: base intensity ~1.0 (engine default)
	// In vacuum: reduced to ~0.3 (stars/space provides some ambient)
	const float Base = bVacuum ? 0.3f : 1.0f;

	// Cloud cover reduces ambient light slightly
	const float CloudDim = 1.0f - 0.3f * Cloud01;

	SkyLight->SetIntensity(Base * CloudDim * SkyLightIntensityScale);

	// ARCHITECTURE: RecaptureSky() is handled by OnSkyRecaptureTimer (1Hz)
	// With Lumen: Recapture isn't strictly needed - Lumen traces through SkyAtmosphere
	// Without Lumen: Timer-based recapture provides ambient updates without frame stalls
	// 
	// We do NOT call RecaptureSky() here because:
	// 1. It's a synchronous GPU flush (40+ms stall)
	// 2. Lumen handles sky GI via raytracing
	// 3. Timer provides 1Hz updates for non-Lumen platforms
	
	bSkyLightDirty = false;
}

void AUniversalSkyActor::ApplyPostProcess(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather)
{
	UPostProcessComponent* PostProcess = GetPostProcessComponent();
	if (!PostProcess) { return; } // Silently skip if not assigned

	// Post-process effects driven by weather and environment
	// Example: Adjust exposure, color grading, vignette based on conditions

	const bool bVacuum = (Medium.Density <= KINDA_SMALL_NUMBER) || (Medium.PressurePa <= 1.0f);
	const float Storm01 = FMath::Clamp(Weather.Storm01, 0.0f, 1.0f);
	const float Fog01 = FMath::Clamp(Weather.Fog01, 0.0f, 1.0f);

	// Storm conditions: darken, increase contrast
	// Fog conditions: reduce contrast, add haze
	// Vacuum: neutral post-processing

	// Auto-exposure adjustment based on atmospheric density and weather
	// Dense atmosphere with fog: brighter compensation
	// Storm: darker, more dramatic
	const float ExposureCompensation = bVacuum ? 0.0f :
		FMath::Lerp(0.0f, -1.0f, Storm01) + FMath::Lerp(0.0f, 0.5f, Fog01);

	// Set exposure bias (requires PostProcessVolume with Exposure settings exposed)
	// Note: Direct property access may vary by UE version
	// PostProcess->Settings.AutoExposureBias = ExposureCompensation;

	// Log initial setup (uses member variable, resets per PIE)
	if (!bPostProcessLogged)
	{
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: PostProcess configured (exposure compensation ready)"));
		bPostProcessLogged = true;
	}

	// Additional post-process effects can be added here:
	// - Vignette intensity based on storm
	// - Color grading LUT based on atmosphere composition
	// - Bloom intensity based on solar irradiance
	
	bPostProcessDirty = false;
}

void AUniversalSkyActor::ApplyStarfield()
{
	// Guard rails: validate component and system
	if (!StarfieldComponent)
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarfieldComponent is null"));
		return;
	}

	if (!StarfieldNiagaraSystem)
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarfieldNiagaraSystem not assigned. Please assign in editor."));
		return;
	}

	// Ensure system asset is assigned (idempotent)
	if (StarfieldComponent->GetAsset() != StarfieldNiagaraSystem)
	{
		StarfieldComponent->SetAsset(StarfieldNiagaraSystem);
		bStarfieldInitialized = false; // Force re-push if asset changed
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::ApplyStarfield - Assigned StarfieldNiagaraSystem"));
	}

	// CRITICAL: Activate component BEFORE setting parameters
	// Parameters don't exist until the system is instantiated
	if (!StarfieldComponent->IsActive())
	{
		StarfieldComponent->Activate(true);
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::ApplyStarfield - Starfield component activated"));
	}

	// Get game instance and star catalog subsystem
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - GameInstance is null"));
		return;
	}

	UStarCatalogSubsystem* StarSys = GI->GetSubsystem<UStarCatalogSubsystem>();
	if (!StarSys)
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - StarCatalogSubsystem not available"));
		return;
	}

	// Ensure star data is loaded (idempotent - won't reload if already loaded)
	if (!StarSys->EnsureLoaded())
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - Failed to load star catalog"));
		return;
	}

	const TArray<FStarRecord>& Stars = StarSys->GetStars();
	const int32 TotalStarCount = Stars.Num();
	if (TotalStarCount <= 0)
	{
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - No stars available from catalog"));
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
				UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::ApplyStarfield - Near-zero star direction detected, using fallback"));
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
		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::ApplyStarfield - Filtered %d/%d stars (culled %d dimmer than mag %.1f)"),
			VisibleStarCount, TotalStarCount, CulledCount, MaxVisibleMagnitude);

		// Push arrays to Niagara using UE 5.7 Data Interface Array Function Library
		if (VisibleStarCount > 0)
		{
			// CRITICAL: Use SetNiagaraArrayPosition for spatial data (not SetNiagaraArrayVector)
			// NOTE: Niagara converts User.ParameterName to User_ParameterName automatically
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(StarfieldComponent, FName(TEXT("User_StarPositions")), StarPositions);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(StarfieldComponent, FName(TEXT("User_StarMagnitudes")), StarMagnitudes);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(StarfieldComponent, FName(TEXT("User_StarColors")), StarColors);

			UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: Pushed %d stars to Niagara"),
				StarPositions.Num());

#if !UE_BUILD_SHIPPING
			// Development diagnostic: Sample star data
			for (int32 i = 0; i < FMath::Min(3, StarPositions.Num()); i++)
			{
				UE_LOG(LogUETPFCore, Verbose, TEXT("  Star[%d]: Pos=(%.0f, %.0f, %.0f), Mag=%.2f, RGB=(%.2f, %.2f, %.2f)"),
					i, StarPositions[i].X, StarPositions[i].Y, StarPositions[i].Z,
					StarMagnitudes[i], StarColors[i].R, StarColors[i].G, StarColors[i].B);
			}
#endif
		}

		// Set scalar user parameters to match filtered arrays
		// NOTE: Niagara converts dots to underscores in parameter names
		StarfieldComponent->SetVariableInt(FName(TEXT("User_StarCount")), VisibleStarCount);
		StarfieldComponent->SetVariableFloat(FName(TEXT("User_StarSphereRadius")), StarSphereRadiusCm);

		// Mark as initialized
		bStarfieldInitialized = true;
		CachedStarCount = TotalStarCount;

		UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::ApplyStarfield - Starfield initialized successfully. VisibleStars=%d, SphereRadius=%.0f cm"),
			VisibleStarCount, StarSphereRadiusCm);
		
		// Force component to acknowledge changes
		StarfieldComponent->ReinitializeSystem();

#if !UE_BUILD_SHIPPING
		// Development diagnostic: Verify Niagara parameter reception
		bool bStarCountValid = false;
		bool bSphereRadiusValid = false;
		int32 VerifyStarCount = StarfieldComponent->GetVariableInt(FName(TEXT("User_StarCount")), bStarCountValid);
		float VerifySphereRadius = StarfieldComponent->GetVariableFloat(FName(TEXT("User_StarSphereRadius")), bSphereRadiusValid);

		UE_LOG(LogUETPFCore, Verbose, TEXT("Starfield Niagara: StarCount=%d (valid=%d), SphereRadius=%.0f (valid=%d)"),
			VerifyStarCount, bStarCountValid, VerifySphereRadius, bSphereRadiusValid);
#endif
		
		// Mark SkyLight for deferred recapture after starfield initialization
		// SetCaptureIsDirty is non-blocking; actual recapture happens next frame
		if (USkyLightComponent* SkyLight = GetSkyLightComponent())
		{
			SkyLight->SetCaptureIsDirty();
			UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor: SkyLight marked dirty after starfield initialization"));
		}
	}

	// Verify component state
	UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::ApplyStarfield - Component Active=%d, Asset=%s"),
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
		UE_LOG(LogUETPFCore, Warning, TEXT("UniversalSkyActor::SetStarfieldBounds - StarfieldComponent is null"));
		return;
	}

	// Set bounds using FBoxSphereBounds to prevent frustum culling
	// For LWC (Large World Coordinates) with 8km x 21km island and interplanetary travel,
	// we need generous bounds to ensure starfield is always visible
	// Starfield acts as a skybox - disable distance culling so it's always rendered
	StarfieldComponent->SetCullDistance(0.0f); // 0 = never cull
	
	UE_LOG(LogUETPFCore, Log, TEXT("UniversalSkyActor::SetStarfieldBounds - Disabled culling for starfield (cull distance = 0)"));
}



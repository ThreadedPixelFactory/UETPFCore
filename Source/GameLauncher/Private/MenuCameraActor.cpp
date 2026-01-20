// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MenuCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"

AMenuCameraActor::AMenuCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics; // Update before physics for smoothest camera

	// Create root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(Root);

	// Create camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->SetFieldOfView(90.0f); // Realistic first-person cinematic FOV
	
	// Configure far clip plane for large-scale scenes
	CameraComponent->OrthoFarClipPlane = 2000000.0f; // 20 km
	
	// ========================================
	// LUMEN-OPTIMIZED POST-PROCESS SETTINGS
	// ========================================
	// Must set blend weight to 1.0 for overrides to work
	CameraComponent->PostProcessBlendWeight = 1.0f;
	
	// Use Histogram for realistic adaptive exposure (works best with Lumen)
	CameraComponent->PostProcessSettings.bOverride_AutoExposureMethod = true;
	CameraComponent->PostProcessSettings.AutoExposureMethod = AEM_Histogram;
	
	// Critical: Exposure bias for space/night sky viewing
	CameraComponent->PostProcessSettings.bOverride_AutoExposureBias = true;
	CameraComponent->PostProcessSettings.AutoExposureBias = 2.0f; // +4 stops for dark space scenes
	
	// Exposure range for HDR stars and dark space
	CameraComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	CameraComponent->PostProcessSettings.AutoExposureMinBrightness = 0.01f; // Allow darker
	
	CameraComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	CameraComponent->PostProcessSettings.AutoExposureMaxBrightness = 7.0f; // Allow brighter stars
	
	// Exposure speed - slower for cinematic feel
	CameraComponent->PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
	CameraComponent->PostProcessSettings.AutoExposureSpeedUp = 2.0f; // Moderate adaptation
	
	CameraComponent->PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
	CameraComponent->PostProcessSettings.AutoExposureSpeedDown = 1.0f; // Slower dark adaptation
	
	// Lumen settings for optimal quality/performance
	CameraComponent->PostProcessSettings.bOverride_ReflectionMethod = true;
	CameraComponent->PostProcessSettings.ReflectionMethod = EReflectionMethod::Lumen;
	
	// Global Illumination
	CameraComponent->PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = true;
	CameraComponent->PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;

	// Create spline component for predefined camera paths
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);
	SplineComponent->SetDrawDebug(false);
}

void AMenuCameraActor::BeginPlay()
{
	Super::BeginPlay();

	// Initialize orbit angle based on current rotation
	OrbitAngle = GetActorRotation().Yaw;
	
	// DIAGNOSTIC: Log camera configuration
	UE_LOG(LogTemp, Warning, TEXT("🎥 MENU CAMERA DIAGNOSTICS (Lumen Optimized):"));
	UE_LOG(LogTemp, Warning, TEXT("  └─ Location: %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("  └─ Rotation: %s"), *GetActorRotation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("  └─ FOV: %.1f"), CameraComponent->FieldOfView);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Far Clip Plane: %.1f"), CameraComponent->OrthoFarClipPlane);
	UE_LOG(LogTemp, Warning, TEXT("  └─ PostProcess Blend Weight: %.2f"), CameraComponent->PostProcessBlendWeight);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Auto Exposure: %d (1=Histogram, 2=Basic)"), (int32)CameraComponent->PostProcessSettings.AutoExposureMethod);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Exposure Bias: %.2f stops"), CameraComponent->PostProcessSettings.AutoExposureBias);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Exposure Range: [%.3f - %.1f]"), 
		CameraComponent->PostProcessSettings.AutoExposureMinBrightness,
		CameraComponent->PostProcessSettings.AutoExposureMaxBrightness);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Lumen GI: %d, Lumen Reflections: %d"),
		(int32)CameraComponent->PostProcessSettings.DynamicGlobalIlluminationMethod,
		(int32)CameraComponent->PostProcessSettings.ReflectionMethod);
	UE_LOG(LogTemp, Warning, TEXT("  └─ Component Active: %d"), CameraComponent->IsActive());
}

void AMenuCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update transition if active
	if (bIsTransitioning)
	{
		UpdateTransition(DeltaTime);
	}
	// Update auto-orbit if enabled and not transitioning
	else if (bAutoOrbitEnabled)
	{
		UpdateAutoOrbit(DeltaTime);
	}
}

// ========================================
// Camera Transition API
// ========================================

void AMenuCameraActor::TransitionToLocation(FVector TargetLocation, FRotator TargetRotation, float Duration, EEasingFunc::Type EasingFunc)
{
	FMenuCameraTransition Transition;
	Transition.StartLocation = GetActorLocation();
	Transition.StartRotation = GetActorRotation();
	Transition.TargetLocation = TargetLocation;
	Transition.TargetRotation = TargetRotation;
	Transition.Duration = Duration;
	Transition.EasingFunc = EasingFunc;
	Transition.bTrackTarget = false;
	Transition.TransitionSpline = nullptr;

	ExecuteTransition(Transition);
}

void AMenuCameraActor::TransitionAlongSpline(USplineComponent* Spline, float Duration, EEasingFunc::Type EasingFunc)
{
	if (!Spline)
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuCameraActor::TransitionAlongSpline - Spline is null"));
		return;
	}

	FMenuCameraTransition Transition;
	Transition.StartLocation = GetActorLocation();
	Transition.StartRotation = GetActorRotation();
	Transition.TransitionSpline = Spline;
	Transition.Duration = Duration;
	Transition.EasingFunc = EasingFunc;
	Transition.bTrackTarget = false;

	// Target location/rotation will be evaluated from spline
	Transition.TargetLocation = Spline->GetLocationAtSplineInputKey(Spline->GetNumberOfSplinePoints() - 1.0f, ESplineCoordinateSpace::World);
	Transition.TargetRotation = Spline->GetRotationAtSplineInputKey(Spline->GetNumberOfSplinePoints() - 1.0f, ESplineCoordinateSpace::World);

	ExecuteTransition(Transition);
}

void AMenuCameraActor::TransitionTrackingTarget(AActor* TargetActor, FVector TrackingOffset, float Duration, EEasingFunc::Type EasingFunc)
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuCameraActor::TransitionTrackingTarget - TargetActor is null"));
		return;
	}

	FMenuCameraTransition Transition;
	Transition.StartLocation = GetActorLocation();
	Transition.StartRotation = GetActorRotation();
	Transition.TargetActor = TargetActor;
	Transition.TrackingOffset = TrackingOffset;
	Transition.Duration = Duration;
	Transition.EasingFunc = EasingFunc;
	Transition.bTrackTarget = true;
	Transition.TransitionSpline = nullptr;

	// Target location/rotation will be computed each frame
	Transition.TargetLocation = TargetActor->GetActorLocation() + TrackingOffset;
	const FVector LookDirection = (TargetActor->GetActorLocation() - Transition.TargetLocation).GetSafeNormal();
	Transition.TargetRotation = LookDirection.Rotation();

	ExecuteTransition(Transition);
}

void AMenuCameraActor::ExecuteTransition(const FMenuCameraTransition& Transition)
{
	CurrentTransition = Transition;
	TransitionTime = 0.0f;
	bIsTransitioning = true;

	// Disable auto-orbit during transition
	if (bAutoOrbitEnabled)
	{
		bAutoOrbitEnabled = false;
	}
}

void AMenuCameraActor::StopTransition()
{
	bIsTransitioning = false;
	TransitionTime = 0.0f;
}

// ========================================
// Manual Control
// ========================================

void AMenuCameraActor::SetManualControlEnabled(bool bEnabled)
{
	bManualControlEnabled = bEnabled;
}

void AMenuCameraActor::ApplyManualRotation(float DeltaYaw, float DeltaPitch)
{
	if (!bManualControlEnabled)
	{
		return;
	}

	FRotator CurrentRotation = GetActorRotation();
	
	// Apply yaw (no limits)
	CurrentRotation.Yaw += DeltaYaw * ManualRotationSensitivity;

	// Apply pitch (with limits)
	CurrentRotation.Pitch = FMath::Clamp(
		CurrentRotation.Pitch + DeltaPitch * ManualRotationSensitivity,
		PitchLimits.X,
		PitchLimits.Y
	);

	SetActorRotation(CurrentRotation);
}

// ========================================
// Auto-Orbit
// ========================================

void AMenuCameraActor::SetAutoOrbitEnabled(bool bEnabled, FVector NewOrbitCenter, float NewOrbitSpeed)
{
	bAutoOrbitEnabled = bEnabled;
	OrbitCenter = NewOrbitCenter;
	OrbitSpeed = NewOrbitSpeed;

	if (bEnabled)
	{
		// Calculate current orbit angle and radius
		const FVector CurrentLocation = GetActorLocation();
		const FVector DeltaToCenter = CurrentLocation - OrbitCenter;
		OrbitRadius = DeltaToCenter.Size2D();
		OrbitAngle = FMath::Atan2(DeltaToCenter.Y, DeltaToCenter.X) * 180.0f / PI;
	}
}

// ========================================
// Internal Methods
// ========================================

void AMenuCameraActor::UpdateTransition(float DeltaTime)
{
	TransitionTime += DeltaTime;
	const float Alpha = FMath::Clamp(TransitionTime / CurrentTransition.Duration, 0.0f, 1.0f);
	const float EasedAlpha = EvaluateEasing(Alpha, CurrentTransition.EasingFunc);

	FVector NewLocation;
	FRotator NewRotation;

	// Handle target tracking
	if (CurrentTransition.bTrackTarget && CurrentTransition.TargetActor)
	{
		// Update target location/rotation each frame to follow moving target
		const FVector TargetActorLocation = CurrentTransition.TargetActor->GetActorLocation();
		CurrentTransition.TargetLocation = TargetActorLocation + CurrentTransition.TrackingOffset;
		
		const FVector LookDirection = (TargetActorLocation - CurrentTransition.TargetLocation).GetSafeNormal();
		CurrentTransition.TargetRotation = LookDirection.Rotation();
	}

	// Handle spline-based transition
	if (CurrentTransition.TransitionSpline)
	{
		const float SplineLength = CurrentTransition.TransitionSpline->GetSplineLength();
		const float DistanceAlongSpline = EasedAlpha * SplineLength;
		
		NewLocation = CurrentTransition.TransitionSpline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
		NewRotation = CurrentTransition.TransitionSpline->GetRotationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
	}
	// Handle linear transition
	else
	{
		NewLocation = FMath::Lerp(CurrentTransition.StartLocation, CurrentTransition.TargetLocation, EasedAlpha);
		NewRotation = FMath::Lerp(CurrentTransition.StartRotation, CurrentTransition.TargetRotation, EasedAlpha);
	}

	// Apply new transform
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);

	// Check if transition is complete
	if (Alpha >= 1.0f)
	{
		bIsTransitioning = false;
		TransitionTime = 0.0f;
	}
}

void AMenuCameraActor::UpdateAutoOrbit(float DeltaTime)
{
	// Update orbit angle
	OrbitAngle += OrbitSpeed * DeltaTime;
	
	// Wrap angle to 0-360
	if (OrbitAngle >= 360.0f)
	{
		OrbitAngle -= 360.0f;
	}

	// Calculate new position on orbit
	const float AngleRad = OrbitAngle * PI / 180.0f;
	const FVector NewLocation = OrbitCenter + FVector(
		FMath::Cos(AngleRad) * OrbitRadius,
		FMath::Sin(AngleRad) * OrbitRadius,
		GetActorLocation().Z // Maintain current height
	);

	// Look at orbit center
	const FVector LookDirection = (OrbitCenter - NewLocation).GetSafeNormal();
	const FRotator NewRotation = LookDirection.Rotation();

	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
}

float AMenuCameraActor::EvaluateEasing(float Alpha, EEasingFunc::Type EasingFunc) const
{
	// Use UE5's built-in easing function
	float BlendExp = 2.0f; // Standard exponent for ease in/out
	return UKismetMathLibrary::Ease(0.0f, 1.0f, Alpha, EasingFunc, BlendExp);
}

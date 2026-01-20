// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MenuCameraActor.generated.h"

class UNiagaraComponent;

/**
 * Menu Camera Transition Data
 * 
 * Defines a camera movement from one position/rotation to another,
 * with optional spline-based path and target tracking.
 */
USTRUCT(BlueprintType)
struct FMenuCameraTransition
{
	GENERATED_BODY()

	/** Starting location for the transition (world space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	FVector StartLocation = FVector::ZeroVector;

	/** Starting rotation for the transition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	FRotator StartRotation = FRotator::ZeroRotator;

	/** Target location for the transition (world space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	FVector TargetLocation = FVector::ZeroVector;

	/** Target rotation for the transition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	FRotator TargetRotation = FRotator::ZeroRotator;

	/** Duration of the transition in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float Duration = 2.0f;

	/** Easing function for smooth transitions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	TEnumAsByte<EEasingFunc::Type> EasingFunc = EEasingFunc::EaseInOut;

	/** Optional spline to follow during transition (overrides linear interpolation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	TObjectPtr<USplineComponent> TransitionSpline = nullptr;

	/** Whether to track a moving target (like particles) during transition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	bool bTrackTarget = false;

	/** Actor to track during transition (typically a Niagara particle system) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** Offset from target actor when tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Transition")
	FVector TrackingOffset = FVector(-500.0f, 0.0f, 100.0f);
};

/**
 * AMenuCameraActor
 * 
 * Specialized camera actor for main menu with cinematic transitions.
 * Supports:
 * - Smooth transitions between menu states
 * - Spline-based camera paths
 * - Target tracking (following Niagara particles)
 * - Manual control (mouse drag to rotate view)
 * - Cinematic auto-orbit
 * 
 * Performance: All transitions use smooth interpolation on game thread.
 * No physics simulation. Suitable for 60+ FPS menu rendering.
 */
UCLASS(BlueprintType, Blueprintable)
class GAMELAUNCHER_API AMenuCameraActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMenuCameraActor();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// ========================================
	// Camera Transition API
	// ========================================

	/**
	 * Start a camera transition to a new location and rotation
	 * 
	 * @param TargetLocation - World space destination
	 * @param TargetRotation - Target camera rotation
	 * @param Duration - Time in seconds for transition
	 * @param EasingFunc - Interpolation curve
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void TransitionToLocation(FVector TargetLocation, FRotator TargetRotation, float Duration = 2.0f, 
		EEasingFunc::Type EasingFunc = EEasingFunc::EaseInOut);

	/**
	 * Start a camera transition following a spline path
	 * 
	 * @param Spline - Spline component to follow
	 * @param Duration - Time in seconds for transition
	 * @param EasingFunc - Interpolation curve
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void TransitionAlongSpline(USplineComponent* Spline, float Duration = 2.0f, 
		EEasingFunc::Type EasingFunc = EEasingFunc::EaseInOut);

	/**
	 * Start a camera transition that tracks a moving target (e.g., particles)
	 * 
	 * @param TargetActor - Actor to follow (typically Niagara system)
	 * @param TrackingOffset - Camera offset from target
	 * @param Duration - Time in seconds for transition
	 * @param EasingFunc - Interpolation curve
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void TransitionTrackingTarget(AActor* TargetActor, FVector TrackingOffset, float Duration = 2.0f, 
		EEasingFunc::Type EasingFunc = EEasingFunc::EaseInOut);

	/**
	 * Execute a pre-configured transition
	 * 
	 * @param Transition - Transition data structure
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void ExecuteTransition(const FMenuCameraTransition& Transition);

	/**
	 * Stop current transition and settle at current position
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void StopTransition();

	/**
	 * Check if camera is currently transitioning
	 */
	UFUNCTION(BlueprintPure, Category = "Menu Camera")
	bool IsTransitioning() const { return bIsTransitioning; }

	/**
	 * Get the camera component
	 */
	UFUNCTION(BlueprintPure, Category = "Menu Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	// ========================================
	// Manual Control
	// ========================================

	/**
	 * Enable/disable manual camera control (mouse drag to rotate)
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void SetManualControlEnabled(bool bEnabled);

	/**
	 * Apply manual rotation input (typically from mouse)
	 * 
	 * @param DeltaYaw - Horizontal rotation delta
	 * @param DeltaPitch - Vertical rotation delta
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void ApplyManualRotation(float DeltaYaw, float DeltaPitch);

	// ========================================
	// Auto-Orbit
	// ========================================

	/**
	 * Enable/disable automatic orbit around a point
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void SetAutoOrbitEnabled(bool bEnabled, FVector OrbitCenter = FVector::ZeroVector, float OrbitSpeed = 10.0f);

protected:
	// ========================================
	// Components
	// ========================================

	/** Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	/** Optional spline component for predefined camera paths */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USplineComponent> SplineComponent;

	// ========================================
	// Transition State
	// ========================================

	/** Whether a transition is currently in progress */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsTransitioning = false;

	/** Current transition data */
	UPROPERTY()
	FMenuCameraTransition CurrentTransition;

	/** Current time in transition (0 to Duration) */
	UPROPERTY()
	float TransitionTime = 0.0f;

	// ========================================
	// Manual Control State
	// ========================================

	/** Whether manual control is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Manual Control")
	bool bManualControlEnabled = false;

	/** Sensitivity for manual rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Manual Control", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ManualRotationSensitivity = 1.0f;

	/** Pitch limits for manual control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Manual Control")
	FVector2D PitchLimits = FVector2D(-80.0f, 80.0f);

	// ========================================
	// Auto-Orbit State
	// ========================================

	/** Whether auto-orbit is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Auto Orbit")
	bool bAutoOrbitEnabled = false;

	/** Center point for orbit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Auto Orbit")
	FVector OrbitCenter = FVector::ZeroVector;

	/** Orbit speed in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Auto Orbit", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float OrbitSpeed = 10.0f;

	/** Orbit radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Auto Orbit", meta = (ClampMin = "100.0"))
	float OrbitRadius = 2000.0f;

	/** Current orbit angle */
	float OrbitAngle = 0.0f;

	// ========================================
	// Internal Methods
	// ========================================

	/** Update transition state */
	void UpdateTransition(float DeltaTime);

	/** Update auto-orbit */
	void UpdateAutoOrbit(float DeltaTime);

	/** Evaluate easing function */
	float EvaluateEasing(float Alpha, EEasingFunc::Type EasingFunc) const;
};

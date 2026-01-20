// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpecTypes.h"
#include "PhysicsIntegrationSubsystem.generated.h"

class USurfaceQuerySubsystem;
class UEnvironmentSubsystem;
class UPrimitiveComponent;

/**
 * Delegate for when an impact exceeds damage threshold.
 * Consumers can bind to this for destruction/FX.
 * 
 * Parameters:
 * - Component: The component that experienced the impact
 * - Hit: The hit result containing collision information
 * - ImpactEnergy: Calculated impact energy in Joules
 * - DamageSpecId: The damage spec associated with this component
 * 
 * @note Bind to this for implementing destruction systems, FX spawning, or damage application
 * @see UDamageSpec for threshold configuration
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnImpactDamage,
	UPrimitiveComponent*, Component,
	const FHitResult&, Hit,
	float, ImpactEnergy,
	FDamageSpecId, DamageSpecId
);

/**
 * Delegate for when a body settles (goes to sleep).
 * Useful for delta persistence - allows saving final resting state.
 * 
 * Parameters:
 * - Component: The component that settled
 * - FinalTransform: The final resting transform
 * 
 * @note Bind to this for implementing world delta persistence
 * @note Only fired for registered physics bodies
 * @see FTransformDelta for persistence data structure
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnBodySettled,
	UPrimitiveComponent*, Component,
	FTransform, FinalTransform
);

/**
 * Per-body physics state cache.
 * Stored to avoid redundant queries each frame.
 */
USTRUCT()
struct FBodyPhysicsCache
{
	GENERATED_BODY()

	/** Cached environment context */
	FEnvironmentContext EnvironmentContext;

	/** Last query time */
	double LastQueryTime = 0.0;

	/** Accumulated drag force this frame */
	FVector AccumulatedDrag = FVector::ZeroVector;

	/** Is body currently asleep? */
	bool bIsSleeping = false;

	/** Frame counter for update throttling */
	int32 LastUpdateFrame = 0;
};

/**
 * World subsystem that integrates UETPFCore physics with Chaos.
 * 
 * Responsibilities:
 * - Query SurfaceState at contact points → modify friction/restitution
 * - Query EnvironmentContext for bodies → apply drag/buoyancy forces
 * - Monitor collision events → trigger damage when thresholds exceeded
 * - Track body sleep states → emit settle events for delta persistence
 * 
 * This is the "Systems consume context" step in the data flow pipeline.
 */
UCLASS()
class UETPFCORE_API UPhysicsIntegrationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPhysicsIntegrationSubsystem();

	//--- USubsystem Interface ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	//--- UTickableWorldSubsystem Interface ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	//==========================================================================
	// BODY REGISTRATION
	//==========================================================================

	/**
	 * Register a physics body for integration.
	 * Registered bodies receive drag/buoyancy forces and contact friction modification.
	 * 
	 * @param Component - The primitive component with physics
	 * @param DamageSpecId - Optional damage spec for impact handling
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Registration")
	void RegisterPhysicsBody(UPrimitiveComponent* Component, FDamageSpecId DamageSpecId = FDamageSpecId());

	/**
	 * Unregister a physics body.
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Registration")
	void UnregisterPhysicsBody(UPrimitiveComponent* Component);

	/**
	 * Check if a component is registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Registration")
	bool IsBodyRegistered(UPrimitiveComponent* Component) const;

	//==========================================================================
	// DRAG AND BUOYANCY
	//==========================================================================

	/**
	 * Calculate drag force for a body at its current location.
	 * Uses EnvironmentSubsystem for medium properties.
	 * 
	 * @param Component - The physics component
	 * @param Velocity - Current velocity (cm/s)
	 * @param DragArea - Effective drag area (cm²)
	 * @param DragCoefficient - Cd (typically 0.3-1.2)
	 * @return Drag force vector (opposing velocity)
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Forces")
	FVector CalculateBodyDrag(UPrimitiveComponent* Component, const FVector& Velocity, 
		float DragArea, float DragCoefficient = 0.5f) const;

	/**
	 * Calculate buoyancy force for a body at its current location.
	 * 
	 * @param Component - The physics component
	 * @param DisplacedVolume - Volume of medium displaced (cm³)
	 * @return Buoyancy force vector
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Forces")
	FVector CalculateBodyBuoyancy(UPrimitiveComponent* Component, float DisplacedVolume) const;

	/**
	 * Apply accumulated drag and buoyancy forces to a body.
	 * Called automatically during Tick for registered bodies.
	 */
	void ApplyEnvironmentalForces(UPrimitiveComponent* Component, float DeltaTime);

	//==========================================================================
	// CONTACT FRICTION
	//==========================================================================

	/**
	 * Get the surface state at a contact point.
	 * Combines SurfaceQuerySubsystem with environmental modifiers.
	 * 
	 * @param HitResult - The contact hit result
	 * @return FSurfaceState with effective friction values
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Contact")
	FSurfaceState GetContactSurfaceState(const FHitResult& HitResult) const;

	/**
	 * Modify physics material friction for a contact.
	 * Call this from collision callbacks to override default friction.
	 * 
	 * @param Component - Component involved in contact
	 * @param OtherComponent - Other component in contact
	 * @param HitResult - Contact hit result
	 * @param OutFriction - Modified friction to apply
	 * @param OutRestitution - Modified restitution to apply
	 * @return True if values were modified
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Contact")
	bool GetModifiedContactFriction(
		UPrimitiveComponent* Component,
		UPrimitiveComponent* OtherComponent,
		const FHitResult& HitResult,
		float& OutFriction,
		float& OutRestitution
	) const;

	//==========================================================================
	// DELEGATES
	//==========================================================================

	/**
	 * Fired when an impact exceeds the damage threshold defined in the body's DamageSpec.
	 * Bind to this to implement destruction, FX, or gameplay responses to damaging impacts.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Physics|Events")
	FOnImpactDamage OnImpactDamage;

	/**
	 * Fired when a registered physics body goes to sleep (settles after movement).
	 * Useful for persisting final object positions in world delta systems.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Physics|Events")
	FOnBodySettled OnBodySettled;

	//==========================================================================
	// IMPACT AND DAMAGE
	//==========================================================================

	/**
	 * Calculate impact energy from a collision.
	 * Uses relative velocity and effective mass.
	 * 
	 * Formula: E ≈ (Impulse²) / (2 * Mass)
	 * This is an approximation assuming elastic collision.
	 * 
	 * @param NormalImpulse - Impulse magnitude from collision (kg·cm/s in Unreal units)
	 * @param Mass - Mass of the impacting body (kg)
	 * @return Impact energy in Joules (approximate)
	 * 
	 * @note Use this to determine if DamageSpec thresholds are exceeded
	 * @see FOnImpactDamage delegate for handling damage events
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Impact")
	float CalculateImpactEnergy(float NormalImpulse, float Mass) const;

	/**
	 * Process a collision hit for potential damage.
	 * Compares impact energy against DamageSpec thresholds.
	 * Fires OnImpactDamage delegate if threshold exceeded.
	 * 
	 * @param Component - The component that was hit
	 * @param OtherComponent - The other component in collision
	 * @param NormalImpulse - Impact impulse vector (kg·cm/s)
	 * @param HitResult - Hit details including location, normal, etc.
	 * 
	 * @note Automatically called for registered bodies during collision processing
	 * @note Can be called manually if you have custom collision handling
	 */
	UFUNCTION(BlueprintCallable, Category = "Physics|Impact")
	void ProcessCollisionHit(
		UPrimitiveComponent* Component,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);

	//==========================================================================
	// CONFIGURATION
	//==========================================================================

	/** Minimum velocity (cm/s) to apply drag forces - prevents jitter at rest */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Config")
	float MinDragVelocity = 10.0f;

	/** 
	 * Update rate for environment queries (Hz) - controls how often bodies query EnvironmentSubsystem.
	 * Higher values = more accurate but more expensive. 10Hz is usually sufficient.
	 * Set to 0 for every-frame updates (not recommended for performance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Config")
	float EnvironmentQueryRate = 10.0f;

	/** Maximum bodies to update per frame (for throttling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Config")
	int32 MaxBodiesPerFrame = 50;

protected:
	/** Update environment cache for a body */
	void UpdateBodyEnvironmentCache(UPrimitiveComponent* Component, FBodyPhysicsCache& Cache);

	/** Check and emit settle events for sleeping bodies */
	void CheckBodySleepState(UPrimitiveComponent* Component, FBodyPhysicsCache& Cache);

	/** Get damage spec for a component */
	UDamageSpec* GetDamageSpecForComponent(UPrimitiveComponent* Component) const;

private:
	/** Reference to surface query subsystem */
	UPROPERTY()
	TObjectPtr<USurfaceQuerySubsystem> SurfaceQuerySubsystem;

	/** Reference to environment subsystem */
	UPROPERTY()
	TObjectPtr<UEnvironmentSubsystem> EnvironmentSubsystem;

	/** Registered physics bodies and their damage specs */
	UPROPERTY()
	TMap<UPrimitiveComponent*, FDamageSpecId> RegisteredBodies;

	/** Cached physics state per body */
	TMap<UPrimitiveComponent*, FBodyPhysicsCache> BodyCaches;

	/** Registered damage specs */
	UPROPERTY()
	TMap<FName, UDamageSpec*> DamageSpecMap;

	/** Current frame counter for update distribution */
	int32 CurrentFrame = 0;
};

// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/PhysicsIntegrationSubsystem.h"
#include "Subsystems/SurfaceQuerySubsystem.h"
#include "Subsystems/EnvironmentSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodyInstance.h"

DECLARE_CYCLE_STAT(TEXT("PhysicsIntegration Tick"), STAT_PhysicsIntegrationTick, STATGROUP_Game);

//=============================================================================
// UPhysicsIntegrationSubsystem
//=============================================================================

UPhysicsIntegrationSubsystem::UPhysicsIntegrationSubsystem()
{
}

void UPhysicsIntegrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Get references to other subsystems
	SurfaceQuerySubsystem = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
	EnvironmentSubsystem = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();

	UE_LOG(LogTemp, Log, TEXT("PhysicsIntegrationSubsystem initialized"));
}

void UPhysicsIntegrationSubsystem::Deinitialize()
{
	RegisteredBodies.Empty();
	BodyCaches.Empty();
	DamageSpecMap.Empty();

	Super::Deinitialize();
}

bool UPhysicsIntegrationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UPhysicsIntegrationSubsystem::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysicsIntegrationTick);

	CurrentFrame++;

	// Process registered bodies
	int32 BodiesProcessed = 0;
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	const double QueryInterval = 1.0 / FMath::Max(EnvironmentQueryRate, 1.0f);

	for (auto& Pair : RegisteredBodies)
	{
		UPrimitiveComponent* Component = Pair.Key;
		
		if (!IsValid(Component))
		{
			continue;
		}

		// Throttle updates
		if (BodiesProcessed >= MaxBodiesPerFrame)
		{
			break;
		}

		// Get or create cache
		FBodyPhysicsCache& Cache = BodyCaches.FindOrAdd(Component);

		// Check if we need to update environment cache
		if ((CurrentTime - Cache.LastQueryTime) >= QueryInterval)
		{
			UpdateBodyEnvironmentCache(Component, Cache);
		}

		// Apply environmental forces
		ApplyEnvironmentalForces(Component, DeltaTime);

		// Check sleep state for settle events
		CheckBodySleepState(Component, Cache);

		BodiesProcessed++;
	}
}

TStatId UPhysicsIntegrationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPhysicsIntegrationSubsystem, STATGROUP_Tickables);
}

//=============================================================================
// BODY REGISTRATION
//=============================================================================

void UPhysicsIntegrationSubsystem::RegisterPhysicsBody(UPrimitiveComponent* Component, FDamageSpecId DamageSpecId)
{
	if (!Component)
	{
		return;
	}

	if (!Component->IsSimulatingPhysics())
	{
		UE_LOG(LogTemp, Warning, TEXT("RegisterPhysicsBody: Component %s is not simulating physics"), 
			*Component->GetName());
	}

	RegisteredBodies.Add(Component, DamageSpecId);
	BodyCaches.Add(Component, FBodyPhysicsCache());

	UE_LOG(LogTemp, Verbose, TEXT("Registered physics body: %s"), *Component->GetName());
}

void UPhysicsIntegrationSubsystem::UnregisterPhysicsBody(UPrimitiveComponent* Component)
{
	if (Component)
	{
		RegisteredBodies.Remove(Component);
		BodyCaches.Remove(Component);
	}
}

bool UPhysicsIntegrationSubsystem::IsBodyRegistered(UPrimitiveComponent* Component) const
{
	return Component && RegisteredBodies.Contains(Component);
}

//=============================================================================
// DRAG AND BUOYANCY
//=============================================================================

FVector UPhysicsIntegrationSubsystem::CalculateBodyDrag(UPrimitiveComponent* Component, 
	const FVector& Velocity, float DragArea, float DragCoefficient) const
{
	if (!Component || !EnvironmentSubsystem)
	{
		return FVector::ZeroVector;
	}

	// Check minimum velocity
	const float Speed = Velocity.Size();
	if (Speed < MinDragVelocity)
	{
		return FVector::ZeroVector;
	}

	// Get location for environment query
	const FVector Location = Component->GetComponentLocation();

	// Delegate to EnvironmentSubsystem which has the full calculation
	return EnvironmentSubsystem->CalculateDragForce(Location, Velocity, DragArea, DragCoefficient);
}

FVector UPhysicsIntegrationSubsystem::CalculateBodyBuoyancy(UPrimitiveComponent* Component, 
	float DisplacedVolume) const
{
	if (!Component || !EnvironmentSubsystem)
	{
		return FVector::ZeroVector;
	}

	const FVector Location = Component->GetComponentLocation();
	return EnvironmentSubsystem->CalculateBuoyancyForce(Location, DisplacedVolume);
}

void UPhysicsIntegrationSubsystem::ApplyEnvironmentalForces(UPrimitiveComponent* Component, float DeltaTime)
{
	if (!Component || !Component->IsSimulatingPhysics())
	{
		return;
	}

	// Get body instance for physics properties
	FBodyInstance* BodyInstance = Component->GetBodyInstance();
	if (!BodyInstance)
	{
		return;
	}

	// Get current velocity
	const FVector Velocity = Component->GetPhysicsLinearVelocity();

	// Skip if nearly stationary
	if (Velocity.SizeSquared() < MinDragVelocity * MinDragVelocity)
	{
		return;
	}

	// Get cached environment context
	FBodyPhysicsCache* Cache = BodyCaches.Find(Component);
	if (!Cache || !Cache->EnvironmentContext.bIsValid)
	{
		return;
	}

	// Calculate drag using cached environment
	// Estimate drag area from bounds (simplified)
	FBoxSphereBounds Bounds = Component->Bounds;
	float DragArea = PI * Bounds.SphereRadius * Bounds.SphereRadius; // Cross-sectional area estimate

	// Get drag coefficient from component or use default
	float DragCoeff = 0.5f; // Default sphere Cd
	
	// Calculate drag force
	const FVector Location = Component->GetComponentLocation();
	FVector DragForce = EnvironmentSubsystem->CalculateDragForce(Location, Velocity, DragArea, DragCoeff);

	// Apply drag force
	if (!DragForce.IsNearlyZero())
	{
		Component->AddForce(DragForce, NAME_None, false);
	}

	// Calculate and apply buoyancy if in dense medium
	if (Cache->EnvironmentContext.Density > 10.0f) // Denser than thin atmosphere
	{
		// Estimate displaced volume from bounds
		float DisplacedVolume = (4.0f / 3.0f) * PI * FMath::Pow(Bounds.SphereRadius, 3.0f);
		
		FVector BuoyancyForce = EnvironmentSubsystem->CalculateBuoyancyForce(Location, DisplacedVolume);
		
		if (!BuoyancyForce.IsNearlyZero())
		{
			Component->AddForce(BuoyancyForce, NAME_None, false);
		}
	}
}

//=============================================================================
// CONTACT FRICTION
//=============================================================================

FSurfaceState UPhysicsIntegrationSubsystem::GetContactSurfaceState(const FHitResult& HitResult) const
{
	if (!SurfaceQuerySubsystem)
	{
		return FSurfaceState();
	}

	return SurfaceQuerySubsystem->GetSurfaceStateFromHit(HitResult);
}

bool UPhysicsIntegrationSubsystem::GetModifiedContactFriction(
	UPrimitiveComponent* Component,
	UPrimitiveComponent* OtherComponent,
	const FHitResult& HitResult,
	float& OutFriction,
	float& OutRestitution) const
{
	if (!SurfaceQuerySubsystem)
	{
		return false;
	}

	// Get surface state at contact point
	FSurfaceState SurfaceState = SurfaceQuerySubsystem->GetSurfaceStateFromHit(HitResult);

	if (!SurfaceState.bIsValid)
	{
		return false;
	}

	// Use dynamic friction for sliding contacts
	OutFriction = SurfaceState.FrictionDynamic;
	OutRestitution = SurfaceState.Restitution;

	return true;
}

//=============================================================================
// IMPACT AND DAMAGE
//=============================================================================

float UPhysicsIntegrationSubsystem::CalculateImpactEnergy(float NormalImpulse, float Mass) const
{
	// Impulse = m * Δv, so Δv = Impulse / m
	// Kinetic energy = 0.5 * m * v²
	// For collision: E ≈ Impulse² / (2 * m)
	
	if (Mass <= 0.0f)
	{
		return 0.0f;
	}

	// Convert impulse from UE units (kg*cm/s) to SI (kg*m/s)
	float ImpulseSI = NormalImpulse / 100.0f;
	
	// Energy in Joules
	float Energy = (ImpulseSI * ImpulseSI) / (2.0f * Mass);

	return Energy;
}

void UPhysicsIntegrationSubsystem::ProcessCollisionHit(
	UPrimitiveComponent* Component,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& HitResult)
{
	if (!Component)
	{
		return;
	}

	// Get damage spec for this component
	FDamageSpecId* FoundSpecId = RegisteredBodies.Find(Component);
	if (!FoundSpecId || !FoundSpecId->IsValid())
	{
		return;
	}

	// Get the damage spec
	UDamageSpec* DamageSpec = GetDamageSpecForComponent(Component);
	if (!DamageSpec)
	{
		return;
	}

	// Calculate impact energy
	FBodyInstance* BodyInstance = Component->GetBodyInstance();
	float Mass = BodyInstance ? BodyInstance->GetBodyMass() : 1.0f;
	float ImpactEnergy = CalculateImpactEnergy(NormalImpulse.Size(), Mass);

	// Check against threshold
	if (ImpactEnergy >= DamageSpec->ImpactThresholdMin)
	{
		// Broadcast damage event
		OnImpactDamage.Broadcast(Component, HitResult, ImpactEnergy, *FoundSpecId);

		UE_LOG(LogTemp, Verbose, TEXT("Impact damage: %s, Energy: %.2f J (threshold: %.2f J)"),
			*Component->GetName(), ImpactEnergy, DamageSpec->ImpactThresholdMin);
	}
}

//=============================================================================
// INTERNAL
//=============================================================================

void UPhysicsIntegrationSubsystem::UpdateBodyEnvironmentCache(UPrimitiveComponent* Component, 
	FBodyPhysicsCache& Cache)
{
	if (!Component || !EnvironmentSubsystem)
	{
		return;
	}

	const FVector Location = Component->GetComponentLocation();
	Cache.EnvironmentContext = EnvironmentSubsystem->GetEnvironmentAtLocation(Location);
	Cache.LastQueryTime = GetWorld()->GetTimeSeconds();
}

void UPhysicsIntegrationSubsystem::CheckBodySleepState(UPrimitiveComponent* Component, 
	FBodyPhysicsCache& Cache)
{
	if (!Component)
	{
		return;
	}

	FBodyInstance* BodyInstance = Component->GetBodyInstance();
	if (!BodyInstance)
	{
		return;
	}

	bool bCurrentlySleeping = BodyInstance->IsInstanceAwake() == false;

	// Detect transition to sleep
	if (bCurrentlySleeping && !Cache.bIsSleeping)
	{
		// Body just settled - broadcast event
		FTransform FinalTransform = Component->GetComponentTransform();
		OnBodySettled.Broadcast(Component, FinalTransform);

		UE_LOG(LogTemp, Verbose, TEXT("Body settled: %s at %s"),
			*Component->GetName(), *FinalTransform.GetLocation().ToString());
	}

	Cache.bIsSleeping = bCurrentlySleeping;
}

UDamageSpec* UPhysicsIntegrationSubsystem::GetDamageSpecForComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return nullptr;
	}

	FDamageSpecId const* FoundSpecId = RegisteredBodies.Find(Component);
	if (!FoundSpecId || !FoundSpecId->IsValid())
	{
		return nullptr;
	}

	UDamageSpec* const* FoundSpec = DamageSpecMap.Find(FoundSpecId->Id);
	if (FoundSpec)
	{
		return *FoundSpec;
	}

	return nullptr;
}

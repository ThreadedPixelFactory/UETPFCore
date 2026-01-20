// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "SpecTypes.h"

//=============================================================================
// USurfaceSpec
//=============================================================================

USurfaceSpec::USurfaceSpec()
{
	// Initialize default wetness response curve (1.0 when dry, 0.7 when fully wet)
	FRichCurve* WetnessCurve = WetnessResponseCurve.GetRichCurve();
	WetnessCurve->AddKey(0.0f, 1.0f);
	WetnessCurve->AddKey(1.0f, 0.7f);

	// Initialize default temperature response curve (normal at 288K, reduced at freezing)
	FRichCurve* TempCurve = TemperatureResponseCurve.GetRichCurve();
	TempCurve->AddKey(233.0f, 0.3f);  // -40°C - icy
	TempCurve->AddKey(273.0f, 0.5f);  // 0°C - near freezing
	TempCurve->AddKey(288.0f, 1.0f);  // 15°C - normal
	TempCurve->AddKey(373.0f, 1.0f);  // 100°C - hot but stable
}

FPrimaryAssetId USurfaceSpec::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("SurfaceSpec"), GetFName());
}

bool USurfaceSpec::ToStruct(FRuntimeSurfaceSpec& OutSpec) const
{
	// Placeholder scale factor - TODO: Add dedicated MaxDeformationDepth field to asset
	constexpr float kDeformationDepthScale = 10.0f;
	// Thresholds for derived flags (match vehicle/surface delta pipeline)
	constexpr float kSlipperyFrictionThreshold = 0.4f;
	constexpr float kDeformableThreshold = 0.01f;

	// Identity
	OutSpec.SpecId = SpecId.Id;
	OutSpec.Version = 1; // TODO: Add SpecVersion field to DataAsset for migration
	OutSpec.DisplayName = DisplayName.ToString();

	// Friction
	OutSpec.StaticFriction = FrictionStatic;
	OutSpec.DynamicFriction = FrictionDynamic;
	OutSpec.Restitution = Restitution;

	// Wetness response: sample curve at full wetness (1.0), default to 1.0 if no curve
	// 1.0 = no change, <1 = slipperier when wet, >1 = stickier (mud/adhesion)
	OutSpec.WetFrictionMultiplier = 1.0f; // Explicit default (no wetness effect)
	const FRichCurve* WetnessCurve = WetnessResponseCurve.GetRichCurveConst();
	const bool bHasWetnessCurve = WetnessCurve && WetnessCurve->GetNumKeys() > 0;
	if (bHasWetnessCurve)
	{
		// Clamp immediately to prevent bad curve values
		constexpr float kWetFrictionMin = 0.0f;
		constexpr float kWetFrictionMax = 2.0f;
		OutSpec.WetFrictionMultiplier = FMath::Clamp(WetnessCurve->Eval(1.0f), kWetFrictionMin, kWetFrictionMax);

	}

	// Deformation
	OutSpec.DeformationStrength = DeformationStrength;
	OutSpec.DeformationRatePerS = DeformationStrength; // Use strength as rate proxy until we add dedicated field
	OutSpec.MaxDeformationDepthCm = DeformationStrength * kDeformationDepthScale;
	OutSpec.RecoveryRatePerS = DeformationRecoveryRate;

	// Validate and clamp BEFORE deriving flags
	OutSpec.ValidateAndClamp();

	// Derive flags from final clamped values
	OutSpec.bIsDeformable = OutSpec.DeformationStrength > kDeformableThreshold;
	OutSpec.bIsSlippery = OutSpec.StaticFriction < kSlipperyFrictionThreshold;
	OutSpec.bAffectedByWetness = bHasWetnessCurve;

	// Temperature response: precompute LUT if curve exists
	if (TemperatureResponseCurve.GetRichCurveConst() 
		&& TemperatureResponseCurve.GetRichCurveConst()->GetNumKeys() > 0) {
		constexpr int32 NumSamples = 16;
		const float MinTemp = 200.f;
		const float MaxTemp = 350.f;

		OutSpec.bHasTemperatureResponse = true;
		OutSpec.TempFrictionLUT.MinTempK = MinTemp;
		OutSpec.TempFrictionLUT.MaxTempK = MaxTemp;
		OutSpec.TempFrictionLUT.Samples.SetNum(NumSamples);

		for (int32 i = 0; i < NumSamples; ++i) {
			const float Alpha = float(i) / float(NumSamples - 1);
			const float Temp = FMath::Lerp(MinTemp, MaxTemp, Alpha);
			const float Mult = TemperatureResponseCurve.GetRichCurveConst()->Eval(Temp);
			OutSpec.TempFrictionLUT.Samples[i] = FMath::Clamp(Mult, 0.0f, 2.0f);
		}
	} else {
		OutSpec.bHasTemperatureResponse = false;
	}

	return true;
}

//=============================================================================
// UMediumSpec
//=============================================================================

UMediumSpec::UMediumSpec()
{
}

FPrimaryAssetId UMediumSpec::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MediumSpec"), GetFName());
}

bool UMediumSpec::ToStruct(FRuntimeMediumSpec& OutSpec) const
{
	// Identity
	OutSpec.SpecId = SpecId.Id;
	OutSpec.Version = 1; // TODO: Add SpecVersion field to DataAsset for migration
	OutSpec.DisplayName = DisplayName.ToString();

	// Physical properties (keep separate drag coefficients)
	OutSpec.Density = Density;
	OutSpec.LinearDragCoeff = LinearDragCoeff;
	OutSpec.QuadraticDragCoeff = QuadraticDragCoeff;
	OutSpec.Viscosity = ViscosityProxy;
	OutSpec.PressurePa = Pressure * 1000.0f; // Convert kPa to Pa

	// Thermal (Temperature is already Kelvin in DataAsset by default)
	OutSpec.TemperatureK = TemperatureK;
	OutSpec.ThermalConductivityWmK = ThermalConductivityWmK > 0.0f ? ThermalConductivityWmK : 0.026f; // Default air value - TODO: Add field to DataAsset

	// Gravity
	OutSpec.Gravity = GravityVector;

	// Audio
	OutSpec.SpeedOfSound = SpeedOfSound;
	OutSpec.AbsorptionCoefficient = FMath::Clamp(1.0f - SoundAttenuationMultiplier, 0.0f, 1.0f);

	// Validate and clamp
	OutSpec.ValidateAndClamp();

	return true;
}

//=============================================================================
// UDamageSpec
//=============================================================================

UDamageSpec::UDamageSpec()
{
}

FPrimaryAssetId UDamageSpec::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DamageSpec"), GetFName());
}

//=============================================================================
// UFXProfile
//=============================================================================

UFXProfile::UFXProfile()
{
}

FPrimaryAssetId UFXProfile::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("FXProfile"), GetFName());
}

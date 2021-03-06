// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../AbilityFramework.h"
#include "GameplayTagContainer.h"
#include "../AFAbilityComponent.h"
#include "GAAttributesBase.h"
#include "../AFAbilityInterface.h"

#include "GAAttributeBase.h"
DEFINE_STAT(STAT_CalculateBonus);
DEFINE_STAT(STAT_CurrentBonusByTag);
DEFINE_STAT(STAT_FinalBonusByTag);
//UGAAttributeBase::UGAAttributeBase(const FObjectInitializer& ObjectInitializer)
//	: Super(ObjectInitializer)
//{
//
//}
FAFAttributeBase::FAFAttributeBase()
	: CurrentValue(0),
	BonusValue(0)
{
	Modifiers.AddDefaulted(7);
};
FAFAttributeBase::FAFAttributeBase(float BaseValueIn)
	: BaseValue(BaseValueIn),
	CurrentValue(BaseValue),
	BonusValue(0)
{
	Modifiers.AddDefaulted(7);
};


void FAFAttributeBase::InitializeAttribute()
{
	CurrentValue = BaseValue;
	CalculateBonus();
	CurrentValue = GetFinalValue();
	Modifiers.Empty();
	Modifiers.AddDefaulted(7);// static_cast<int32>(EGAAttributeMod::Invalid));
	//Modifiers.AddDefaulted(static_cast<int32>(EGAAttributeMod::Invalid));
	
}

void FAFAttributeBase::CalculateBonus()
{
	SCOPE_CYCLE_COUNTER(STAT_CalculateBonus);
	float AdditiveBonus = 0;
	float SubtractBonus = 0;
	float MultiplyBonus = 1;
	float DivideBonus = 1;
	//auto ModIt = Modifiers.CreateConstIterator();
	TMap<FGAEffectHandle, FGAEffectMod>& Additive = Modifiers[static_cast<int32>(EGAAttributeMod::Add)];
	TMap<FGAEffectHandle, FGAEffectMod>& Subtractive = Modifiers[static_cast<int32>(EGAAttributeMod::Subtract)];
	TMap<FGAEffectHandle, FGAEffectMod>& Multiplicative = Modifiers[static_cast<int32>(EGAAttributeMod::Multiply)];
	TMap<FGAEffectHandle, FGAEffectMod>& Divide = Modifiers[static_cast<int32>(EGAAttributeMod::Divide)];
	for (auto ModIt = Additive.CreateConstIterator(); ModIt; ++ModIt)
	{
		AdditiveBonus += ModIt->Value.Value;
	}
	for (auto ModIt = Subtractive.CreateConstIterator(); ModIt; ++ModIt)
	{
		SubtractBonus += ModIt->Value.Value;
	}
	for (auto ModIt = Multiplicative.CreateConstIterator(); ModIt; ++ModIt)
	{
		MultiplyBonus += ModIt->Value.Value;
	}
	for (auto ModIt = Divide.CreateConstIterator(); ModIt; ++ModIt)
	{
		DivideBonus += ModIt->Value.Value;
	}
	//for (ModIt; ModIt; ++ModIt)
	//{
	//	const FGAEffectMod& mod = ModIt->Value;
	//	switch (mod.AttributeMod)
	//	{
	//	case EGAAttributeMod::Add:
	//		AdditiveBonus += mod.Value;
	//		break;
	//	case EGAAttributeMod::Subtract:
	//		SubtractBonus += mod.Value;
	//		break;
	//	case EGAAttributeMod::Multiply:
	//		MultiplyBonus += mod.Value;
	//		break;
	//	case EGAAttributeMod::Divide:
	//		DivideBonus += mod.Value;
	//		break;
	//	default:
	//		break;
	//	}
	//}
	float OldBonus = BonusValue;
	//calculate final bonus from modifiers values.
	//we don't handle stacking here. It's checked and handled before effect is added.
	BonusValue = (AdditiveBonus - SubtractBonus);
	BonusValue = (BonusValue * MultiplyBonus);
	BonusValue = (BonusValue / DivideBonus);
	//this is absolute maximum (not clamped right now).
	float addValue = BonusValue - OldBonus;
	//reset to max = 200
	CurrentValue = CurrentValue + addValue;
	/*
	BaseValue = 200;
	CurrentValue = 200;
	BonusValue = 50;
	CurrentValue = 200 + 50;
	CurentValue == 250;

	Damage taken.
	CurrentValue = 250 - 25;
	CurrentValue == 225;
	Bonus Ended - THERE IS NO SUBTRACTION ONLY FULL STATCK RECALCULATION;
	Expected Result : 175; (225 - 50)
	OldBonusValue = 50;
	BonusValue = 0;
	CurrentValue == 225;
	BonusValue - OldBonusValue = -50;
	CurrentValue = CurrentValue + (-50); ??

	TwoBonuses 50 + 50;
	CurrentValue = 300 - 25;
	CurrentValue == 275;
	Bonus Ended - THERE IS NO SUBTRACTION ONLY FULL STATCK RECALCULATION;
	Expected Result : 225; (275 - 50)
	OldBonusValue = 100;
	BonusValue = 50;
	CurrentValue == 225;
	BonusValue - OldBonusValue = -50;
	CurrentValue = CurrentValue + (-50); ??

	Inverse Bonus is going to be Increased:
	TwoBonuses 50 + 50;
	CurrentValue = 300 - 25;
	CurrentValue == 275;
	Bonus Ended - THERE IS NO SUBTRACTION ONLY FULL STATCK RECALCULATION;
	Expected Result : 325; (275 + 50)
	OldBonusValue = 100;
	new BonusValue = 150; (new bonus gives +50)
	CurrentValue == 275;
	BonusValue - OldBonusValue = 50; (150 - 100) = 50
	CurrentValue = CurrentValue + (50); ??
	*/
}
//check for tags.
bool FAFAttributeBase::CheckIfModsMatch(const FGAEffectHandle& InHandle, const FGAEffectMod& InMod)
{
	TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(InMod.AttributeMod)];
	auto It = mods.CreateConstIterator();
	for (It; It; ++It)
	{
		if (It->Key.HasAllAttributeTags(InHandle)) //or maybe the other way around ?
		{
			return true;
		}
	}
	if (mods.Num() <= 0)
		return true;
	return false; 
}
bool FAFAttributeBase::CheckIfStronger(const FGAEffectMod& InMod)
{
	TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(InMod.AttributeMod)];
	auto It = mods.CreateConstIterator();
	for (It; It; ++It)
	{
		if (InMod > It->Value)
		{
			return true;
		}
	}
	if (mods.Num() <= 0)
	{
		return true;
	}
	return false;
}
float FAFAttributeBase::Modify(const FGAEffectMod& ModIn, const FGAEffectHandle& HandleIn,
	FGAEffectProperty& InProperty)
{
	float returnValue = -1;
	bool isPeriod = InProperty.Period > 0;
	bool IsDuration = InProperty.Duration > 0;
	if ( !isPeriod & IsDuration)
	{
		FGAModifier AttrMod(ModIn.AttributeMod, ModIn.Value, HandleIn);
		AttrMod.Tags.AppendTags(HandleIn.GetEffectSpec()->AttributeTags);
		AddBonus(ModIn, HandleIn);
		return ModIn.Value;
	}
	else
	{
		switch (ModIn.AttributeMod)
		{
		case EGAAttributeMod::Add:
		{
			float OldCurrentValue = CurrentValue;
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Add:: OldCurrentValue: %f"), OldCurrentValue);
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Add:: AddValue: %f"), ModIn.Value);
			float Val = CurrentValue - (OldCurrentValue + ModIn.Value);
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Add:: ActuallAddVal: %f"), Val);
			CurrentValue -= Val;
			CurrentValue = FMath::Clamp<float>(CurrentValue, 0, GetFinalValue());
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Add:: CurrentValue: %f"), CurrentValue);
			returnValue = CurrentValue;
			break;
		}
		case EGAAttributeMod::Subtract:
		{
			float OldCurrentValue = CurrentValue;
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Subtract:: OldCurrentValue: %f"), OldCurrentValue);
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Subtract:: SubtractValue: %f"), ModIn.Value);
			float Val = CurrentValue - (OldCurrentValue - ModIn.Value);
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Subtract:: ActuallSubtractVal: %f"), Val);
			CurrentValue -= Val;
			CurrentValue = FMath::Clamp<float>(CurrentValue, 0, GetFinalValue());
			UE_LOG(GameAttributes, Log, TEXT("FAFAttributeBase::Subtract:: CurrentValue: %f"), CurrentValue);

			returnValue = CurrentValue;
			break;
		}
		case EGAAttributeMod::Multiply:
		{
			returnValue = -1;
			break;
		}
		case EGAAttributeMod::Divide:
		{
			returnValue = -1;
			break;
		}
		}
	}
	return returnValue;
}

void FAFAttributeBase::AddBonus(const FGAEffectMod& ModIn, const FGAEffectHandle& Handle)
{
	TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(ModIn.AttributeMod)];
	FGAEffectMod& modsTemp = mods.FindOrAdd(Handle);
	modsTemp = ModIn;
	//switch (Stacking)
	//{
	//	case EAFAttributeStacking::Add:
	//	{
	//		TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(ModIn.AttributeMod)];
	//		FGAEffectMod& modsTemp = mods.FindOrAdd(Handle);
	//		modsTemp = ModIn;
	//		break;
	//	}
	//	case EAFAttributeStacking::Override:
	//	{
	//		if (CheckIfModsMatch(Handle, ModIn))
	//		{
	//			RemoveBonus(Handle, ModIn.AttributeMod);
	//			TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(ModIn.AttributeMod)];
	//			FGAEffectMod& modsTemp = mods.FindOrAdd(Handle);
	//			modsTemp = ModIn;
	//		}
	//		break;
	//	}
	//	case EAFAttributeStacking::StrongerOverride:
	//	{
	//		if (CheckIfStronger(ModIn))
	//		{
	//			RemoveBonus(Handle, ModIn.AttributeMod);
	//			TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(ModIn.AttributeMod)];
	//			FGAEffectMod& modsTemp = mods.FindOrAdd(Handle);
	//			modsTemp = ModIn;
	//		}
	//		break;
	//	}
	//}
	CalculateBonus();
}
void FAFAttributeBase::RemoveBonus(const FGAEffectHandle& Handle, EGAAttributeMod InMod)
{
	TMap<FGAEffectHandle, FGAEffectMod>& mods = Modifiers[static_cast<int32>(InMod)];
	mods.Remove(Handle);
	//Modifiers.Remove(Handle);
	CalculateBonus();
}
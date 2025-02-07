// Fill out your copyright notice in the Description page of Project Settings.


#include "WarroprTypes/WarriorStructTypes.h"

#include "AbilitySystem/Abilites/WarriorGameplayAbility.h"

bool FWarriorHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}

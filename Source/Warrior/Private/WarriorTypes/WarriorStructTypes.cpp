// Fill out your copyright notice in the Description page of Project Settings.


#include "..\..\Public\WarriorTypes/WarriorStructTypes.h"

#include "AbilitySystem/Abilites/WarriorHeroGameplayAbility.h"

bool FWarriorHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}

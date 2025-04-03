// Fill out your copyright notice in the Description page of Project Settings.
#include "WarriorGameInstance.h"

#include "MoviePlayer.h"

void UWarriorGameInstance::Init()
{
	Super::Init();

	// 监听地图预加载事件
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnDestinationWorldLoaded);
}

void UWarriorGameInstance::OnPreLoadMap(const FString& MapName)
{
	FLoadingScreenAttributes LoadingScreenAttribute;
	LoadingScreenAttribute.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreenAttribute.MinimumLoadingScreenDisplayTime = 2.f;
	LoadingScreenAttribute.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreenAttribute);
}

void UWarriorGameInstance::OnDestinationWorldLoaded(UWorld* LoadedWorld)
{
	GetMoviePlayer()->StopMovie();
}

TSoftObjectPtr<UWorld> UWarriorGameInstance::GetGameLevelByTag(FGameplayTag InTag) const
{
	for (const FWarriorGameLevelSet& GameLevelSet : GameLevelSets)
	{
		if (!GameLevelSet.IsValid()) continue;

		if (GameLevelSet.LevelTag == InTag)
		{
			return GameLevelSet.Level;
		}
	}

	return TSoftObjectPtr<UWorld>();
}

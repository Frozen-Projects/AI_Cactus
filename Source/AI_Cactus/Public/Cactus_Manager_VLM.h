// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Cactus_Includes.h"

#include "Cactus_Manager_VLM.generated.h"

using namespace cactus;

UCLASS()
class AI_CACTUS_API ACactus_Manager_VLM : public AActor
{
	GENERATED_BODY()
	
private:

	FString Model_Path;
	TSharedPtr<cactus_context, ESPMode::ThreadSafe> Cactus_Context;
	common_params Cactus_Params;

	FTimerHandle Handle_Counter;
	FTimerDelegate Delegate_Counter;

protected:

	// Called when the game starts or when spawned.
	virtual void BeginPlay() override;

	// Called when the game starts or when destroyed.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	

	// Sets default values for this actor's properties.
	ACactus_Manager_VLM();

	// Called every frame.
	virtual void Tick(float DeltaTime) override;

};

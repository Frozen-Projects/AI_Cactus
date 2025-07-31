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

	FString Path_Model;

	// Path to the MMProj file, if applicable.
	FString Path_MMProj;

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

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool Init_Cactus(FCactusModelParams_VLM VLM_Params);

	/*
	* @param In_Path_MMProj: Only required if using a model that requires a MMProj file. Otherwise, don't change this parameter or it will return false.
	*/
	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool SetModelPath(const FString& In_Path_Model, const FString& In_Path_MMProj = "MMPROJ_DISABLED");

	UFUNCTION(BlueprintPure, Category = "AI Cactus")
	virtual FString GetModelPath() const;

	UFUNCTION(BlueprintPure, Category = "AI Cactus")
	virtual FString GetMMProjPath() const;
};

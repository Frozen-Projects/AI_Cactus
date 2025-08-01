// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Cactus_Tools.h"

#include "Cactus_Manager_LLM.generated.h"

using namespace cactus;

UCLASS()
class AI_CACTUS_API ACactus_Manager_LLM : public AActor
{
	GENERATED_BODY()
	
private:

	FString Path_Model;
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
	ACactus_Manager_LLM();

	// Called every frame.
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool Init_Cactus(FCactusModelParams_LLM LLM_Params);
	
	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool SetModelPath(const FString& In_Path);

	UFUNCTION(BlueprintPure, Category = "AI Cactus")
	virtual FString GetModelPath() const;

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void GenerateText(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString Input, int32 MaxTokens = 100);

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void RunConversation(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString Input, int32 MaxTokens = 250, const FString& Assistant_Marker = "<|im_start|>assistant");

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool ClearConversation();

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void ExportConversation(FDelegateCactusSave DelegateSave, const FString& SavePath);

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void ImportConversation(FDelegateCactusSave DelegateLoad, const FString& FilePath, const FString& Assistant_Marker = "<|im_start|>assistant");
};
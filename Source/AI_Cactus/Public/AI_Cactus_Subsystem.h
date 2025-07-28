// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "AI_Cactus_Includes.h"

#include "AI_Cactus_Subsystem.generated.h"

using namespace cactus;

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_FiveParams(FDelegateCactus, bool, bIsSuccessfull, FString, Out_Result, double, Out_TT, double, Out_TTF, int32, Out_Tokens);

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_OneParam(FDelegateCounter, int32, Out_Counter);

UCLASS()
class AI_CACTUS_API UCactusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:

	FString Model_Path;
	TSharedPtr<cactus_context, ESPMode::ThreadSafe> Cactus_Context;
	common_params Cactus_Params;

	FTimerHandle Handle_Counter;
	FTimerDelegate Delegate_Counter;

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool Init_Cactus(int32 NumberThreads = 4, const FString& AntiPrompt = "<|im_end|>");
	
	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual bool SetModelPath(const FString& Path);

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual FString GetModelPath() const;

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void GenerateText(FDelegateCactus DelegateCactus, FDelegateCounter DelegateCounter, FString Input, int32 MaxTokens = 100);

	UFUNCTION(BlueprintCallable, Category = "AI Cactus")
	virtual void RunConversation(FDelegateCactus DelegateCactus, FDelegateCounter DelegateCounter, FString Input, int32 MaxTokens = 250);

};

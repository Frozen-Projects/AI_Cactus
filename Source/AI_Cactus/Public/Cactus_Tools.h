#pragma once

#include "Cactus_Tools.generated.h"

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_FiveParams(FDelegateCactus, bool, bIsSuccessfull, FString, Out_Result, double, Out_TT, double, Out_TTF, int32, Out_Tokens);

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_OneParam(FDelegateCactusCounter, int32, Out_Counter);

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_OneParam(FDelegateCactusSave, bool, bIsSuccessfull);

UCLASS()
class AI_CACTUS_API UCactusConversationSave : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> EmbdTokens;

};
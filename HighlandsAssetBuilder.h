#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HighlandsAssetBuilder.generated.h"

/** Build native static mesh assets from Blender's exact centimetre buffers. */
UCLASS()
class PUTTPOP_API UHighlandsAssetBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Putt Pop|Editor")
    static bool BuildAssets(const FString& SourceDirectory);
};

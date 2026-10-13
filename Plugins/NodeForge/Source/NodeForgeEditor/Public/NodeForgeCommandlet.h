#pragma once
#include "Commandlets/Commandlet.h"
#include "NodeForgeCommandlet.generated.h"

UCLASS()
class NODEFORGEEDITOR_API UNodeForgeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UNodeForgeCommandlet();
    virtual int32 Main(const FString& Params) override;
};


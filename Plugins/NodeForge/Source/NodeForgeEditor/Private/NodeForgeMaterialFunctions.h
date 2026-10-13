#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"
class UMaterialExpressionMaterialFunctionCall;
namespace NodeForgeMaterialFunctions
{
using FObject=TSharedPtr<FJsonObject>;
bool Bind(UMaterialExpressionMaterialFunctionCall* Call,const NodeForgeGraphDocument::FNodeBinding& Binding,FString& Error,int32 ReadRevision = 1);
NodeForgeGraphDocument::FNodeBinding Binding(UMaterialExpressionMaterialFunctionCall* Call);
bool Inspect(const FObject& Config,FObject& Evidence,FString& Error);
FObject Properties(UMaterialExpressionMaterialFunctionCall* Call);
FObject Signature(UMaterialExpressionMaterialFunctionCall* Call);
bool Matches(UMaterialExpressionMaterialFunctionCall* Actual,UMaterialExpressionMaterialFunctionCall* Expected);
}

#include "NodeForgeGraphDocument.h"

namespace NodeForgeGraphDocument
{
const FNodeDescription* FGraphDefinition::FindNode(const FString& SearchId) const
{
    return Nodes.FindByPredicate([&](const FNodeDescription& Node)
        { return Node.Id.Equals(SearchId, ESearchCase::CaseSensitive); });
}
FString FGraphDocument::AssetType() const
{
    if (Asset.IsType<FMaterialDefinition>()) return TEXT("Material");
    if (Asset.IsType<FBlueprintDefinition>()) return TEXT("Blueprint");
    return TEXT("MaterialInstance");
}
}

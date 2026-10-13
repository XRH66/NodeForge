#pragma once
#include "NodeForgeGraphDocument.h"
#include "NodeForgeNodeDefinitions.h"
#include "Dom/JsonObject.h"

namespace NodeForgeGraphCodec
{
using namespace NodeForgeGraphDocument;
using FObject = TSharedPtr<FJsonObject>;
// Decoding validates the closed wire vocabulary; native adapters validate actual
// reflected properties, pins, factories and saved references against this data.
bool Decode(const FObject& Input, FGraphDocument& Output, FString& Error,
    NodeForgeNodeDefinitions::EContext Context = NodeForgeNodeDefinitions::EContext::External);
bool DecodeText(const FString& Text, FGraphDocument& Output, FString& Error,
    NodeForgeNodeDefinitions::EContext Context = NodeForgeNodeDefinitions::EContext::External);
FObject Encode(const FGraphDocument& Document, int32 FormatVersion);
bool EncodeText(const FGraphDocument& Document, int32 FormatVersion, FString& Text, FString& Error);
// Source original mode preserves array order and field presence across versions.
bool EqualOriginal(const FGraphDocument& A, const FGraphDocument& B);
}

#pragma once
#include "CoreMinimal.h"
class SWidget;
namespace NodeForgeInstallationPanel
{
TSharedRef<SWidget> Create(TFunction<TSharedRef<SWidget>()> ProductPanel);
}

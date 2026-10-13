using UnrealBuildTool;
public class NodeForgeEditor : ModuleRules
{
    public NodeForgeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd", "BlueprintGraph", "KismetCompiler", "Kismet", "MaterialEditor", "GraphEditor",
            "AssetRegistry", "AssetTools", "Json", "JsonUtilities", "Slate", "SlateCore",
            "ToolMenus", "DesktopPlatform", "Projects", "RenderCore", "RHI", "ContentBrowser", "ApplicationCore"
        });
        PublicSystemLibraries.Add("bcrypt.lib");
        PublicSystemLibraries.Add("advapi32.lib");
        if (Target.bBuildDeveloperTools)
            PrivateDependencyModuleNames.AddRange(new string[] { "AutomationDriver", "InputCore" });
    }
}

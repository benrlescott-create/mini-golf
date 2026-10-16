using UnrealBuildTool;
public class PuttPop : ModuleRules
{
    public PuttPop(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities"
        });
        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd", "AssetRegistry", "MeshDescription", "StaticMeshDescription"
            });
    }
}

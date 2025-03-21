
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class AssetsTool : ModuleRules
    {
        public AssetsTool(ReadOnlyTargetRules Target) : base(Target)
        {
	        PublicDependencyModuleNames.AddRange(new string[] { "Engine" });
	        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "InputCore",
                    "CoreUObject",
                    "Engine",
                    "UnrealEd",
                    "UMG",                   
                    "Slate",
                    "EditorStyle",
                    "SlateCore",
                    "Paper2D",
                    "SourceControlWindows",
                    "AssetRegistry",
                    "UMGEditor",
                    "AssetManagerEditor",
                    "RenderCore", 
                    "ApexDestruction",
                    "MediaAssets",
                    "SourceControl",
                    "AssetRegistry",
                    "AkAudio",
                    "Paper2D",
                    "EditorWidgets",
                    "Blutility",
                    "UnrealEd",
                    "DeveloperSettings",
                    "AbleCore",
                    "Persona",
                    "MovieScene",
                    "MovieSceneTracks",
                    "LevelSequence",
                    "Landscape",
                    "Utility",
                    "AssetTools",
                    "EditorFramework",
                    "Projects",
                    "ApplicationCore", 
                    "Perforce", 
                    "StandaloneRenderer", 
                    "AbleEditor"
                }
            );
        }
    }
}
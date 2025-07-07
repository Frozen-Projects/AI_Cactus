// Some copyright should be here...

using System;
using System.IO;
using UnrealBuildTool;

public class AI_Cactus : ModuleRules
{
	public AI_Cactus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
        bEnableExceptions = true;

        if (UnrealTargetPlatform.Win64 == Target.Platform)
        {
            bUseRTTI = true;

            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "cactus", "Win64", "include"));

            string Path_Libs = Path.Combine(ModuleDirectory, "ThirdParty", "cactus", "Win64", "lib");
            string[] List_Libs = Directory.GetFiles(Path_Libs, "*.lib", SearchOption.TopDirectoryOnly);

            foreach (string File in List_Libs)
            {
                if (File.EndsWith(".lib"))
                {
                    PublicAdditionalLibraries.Add(File);
                }
            }
        }

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "OpenSSL",
                "libcurl"
				// ... add other public dependencies that you statically link with here ...
			}
			);
				
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
			
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}

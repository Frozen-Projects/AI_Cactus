namespace UnrealBuildTool.Rules
{
    using System.IO;

    public class nlohmann_json : ModuleRules
    {
        public nlohmann_json(ReadOnlyTargetRules Target) : base(Target)
        {
    		Type = ModuleType.External;
            UndefinedIdentifierWarningLevel = WarningLevel.Off;
            bEnableExceptions = true;

            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                bUseRTTI = true;

                PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Win64"));
            }
        }
    }
}

using HarmonyLib;
using System.Reflection;
using System.Runtime.InteropServices;

namespace RuntimePatcher
{
    public class Main
    {
        private static Harmony? harmony;

        [UnmanagedCallersOnly]
        public static void InitializePatches()
        {
            Console.WriteLine("Injected!");

            harmony = new Harmony("com.example.patch");
            harmony.PatchAll(typeof(Main).Assembly);
        }
    }

    [HarmonyPatch]
    public class ProgramPatches
    {
        static MethodBase TargetMethod()
        {
            return AccessTools.Method(AccessTools.TypeByName("DemoApplication.Program"), "F");
        }

        [HarmonyPrefix]
        static void F(ref int i)
        {
            i = 1337;
        }
    }
}

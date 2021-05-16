using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RendererCookAndBuildTool
{
    static class PathInfo
    {
        public static readonly string SolutionDirectory = "..";
        public static readonly string VulkanRendererProjectPath = SolutionDirectory + "\\Vulkan-Renderer";
        public static readonly string ResourceFolder = VulkanRendererProjectPath + "\\Res";
        public static readonly string CompiledShadersFolder = ResourceFolder + "\\CompiledShaders";
        public static readonly string ShadersFolder = ResourceFolder + "\\Shaders";
        public static readonly string TexturesFolder = ResourceFolder + "\\Textures";
        public static readonly string ModelsFolder = ResourceFolder + "\\Models";

        public static readonly string CompileShadersToSprirVGlslc = ResourceFolder + "\\glslc.exe";

        public static readonly string BinOutputPathDebug = SolutionDirectory + "\\x64\\Debug\\";
        public static readonly string BinOutputPathRelease = SolutionDirectory + "\\x64\\Release\\";
        public static readonly string ResFolderOutputPathDebug = BinOutputPathDebug + "\\Res";
        public static readonly string ResFolderOutputPathRelease = BinOutputPathRelease + "\\Res";

        public static readonly string DllGlfwSrcPath = SolutionDirectory + "\\Libs\\glfw3.dll";
        public static readonly string DllAssimpSrcPath = SolutionDirectory + "\\Libs\\assimp-vc142-mt.dll";

        public static readonly string DllIGlfwDstPathDebug = BinOutputPathDebug + "\\glfw3.dll";
        public static readonly string DllIGlfwDstPathRelease = BinOutputPathRelease + "\\glfw3.dll";
        public static readonly string DllAssimpDstPathDebug = BinOutputPathDebug + "\\assimp-vc142-mt.dll";
        public static readonly string DllAssimpDstPathRelease = BinOutputPathRelease + "\\assimp-vc142-mt.dll";
    }
}

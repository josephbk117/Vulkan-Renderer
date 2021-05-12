using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace RendererCookAndBuildTool
{
    class Program
    {
        static int Main(string[] args)
        {
            string[] shaderFiles = Directory.GetFiles(Path.GetFullPath(PathInfo.ShadersFolder));

            bool atleastOneShaderFailed = false;
            Console.WriteLine("GLSLC SpirV compiler process started");

            Directory.CreateDirectory(Path.GetFullPath(PathInfo.CompiledShadersFolder));

            foreach (string shaderFile in shaderFiles)
            {
                string inShaderFile = "\"" + shaderFile + "\"";
                string outShaderFile = "\"" + Path.GetFullPath(PathInfo.CompiledShadersFolder) + "\\" + Path.GetFileName(shaderFile) + ".spv\"";
                if (!ExecuteCommand(Path.GetFullPath(PathInfo.CompileShadersToSprirVGlslc), inShaderFile + " -o " + outShaderFile))
                {
                    atleastOneShaderFailed = true;
                }
            }

            // Skip copying to build folder
            if (atleastOneShaderFailed)
            {
                Console.ReadLine();
                return -1;
            }

            Directory.CreateDirectory(Path.GetFullPath(PathInfo.BinOutputPathDebug));
            Directory.CreateDirectory(Path.GetFullPath(PathInfo.BinOutputPathRelease));
            File.Copy(Path.GetFullPath(PathInfo.DllIGlfwSrcPath), Path.GetFullPath(PathInfo.DllIGlfwDstPathDebug), true);
            File.Copy(Path.GetFullPath(PathInfo.DllIGlfwSrcPath), Path.GetFullPath(PathInfo.DllIGlfwDstPathRelease), true);

            string resFolderOutputPathDebug = Path.GetFullPath(PathInfo.ResFolderOutputPathDebug);
            string resFolderOutputPathRelease = Path.GetFullPath(PathInfo.ResFolderOutputPathRelease);

            List<string> foldersToAddToOutputRes = new List<string>();
            foldersToAddToOutputRes.Add(PathInfo.CompiledShadersFolder);
            foldersToAddToOutputRes.Add(PathInfo.TexturesFolder);

            foreach (string inputFolder in foldersToAddToOutputRes)
            {
                if (Directory.Exists(inputFolder))
                {
                    string[] files = Directory.GetFiles(Path.GetFullPath(inputFolder));

                    foreach (string file in files)
                    {
                        string fileName = Path.GetFileName(file);
                        string outputFolderName = Path.GetFileName(Path.GetDirectoryName(file));
                        string outputFolderPathDebug = resFolderOutputPathDebug + "\\" + outputFolderName;
                        string outputFolderPathRelease = resFolderOutputPathRelease + "\\" + outputFolderName;
                        string destFileDebug = Path.Combine(outputFolderPathDebug, fileName);
                        string destFileRelease = Path.Combine(outputFolderPathRelease, fileName);
                        Directory.CreateDirectory(outputFolderPathDebug);
                        Directory.CreateDirectory(outputFolderPathRelease);
                        File.Copy(file, destFileDebug, true);
                        File.Copy(file, destFileRelease, true);
                    }
                }
            }

            return 0;
        }

        static public bool ExecuteCommand(string exeDir, string args)
        {
            try
            {
                ProcessStartInfo procStartInfo = new ProcessStartInfo
                {
                    FileName = exeDir,
                    Arguments = args,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (Process process = new Process())
                {
                    process.StartInfo = procStartInfo;
                    process.Start();
                    process.WaitForExit();

                    string errorStr = process.StandardError.ReadToEnd();
                    Console.WriteLine("Output Details : " + errorStr);

                    if (errorStr.Length > 1)
                    {
                        return false;
                    }
                }
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occurred executing the following commands");
                Console.WriteLine(exeDir);
                Console.WriteLine(args);
                Console.WriteLine(ex.Message);
                return false;
            }
        }
    }
}

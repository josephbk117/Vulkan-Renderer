using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace RendererCookAndBuildTool
{
    class Program
    {
        static void Main(string[] args)
        {
            string[] shaderFiles = Directory.GetFiles(Path.GetFullPath(PathInfo.ShadersFolder));

            bool atleastOneShaderFailed = false;
            Console.WriteLine("GLSLC SpirV compiler process started");

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
            if(atleastOneShaderFailed)
            {
                Console.ReadLine();
                return;
            }

            Directory.CreateDirectory(Path.GetFullPath(PathInfo.BinOutputPathDebug));
            File.Copy(Path.GetFullPath(PathInfo.DllIGlfwSrcPath), Path.GetFullPath(PathInfo.DllIGlfwDstPathDebug), true);

            string resFolderOutputPath = Path.GetFullPath(PathInfo.ResFolderOutputPathDebug);

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
                        string outputFolderPath = resFolderOutputPath + "\\" + outputFolderName;
                        string destFile = Path.Combine(outputFolderPath, fileName);
                        Directory.CreateDirectory(outputFolderPath);
                        File.Copy(file, destFile, true);
                    }
                }
            }
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

                    Console.WriteLine("Info Output : " + process.StandardOutput.ReadToEnd());

                    string errorStr = process.StandardError.ReadToEnd();
                    Console.WriteLine("Error Output : " + errorStr);

                    if (errorStr.Length > 0)
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

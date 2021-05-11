using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RendererCookAndBuildTool
{
    class Program
    {
        static void Main(string[] args)
        {
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
    }
}

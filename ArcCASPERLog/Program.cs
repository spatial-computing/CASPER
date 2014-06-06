using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace ArcCASPERLog
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: ArcCASPERLog logfilename.log [outputsheet.csv]");
                return 1;
            }

            string outFile, inpFile = args[0];            
            if (args.Length > 1) outFile = args[1];
            else outFile = inpFile.Replace(".log", ".csv");

            try
            {
                int j = 0;
                var logFile = File.ReadAllLines(inpFile);
                int count = 1 + logFile.Count(l => l.StartsWith("Executing ("));
                var logSplit = new string[count];
                for (int i = 0; i < count; i++) logSplit[i] = string.Empty;

                foreach (string l in logFile)
                {
                    if (l.StartsWith("Executing (")) j++;
                    logSplit[j] += l + Environment.NewLine;
                }

                var logs = logSplit.Where(l => (!string.IsNullOrEmpty(l)) && l.StartsWith("Executing ("));
                string Calc = string.Empty, Name = string.Empty, Carma = string.Empty, EvcTime = string.Empty, Mem = string.Empty, carmaTime = string.Empty;
                var csvStrings = new List<string>(logs.Count());

                var NameRex = new Regex("\\\\([A-Za-z0-9 \\-]+)\" SKIP TERMINATE", RegexOptions.None);
                var CalcRex = new Regex(@"Calculation = (\d+\.\d+) \(kernel\), (\d+\.\d+) \(user\)", RegexOptions.None);
                var CarmaRex = new Regex(@"algorithm performed (\d+) CARMA", RegexOptions.None);
                var CarmaRex2 = new Regex(@"algorithm performed (\d+) CARMA loop\(s\) in (\d+\.\d+) seconds.", RegexOptions.None);
                var EvcTimeRex = new Regex(@"Global evacuation cost is (\d+\.\d+)", RegexOptions.None);
                var MemRex = new Regex(@"Peak memory usage is (\d+) MB", RegexOptions.None);
                Match m = null;

                csvStrings.Add("Name,CalcTime,CarmaLoops,CarmaTime,EvcTime,MemUsage");

                foreach (string log in logs)
                {
                    m = NameRex.Match(log);
                    if (m.Success) Name = m.Groups[1].Captures[0].ToString();
                    m = CalcRex.Match(log);
                    if (m.Success) Calc = "=" + m.Groups[1].Captures[0].ToString() + "+" + m.Groups[2].Captures[0].ToString();
                    m = CarmaRex2.Match(log);
                    if (m.Success)
                    {
                        Carma = m.Groups[1].Captures[0].ToString();
                        carmaTime = m.Groups[2].Captures[0].ToString();
                    }
                    else
                    {
                        m = CarmaRex.Match(log);
                        if (m.Success) Carma = m.Groups[1].Captures[0].ToString();
                        carmaTime = string.Empty;
                    }
                    m = EvcTimeRex.Match(log);
                    if (m.Success) EvcTime = m.Groups[1].Captures[0].ToString();
                    m = MemRex.Match(log);
                    if (m.Success) Mem = m.Groups[1].Captures[0].ToString();

                    csvStrings.Add(Name + "," + Calc + "," + Carma + "," + carmaTime + "," + EvcTime + "," + Mem);
                }

                File.WriteAllLines(outFile, csvStrings);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
            
            return 0;
        }
    }
}

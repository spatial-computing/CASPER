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
            if (outFile == inpFile) throw new ArgumentException("input and output file cannot be the same.");

            try
            {
                int j = 0;
                var logFile = File.ReadAllLines(inpFile);
                int count = 1 + logFile.Count(l => l.StartsWith("CASPER for ArcGIS(x86"));
                var logSplit = new string[count];
                for (int i = 0; i < count; i++) logSplit[i] = string.Empty;

                foreach (string l in logFile)
                {
                    if (l.StartsWith("CASPER for ArcGIS(x86")) j++;
                    logSplit[j] += l + Environment.NewLine;
                }

                var logs = logSplit.Where(l => (!string.IsNullOrEmpty(l)) && l.StartsWith("CASPER for ArcGIS(x86"));
                string Itr = string.Empty, Calc = string.Empty, ScenarioName = string.Empty, SetupName = string.Empty, Carma = string.Empty, EvcTime = string.Empty, Mem = string.Empty, carmaTime = string.Empty;
                var csvStrings = new List<string>(logs.Count());

                var NameRex = new Regex("\\\\([A-Za-z0-9 \\-]+)\" SKIP TERMINATE", RegexOptions.None);
                var SetupNameRex = new Regex(@"Solved ([^ ]+) with scenario ([^ \n\r]+)" + Environment.NewLine, RegexOptions.None);                
                var CalcRex = new Regex(@"Calculation = (\d+\.\d+) \(kernel\), (\d+\.\d+) \(user\)", RegexOptions.None);
                var CarmaRex = new Regex(@"algorithm performed (\d+) CARMA", RegexOptions.None);
                var CarmaRex2 = new Regex(@"algorithm performed (\d+) CARMA loop\(s\) in (\d+\.\d+) seconds.", RegexOptions.None);
                var EvcTimeRex1 = new Regex(@". Evacuation cost at the end is: (\d+\.\d+)" + Environment.NewLine, RegexOptions.None);
                var EvcTimeRex2 = new Regex(@". Evacuation costs at each iteration are: [\, \.\d]+ (\d+\.\d+)" + Environment.NewLine, RegexOptions.None);
                var MemRex = new Regex(@". Peak memory usage \(exclude flocking\) was (\d+) MB.", RegexOptions.None);
                var ItrRex = new Regex(@"The program ran for (\d+) iteration", RegexOptions.None);
                Match m = null;
                int Setup = 0;

                csvStrings.Add("SetupName,Scenario,CalcTime (min),CarmaLoops,Iteration,CarmaTime (sec),EvcTime,MemUsage (MB)");

                foreach (string log in logs)
                {
                    m = SetupNameRex.Match(log);
                    if (m.Success)
                    {
                        SetupName = m.Groups[1].Captures[0].ToString();
                        ScenarioName = m.Groups[2].Captures[0].ToString();
                    }
                    else
                    {
                        m = NameRex.Match(log);
                        if (m.Success) SetupName = m.Groups[1].Captures[0].ToString();
                        else SetupName = (++Setup).ToString();
                        ScenarioName = string.Empty;
                    }
                    m = CalcRex.Match(log);
                    if (m.Success) Calc = "=(" + m.Groups[1].Captures[0].ToString() + "+" + m.Groups[2].Captures[0].ToString() + ")/60";
                    else Calc = "-9999";
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
                        else Carma = "-9999";
                        carmaTime = string.Empty;
                    }
                    m = EvcTimeRex1.Match(log);
                    if (m.Success) EvcTime = m.Groups[1].Captures[0].ToString();
                    else
                    {
                        m = EvcTimeRex2.Match(log);
                        if (m.Success) EvcTime = m.Groups[1].Captures[0].ToString();
                        else EvcTime = "-9999";
                    }
                    m = MemRex.Match(log);
                    if (m.Success) Mem = m.Groups[1].Captures[0].ToString();
                    else Mem = "-9999";
                    m = ItrRex.Match(log);
                    if (m.Success) Itr = m.Groups[1].Captures[0].ToString();
                    else Itr = "-9999";

                    csvStrings.Add(SetupName + "," + ScenarioName + "," + Calc + "," + Carma + "," + Itr + "," + carmaTime + "," + EvcTime + "," + Mem);
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

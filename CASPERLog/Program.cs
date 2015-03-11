using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace CASPERLog
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: CASPERLog logfilename.log [outputsheet.csv]");
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
                string Itr1 = string.Empty, Itr2 = string.Empty, Calc = string.Empty, ScenarioName = string.Empty, SetupName = string.Empty,
                    Carma = string.Empty, EvcTime = string.Empty, Mem = string.Empty, carmaTime = string.Empty, stuck = string.Empty;
                var csvStrings = new List<string>(logs.Count());

                var StuckRex = new Regex(@"routes are generated from the evacuee points. (\d+) evacuee(s) were unreachable.", RegexOptions.None);
                var NameRex = new Regex("\\\\([A-Za-z0-9 \\-]+)\" SKIP TERMINATE", RegexOptions.None);
                var SetupNameRex = new Regex(@"Solved ([^ ]+) with scenario ([^ \n\r]+)" + Environment.NewLine, RegexOptions.None);                
                var CalcRex = new Regex(@"Calculation = (\d+\.\d+) \(kernel\), (\d+\.\d+) \(user\)", RegexOptions.None);
                var CarmaRex = new Regex(@"algorithm performed (\d+) CARMA", RegexOptions.None);
                var CarmaRex2 = new Regex(@"algorithm performed (\d+) CARMA loop\(s\) in (\d+\.\d+) seconds.", RegexOptions.None);
                var EvcTimeRex1 = new Regex(@". Evacuation cost at the end is: (\d+\.\d+)" + Environment.NewLine, RegexOptions.None);
                var EvcTimeRex2 = new Regex(@". Evacuation costs at each iteration are: [\, \.\d]+ (\d+\.\d+)" + Environment.NewLine, RegexOptions.None);
                var MemRex = new Regex(@". Peak memory usage \(exclude flocking\) was (\d+) MB.", RegexOptions.None);
                var ItrRex1 = new Regex(@"The program ran for (\d+) iteration", RegexOptions.None);
                var ItrRex2 = new Regex(@"The effective iteration ratio at each pass: ([\, \.\d]+)" + Environment.NewLine, RegexOptions.None);
                Match m = null;
                int Setup = 0;

                csvStrings.Add("SetupName,Scenario,CalcTime (min),CarmaLoops,IterationCount,EffectiveIterationRatios,StuckEvacee,CarmaTime (sec),EvcTime,MemUsage (MB)");

                foreach (string log in logs)
                {
                    m = SetupNameRex.Match(log);
                    if (m.Success)
                    {
                        SetupName = m.Groups[1].Captures[0].ToString();
                        ScenarioName = m.Groups[2].Captures[0].ToString();
                        int DIndex = SetupName.LastIndexOf('D'), init = 0;
                        if (DIndex > 0 && int.TryParse(SetupName.Substring(DIndex + 1), out init))
                        {
                            ScenarioName += "_" + SetupName.Substring(DIndex);
                            SetupName = SetupName.Substring(0, DIndex);
                        }
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
                    m = ItrRex1.Match(log);
                    if (m.Success) Itr1 = m.Groups[1].Captures[0].ToString();
                    else Itr1 = "-9999";
                    m = ItrRex2.Match(log);
                    if (m.Success) Itr2 = m.Groups[1].Captures[0].ToString().Replace(", ","|");
                    else Itr2 = "";
                    m = StuckRex.Match(log);
                    if (m.Success) stuck = m.Groups[1].Captures[0].ToString();
                    else stuck = "-9999";

                    csvStrings.Add(SetupName + "," + ScenarioName + "," + Calc + "," + Carma + "," + Itr1 + "," + Itr2 + "," + stuck + "," + carmaTime + "," + EvcTime + "," + Mem);
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

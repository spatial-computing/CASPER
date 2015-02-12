using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CountQuery
{
    class LogRecord
    {
        public double Improvment;
        public string SetupName;
        public string Scenario;

        public LogRecord(string setupname, string scenario, string improvment)
        {
            if (improvment.EndsWith("%")) improvment = improvment.Substring(0, improvment.Length - 1);
            if (!double.TryParse(improvment, out Improvment)) throw new Exception("bad double value for Improvment");
            SetupName = setupname;
            Scenario = scenario;
        }
    }

    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: CASPERCountQuery logfilename.csv MODE");
                return 1;
            }

            string inpFile = args[0];
            int mode = int.Parse(args[1]);

            try
            {
                var logFile = File.ReadAllLines(inpFile);

                // check header of csv
                var header = logFile[0].Split(',');
                bool check1 = header[0] == "Setup Name";
                bool check2 = header[1] == "Scenario";
                bool check3 = header[13] == "Improve";

                if (!check1 || !check2 || !check3) throw new Exception("header of CSV does not pass the check.");

                List<LogRecord> records = new List<LogRecord>(logFile.Count() - 1);

                for (int i = 1; i < logFile.Count(); i++)
                {
                    var record = logFile[i].Split(',');
                    if (record.Count() != header.Count()) throw new Exception("Bad split by comma in record " + i);
                    records.Add(new LogRecord(record[0], record[1], record[13]));
                }

                var allSetupNames = records.Select(r => r.SetupName);
                var DistSetupNames = allSetupNames.Distinct().OrderBy(s => s).ToArray();
                double[,] output = new double[DistSetupNames.Count(), DistSetupNames.Count()];
                for (int i = 0; i < DistSetupNames.Count(); ++i) for (int j = 0; j < DistSetupNames.Count(); ++j) output[i, j] = 0.0;

                foreach (var record in records.GroupBy(r => r.Scenario))
                {
                    for (int i = 0; i < DistSetupNames.Count(); ++i)
                        for (int j = 0; j < DistSetupNames.Count(); ++j)
                            switch (mode)
                            {
                                case 1:
                                    if (record.First(r => r.SetupName == DistSetupNames[i]).Improvment >= record.First(r => r.SetupName == DistSetupNames[j]).Improvment) output[i, j]++;
                                    break;
                                case 2:
                                    output[i, j] += record.First(r => r.SetupName == DistSetupNames[i]).Improvment - record.First(r => r.SetupName == DistSetupNames[j]).Improvment;
                                    break;
                                case 3:
                                    if (record.First(r => r.SetupName == DistSetupNames[i]).Improvment > record.First(r => r.SetupName == DistSetupNames[j]).Improvment) output[i, j]++;
                                    break;
                                default:
                                    throw new Exception("Bad MODE");
                            }                        
                }

                for (int i = -1; i < DistSetupNames.Count(); ++i)
                {
                    for (int j = -1; j < DistSetupNames.Count(); ++j)
                    {
                        if (i == -1 && j == -1) System.Console.Write("\t");
                        else if (i == -1 && j != -1) System.Console.Write(DistSetupNames[j] + "\t");
                        else if (i != -1 && j == -1) System.Console.Write(Environment.NewLine + DistSetupNames[i] + "\t");
                        else System.Console.Write(output[i, j].ToString("F3") + "\t");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }

            return 0;
        }
    }
}

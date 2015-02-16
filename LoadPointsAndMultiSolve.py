##################
# ---------------------------------------------------------------------------
# LoadPointsAndMultiSolve.py
# Author:       Kaveh Shahabi
# Date:         Feb 16, 2015
# Usage:        LoadPointsAndMultiSolve <scenarios> <Input_Dataset> <Evacuation_Prefix> <Safe_Zone_Prefix> <PubCASPER> <NumOfThreads>
# Description:  Iterates over rows of a table (e.g. scenario names), loads the right points from a dataset into the NA layer, then solves all layers with multiple threads.
#               At the end exports the NA sub layers back to the same dataset.
# ---------------------------------------------------------------------------

# Import arcpy module
import arcpy
import multiprocessing as mp
import numpy

# The function that solves the NALayer
def Solve(Input_Layer_Location, ScenarioNames, Input_Dataset, Evacuation_Prefix, Safe_Zone_Prefix, ThreadCount, ThreadNum):
    # load layer file
    SolveCount = 0
    lyrFile = arcpy.mapping.Layer(Input_Layer_Location)
    # totalSolves = ScenarioNames.shape[0] * len(arcpy.mapping.ListLayers(lyrFile)) / 8
    # arcpy.SetProgressor("step", "Solving {0} experiments(s)".format(ScenarioNames.shape[0]), 0, totalSolves, 1)

    # loop over all scenarios, import them into each NA layer
    for ExpName in ScenarioNames:
        # arcpy.AddMessage("Importing scenario: " + ExpName[0])
        EVC = Input_Dataset + '\\' + Evacuation_Prefix + ExpName[0]
        SAFE = Input_Dataset + '\\' + Safe_Zone_Prefix + ExpName[0]

        # now loop over all NA layers and solve them one by one
        for lyr in arcpy.mapping.ListLayers(lyrFile):
            desc = arcpy.Describe(Input_Layer_Location + "\\" + lyr.longName)
            # only solve if the layer is a network analysis layer
            if desc.dataType == "NALayer":

                # We check if this setup and scenario is intended for this particular sub-process
                SolveCount += 1
                if SolveCount % ThreadCount != ThreadNum:
                    continue

                # arcpy.SetProgressorLabel("Solving " + lyr.name + " with scenario " + ExpName[0] + "...")
                # load input locations
##                arcpy.AddMessage("loading input points to " + lyr.name + " from scenario " + ExpName[0])
##                arcpy.AddLocations_na(lyr, "Evacuees", EVC, "VehicleCount POPULATION #;Name UID #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
##                for msg in range(0, arcpy.GetMessageCount()):
##                    arcpy.AddReturnMessage(msg)
##                arcpy.AddLocations_na(lyr, "Zones", SAFE, "Name OBJECTID #;Capacity Capacity #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
##                for msg in range(0, arcpy.GetMessageCount()):
##                    arcpy.AddReturnMessage(msg)
##
##                # solve the layer
##                arcpy.AddMessage("Solving NALayer " + lyr.name + " with scenario " + ExpName[0])
##                arcpy.Solve_na(lyr, "SKIP", "TERMINATE")
##                for msg in range(0, arcpy.GetMessageCount()):
##                    arcpy.AddReturnMessage(msg)
##
##                # going to export route and edge sub_layers
##                solved_layers = arcpy.mapping.ListLayers(lyr)
##                arcpy.CopyFeatures_management(solved_layers[4], Input_Dataset + "\\Routes_" + lyr.name + "_" + ExpName[0]) #Routes
##                for msg in range(0, arcpy.GetMessageCount()):
##                    arcpy.AddReturnMessage(msg)
##                arcpy.CopyFeatures_management(solved_layers[5], Input_Dataset + "\\EdgeStat_"+ lyr.name + "_" + ExpName[0]) #EdgeStat
##                for msg in range(0, arcpy.GetMessageCount()):
##                    arcpy.AddReturnMessage(msg)
##                del solved_layers
                arcpy.AddMessage(SolveCount + ": Solved " + lyr.name + " with scenario " + ExpName[0])
                print(SolveCount + ": Solved " + lyr.name + " with scenario " + ExpName[0])
                # arcpy.SetProgressorPosition()
            del desc
    del lyrFile

def main():        
    # Check out any necessary licenses
    if arcpy.CheckExtension("Network") == "Available":
        arcpy.CheckOutExtension("Network")
    else:
        arcpy.AddMessage("Network Analyst Extension Is Not Available")
        print "Network Analyst Is Not Available"
        sys.exit(0)

    # Load required toolboxes
    arcpy.env.overwriteOutput = True

    # Script arguments
    scenarios = arcpy.GetParameterAsText(0)
    if scenarios == '#' or not scenarios:
        raise ValueError("scenarios table is missing")

    Input_Dataset = arcpy.GetParameterAsText(1)
    if Input_Dataset == '#' or not Input_Dataset:
        raise ValueError("Input dataset is missing")

    Evacuation_Prefix = arcpy.GetParameterAsText(2)
    if Evacuation_Prefix == '#' or not Evacuation_Prefix:
        Evacuation_Prefix = "Evc_" # provide a default value if unspecified

    Safe_Zone_Prefix = arcpy.GetParameterAsText(3)
    if Safe_Zone_Prefix == '#' or not Safe_Zone_Prefix:
        Safe_Zone_Prefix = "Safe_" # provide a default value if unspecified

    Input_Layer_Location = arcpy.GetParameterAsText(4)
    if Input_Layer_Location == '#' or not Input_Layer_Location:
        raise ValueError("Evacuation routing layer file is missing")

    ThreadCount = int(arcpy.GetParameterAsText(5))
    if ThreadCount == '#' or not ThreadCount:
        ThreadCount = 4 # provide a default value if unspecified

    # Set current workspace
    arcpy.env.workspace = Input_Dataset

    # laod the scenarios table
    ScenarioNames = arcpy.da.TableToNumPyArray(scenarios, 'ShortName')

    # queue the processes and pass the thread nums
    pool = mp.Pool(processes=ThreadCount)
    for ThreadNum in range(0, ThreadCount - 1):
        pool.apply_async(Solve, args=(Input_Layer_Location, ScenarioNames, Input_Dataset, Evacuation_Prefix, Safe_Zone_Prefix, ThreadCount, ThreadNum))
    pool.close()
    pool.join()

    # Update the progressor position
    # arcpy.SetProgressorPosition()
    arcpy.CheckInExtension("Network")
    
if __name__ == "__main__":
    main()

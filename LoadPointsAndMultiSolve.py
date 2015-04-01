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
import os
import arcpy
import multiprocessing as mp
import numpy

# The function that solves the NALayer
def Solve(Input_Layer_Location, ScenarioNames, Input_Dataset, Evacuation_Prefix, Safe_Zone_Prefix, Dynamics_Prefix, ThreadCount, msgLock, dbLock, ThreadNum):
    # Check out any necessary licenses
    if arcpy.CheckExtension("Network") == "Available":
        arcpy.CheckOutExtension("Network")
    else:
        arcpy.AddMessage("Network Analyst Extension Is Not Available")
        print "Network Analyst Is Not Available"
        sys.exit(0)

    # Load required toolboxes
    arcpy.env.overwriteOutput = True    
    SolveCount = 0

    try:
        # load layer file
        lyrFile = arcpy.mapping.Layer(Input_Layer_Location)
        messages = []
        
        # loop over all scenarios, import them into each NA layer
        for ExpName in ScenarioNames:
            # arcpy.AddMessage("Importing scenario: " + ExpName[0])
            EVC = Input_Dataset + '\\' + Evacuation_Prefix + ExpName[0]
            SAFE = Input_Dataset + '\\' + Safe_Zone_Prefix + ExpName[0]
            DYN = Input_Dataset + '\\' + Dynamics_Prefix + ExpName[0]

            # now loop over all NA layers and solve them one by one
            for lyr in arcpy.mapping.ListLayers(lyrFile):
                try:
                    desc = arcpy.Describe(Input_Layer_Location + "\\" + lyr.longName)
                    # only solve if the layer is a network analysis layer
                    if desc.dataType == "NALayer":

                        # We check if this setup and scenario is intended for this particular sub-process
                        SolveCount += 1
                        if SolveCount % ThreadCount != ThreadNum:
                            continue

                        # load input locations
                        del messages[:]
                        try:
                            messages.append("Thread {}: loading input points to {} from scenario {}".format(ThreadNum, lyr.name, ExpName[0]))
                                                
                            arcpy.AddLocations_na(lyr, "Evacuees", EVC, "VehicleCount POPULATION #;Name UID #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
                            for msg in range(0, arcpy.GetMessageCount()):
                                messages.append(arcpy.GetMessage(msg))
                            arcpy.AddLocations_na(lyr, "Zones", SAFE, "Name OBJECTID #;Capacity Capacity #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
                            for msg in range(0, arcpy.GetMessageCount()):
                                messages.append(arcpy.GetMessage(msg))
                            arcpy.AddLocations_na(lyr, "DynamicChanges", DYN, "EdgeDirection Zones_EdgeDirection #;StartingCost Zones_StartingCost #;EndingCost Zones_EndingCost #;CostChangeRatio Zones_CostChangeRatio #;CapacityChangeRatio Zones_CapacityChangeRatio #", "5000 Meters", "", "Streets SHAPE;SoCal_ND_Junctions NONE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "INCLUDE", "Streets #;SoCal_ND_Junctions #")
                            for msg in range(0, arcpy.GetMessageCount()):
                                messages.append(arcpy.GetMessage(msg))
                            
                            # solve the layer
                            messages.append("Solving NALayer " + lyr.name + " with scenario " + ExpName[0])
                            arcpy.Solve_na(lyr, "SKIP", "TERMINATE")
                            for msg in range(0, arcpy.GetMessageCount()):
                                messages.append(arcpy.GetMessage(msg))

                            # going to export route and edge sub_layers
                            solved_layers = arcpy.mapping.ListLayers(lyr)
                            
                            # lock and then write outputs to gdb
                            try:
                                dbLock.acquire()
                                arcpy.CopyFeatures_management(solved_layers[4], Input_Dataset + "\\Routes_" + lyr.name + "_" + ExpName[0]) #Routes
                                for msg in range(0, arcpy.GetMessageCount()):
                                    messages.append(arcpy.GetMessage(msg))
                                arcpy.CopyFeatures_management(solved_layers[5], Input_Dataset + "\\EdgeStat_"+ lyr.name + "_" + ExpName[0]) #EdgeStat
                                for msg in range(0, arcpy.GetMessageCount()):
                                    messages.append(arcpy.GetMessage(msg))
                            finally:
                                dbLock.release()
                                del solved_layers

                            messages.append("Combination {}: Solved {} with scenario {}{}".format(SolveCount, lyr.name, ExpName[0], os.linesep))

                        except BaseException as e:
                            messages.append("Error: {}".format(e))
                            messages.append("Combination {}: Errored {} with scenario {}{}".format(SolveCount, lyr.name, ExpName[0], os.linesep))
                        
                        # lock and then print messages
                        try:
                            msgLock.acquire()
                            for msg in messages:
                                arcpy.AddMessage(msg)
                        finally:
                            msgLock.release()

                except BaseException as e:
                    arcpy.AddError(e)
                finally:
                    del desc
                
    except BaseException as e:
        arcpy.AddError(e)
    finally:
        del lyrFile
        del messages
        arcpy.CheckInExtension("Network")

def main():
    # Script arguments
    scenarios = arcpy.GetParameterAsText(0)
    if scenarios == '#' or not scenarios:
        raise ValueError("1st argument: scenarios table is missing")

    Input_Dataset = arcpy.GetParameterAsText(1)
    if Input_Dataset == '#' or not Input_Dataset:
        raise ValueError("2nd argument: Input dataset is missing")

    Input_Layer_Location = arcpy.GetParameterAsText(2)
    if Input_Layer_Location == '#' or not Input_Layer_Location:
        raise ValueError("3rd argument: Evacuation routing layer file is missing")

    ThreadCountStr = arcpy.GetParameterAsText(3)
    if ThreadCountStr == '#' or not ThreadCountStr:
        ThreadCount = 4 # provide a default value if unspecified
    else:
        ThreadCount = int(ThreadCountStr)

    Evacuation_Prefix = arcpy.GetParameterAsText(4)
    if Evacuation_Prefix == '#' or not Evacuation_Prefix:
        Evacuation_Prefix = "Evc_" # provide a default value if unspecified

    Safe_Zone_Prefix = arcpy.GetParameterAsText(5)
    if Safe_Zone_Prefix == '#' or not Safe_Zone_Prefix:
        Safe_Zone_Prefix = "Safe_" # provide a default value if unspecified

    Dynamics_Prefix = arcpy.GetParameterAsText(6)
    if Dynamics_Prefix == '#' or not Dynamics_Prefix:
        Dynamics_Prefix = "Dynamics_" # provide a default value if unspecified

    # Set current workspace
    arcpy.env.workspace = Input_Dataset

    # laod the scenarios table
    ScenarioNames = arcpy.da.TableToNumPyArray(scenarios, 'ShortName')

    # queue the processes and pass the thread nums
    dbLock = mp.Manager().Lock()
    msgLock = mp.Manager().Lock()
    pool = mp.Pool(processes=ThreadCount)
    for ThreadNum in range(ThreadCount):
        pool.apply_async(Solve, args=(Input_Layer_Location, ScenarioNames, Input_Dataset, Evacuation_Prefix, Safe_Zone_Prefix, Dynamics_Prefix, ThreadCount, msgLock, dbLock, ThreadNum))
    pool.close()
    pool.join()
    
if __name__ == "__main__":
    main()

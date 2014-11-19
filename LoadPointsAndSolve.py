##################
# ---------------------------------------------------------------------------
# LoadPointsAndSolve.py
# Author:       Kaveh Shahabi
# Date:         Nov 14, 2014
# Usage:        LoadPointsAndSolve <Experiments> <Input_Dataset> <Evacuation_Feature_Class_Prefix> <Safe_Zone_Feature_Class_Prefix> <PubCASPER> 
# Description:  Iterates over rows of a table (e.g. experiment names), loads the right points from a dataset into the NA layer, then solves all layers.
#               At the end exports the NA sub layers back to the same dataset.
# ---------------------------------------------------------------------------

# Import arcpy module
import arcpy
import numpy

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
Experiments = arcpy.GetParameterAsText(0)
if Experiments == '#' or not Experiments:
    raise ValueError("Experiments table is missing")

Input_Dataset = arcpy.GetParameterAsText(1)
if Input_Dataset == '#' or not Input_Dataset:
    raise ValueError("Input dataset is missing")

Evacuation_Feature_Class_Prefix = arcpy.GetParameterAsText(2)
if Evacuation_Feature_Class_Prefix == '#' or not Evacuation_Feature_Class_Prefix:
    Evacuation_Feature_Class_Prefix = "Evc_" # provide a default value if unspecified

Safe_Zone_Feature_Class_Prefix = arcpy.GetParameterAsText(3)
if Safe_Zone_Feature_Class_Prefix == '#' or not Safe_Zone_Feature_Class_Prefix:
    Safe_Zone_Feature_Class_Prefix = "Safe_" # provide a default value if unspecified

Input_Layer_Location = arcpy.GetParameterAsText(4)
if Input_Layer_Location == '#' or not Input_Layer_Location:
    raise ValueError("Evacuation routing layer file is missing")

# laod the experiments table
ExpNames = arcpy.da.TableToNumPyArray(Experiments, 'ShortName')

# load layer file
lyrFile = arcpy.mapping.Layer(Input_Layer_Location)

# loop over all experiments, import them into each NA layer
for ExpName in ExpNames:
    arcpy.AddMessage("Importing experiment: " + ExpName[0])
    EVC = Input_Dataset+'\\'+Evacuation_Feature_Class_Prefix+ExpName[0]
    SAFE = Input_Dataset+'\\'+Safe_Zone_Feature_Class_Prefix+ExpName[0]

    # now loop over all NA layers and solve them one by one
    for lyr in arcpy.mapping.ListLayers(lyrFile):
        desc = arcpy.Describe(Input_Layer_Location + "\\" + lyr.longName)
        try:
            # only solve if the layer is associated with the evacuation solver
            if desc.dataType == "NALayer":
                # load input locations
                arcpy.AddLocations_na(lyr, "Evacuees", EVC, "VehicleCount OrgPop #;Name BLKGRPCE #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
                for msg in range(0, arcpy.GetMessageCount()):
                    arcpy.AddReturnMessage(msg)
                arcpy.AddLocations_na(lyr, "Zones", SAFE, "Name OBJECTID #;Capacity Capacity #", "5000 Meters", "", "Streets NONE;SoCal_ND_Junctions SHAPE", "MATCH_TO_CLOSEST", "CLEAR", "NO_SNAP", "5 Meters", "EXCLUDE", "Streets #;SoCal_ND_Junctions #")
                for msg in range(0, arcpy.GetMessageCount()):
                    arcpy.AddReturnMessage(msg)

                # solve the layer
                # arcpy.SetProgressor("step", "Solving " + lyr.nameString + " with experiment " + ExpName[0] + "...", 0, 100, 1)
                arcpy.AddMessage("Solving " + lyr.nameString + " with experiment " + ExpName[0])
                arcpy.Solve_na(lyr, "SKIP", "TERMINATE")
                for msg in range(0, arcpy.GetMessageCount()):
                    arcpy.AddReturnMessage(msg)

                # going to export route and edge sub_layers
                solved_layers = arcpy.mapping.ListLayers(lyr)
                arcpy.CopyFeatures_management(solved_layers[4], Input_Dataset + "\\" + lyr.nameString + "_Routes_" + ExpName[0]) #Routes
                arcpy.CopyFeatures_management(solved_layers[5], Input_Dataset + "\\" + lyr.nameString + "_EdgeStat_" + ExpName[0]) #EdgeStat
                arcpy.AddMessage("Solved " + lyr.nameString + " with experiment " + ExpName[0])
        except AttributeError:
            pass
        del desc
    
del lyrFile

arcpy.CheckInExtension("Network")

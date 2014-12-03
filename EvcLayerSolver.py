##################
# ---------------------------------------------------------------------------
# EvcLayerSolver.py
# Author:       Kaveh Shahabi
# Date:         Nov 13, 2014
# Usage:        EvcLayerSolver.py <Workspace> <Layer_File_Name> 
# Description:  Solves all networtk layers within a layer file and saves the result
#               The workspace is where the network dataset exist.
# ---------------------------------------------------------------------------

# Import arcpy module
import arcpy
import sys
arcpy.env.workspace = arcpy.GetParameterAsText(0)
arcpy.SetParameter(2, False)

# Check out any necessary licenses
if arcpy.CheckExtension("Network") == "Available":
    arcpy.CheckOutExtension("Network")
else:
    arcpy.AddMessage("Network Analyst Extension Is Not Available")
    print "Network Analyst Is Not Available"
    sys.exit(0)

# Script arguments
Layer_Location = arcpy.GetParameterAsText(1)
if Layer_Location == '#' or not Layer_Location:
    raise ValueError("layer location is missing")

# load layer file and loop over all network layers
lyrFile = arcpy.mapping.Layer(Layer_Location)
for lyr in arcpy.mapping.ListLayers(lyrFile):
    desc = arcpy.Describe(Layer_Location + "\\" + lyr.longName)
    try:
        # only solve if the layer is associated with the evacuation solver
        if desc.solverName == "Evacuation Solver":
            arcpy.SetProgressor("step", "Solving " + lyr.longName + "...", 0, 100, 1)
            arcpy.Solve_na(lyr, "SKIP", "TERMINATE")
            
            # load all solver messages and push them to the geoprocessor
            for msg in range(0, arcpy.GetMessageCount()):
                arcpy.AddReturnMessage(msg)
            arcpy.AddMessage("Solved " + lyr.longName)
    except AttributeError:
        pass
    del desc

# This is where we save (overwrite) solved layers. If you want to keep the
# original file untouched then do a SaveAs.
lyrFile.save()
del lyrFile

# Set output parameter
arcpy.SetParameter(2, True)
arcpy.CheckInExtension("Network")

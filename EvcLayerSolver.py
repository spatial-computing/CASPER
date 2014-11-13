# -*- #################
# ---------------------------------------------------------------------------
# EvcLayerSolver.py
# Created on: 2014-06-23 12:00:25.00000
# Usage: EvcLayerSolver.py <Workspace> <Layer_File_Name> 
# Description: Solves all networtk layers within a layer file and saves the result
#              The workspace is where the network dataset exist.
# ---------------------------------------------------------------------------

# Import arcpy module
import arcpy
arcpy.env.workspace = arcpy.GetParameterAsText(0)
arcpy.SetParameter(2, False)

# Check out any necessary licenses
arcpy.CheckOutExtension("Network")

# Script arguments
Layer_Location = arcpy.GetParameterAsText(1)
if Layer_Location == '#' or not Layer_Location:
    raise ValueError("layer location is missing")

lyrFile = arcpy.mapping.Layer(Layer_Location)
for lyr in arcpy.mapping.ListLayers(lyrFile):
    desc = arcpy.Describe(Layer_Location + "\\" + lyr.longName)
    try:
        if desc.solverName == "Evacuation Solver":
            arcpy.SetProgressor("step", "Solving " + lyr.longName + "...", 0, 100, 1)
            arcpy.Solve_na(lyr, "SKIP", "TERMINATE")            
            for msg in range(0, arcpy.GetMessageCount()):
                arcpy.AddReturnMessage(msg)
            arcpy.AddMessage("Solved " + lyr.longName)
    except AttributeError:
        pass
    del desc
lyrFile.save()
del lyrFile

# Set output parameter
arcpy.SetParameter(2, True)















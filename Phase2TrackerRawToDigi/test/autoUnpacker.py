#!/usr/bin


import os,sys


inFolder = "/home/xtaldaq/storageManager/closed"
outFolder = "/home/xtaldaq/unpacker_output/rawData" 
logFolder = "/home/xtaldaq/unpacker_output/log"
template = "/home/xtaldaq/test_unpacker/CMSSW_6_2_0_SLHC16/src/EventFilter/Phase2TrackerRawToDigi/test/DigiProducer_template.py"
runFolder = "/home/xtaldaq/test_unpacker/CMSSW_6_2_0_SLHC16/src/EventFilter/Phase2TrackerRawToDigi/test"
shTemplateFile = "/home/xtaldaq/test_unpacker/CMSSW_6_2_0_SLHC16/src/EventFilter/Phase2TrackerRawToDigi/test/run_template.sh" 


templateFile = open(template,'r')
template=templateFile.read()
templateFile.close()


already_processed = []
for file in os.listdir(outFolder):
	if file.split(".")[-1]=="root":
		already_processed.append(file)


os.system("mkdir temp")

cmd=[]
for file in os.listdir(inFolder):
	if not file[-4:]==".dat" or file.replace(".dat",".root") in already_processed:
		print "Will skip file %s" % file
		continue	
	print "Will process file %s" % file
	
	runNumber=int(file.split(".")[1])
  	

	config=template.replace("__FILE_IN__",inFolder+"/"+file).replace("__FILE_OUT__",outFolder+"/"+file.replace(".dat",".root"))

	config=config.replace("__LOG_DETAILED__",logFolder+"/run%s_detailed.log"%runNumber)
	config=config.replace("__LOG_CRITICAL__",logFolder+"/run%s_critical.log"%runNumber)
	configFileName = runFolder+"/temp/unpacker_config_%s.py"%runNumber
	configFile=open(configFileName,"w")

	configFile.write(config)
	cmd.append("echo Processing file %s"%file)
	cmd.append("cmsRun temp/%s >> %s/run%s_cout.log 2>&1"%(configFileName.split("/")[-1],logFolder,runNumber))

cmdString=""
for c in cmd:
	cmdString+=c
	cmdString+="\n"

shTemplate = open(shTemplateFile,"r")
toExecute = shTemplate.read().replace("__PWD__",runFolder).replace("__CMD__",cmdString)
shTemplate.close()

finalFile=open(runFolder+"/temp/runall.sh","w")
finalFile.write(toExecute)
finalFile.close()

os.system("chmod +x %s/temp/runall.sh"%(runFolder))
  

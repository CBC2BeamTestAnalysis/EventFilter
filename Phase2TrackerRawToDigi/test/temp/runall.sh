#!/bin/bash
cd /home/xtaldaq/test_unpacker/CMSSW_6_2_0_SLHC16/src/EventFilter/Phase2TrackerRawToDigi/test

echo Processing file USC.00000658.0001.A.storageManager.00.0000.dat
cmsRun temp/unpacker_config_658.py >> /home/xtaldaq/unpacker_output/log/run658_cout.log 2>&1




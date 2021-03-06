import FWCore.ParameterSet.Config as cms
import sys

process = cms.Process("RawToDigi")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger = cms.Service("MessageLogger",
       destinations   = cms.untracked.vstring('detailedInfo', 'critical' ),
       detailedInfo   = cms.untracked.PSet( threshold  = cms.untracked.string('DEBUG') ),
       debugModules = cms.untracked.vstring( 'Phase2TrackerDigiProducer', 'Phase2TrackerFEDBuffer' )
)
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1 ))


#process.source = cms.Source("PoolSource",
process.source = cms.Source("NewEventStreamFileReader",
    fileNames = cms.untracked.vstring( 'file:'+sys.argv[-1])
)


#process.load('CalibTracker.SiStripESProducers.fake.Phase2TrackerConfigurableCablingESSource_cfi')
process.load('TestbeamCabling_cfi')
#process.load('DummyCablingTxt_cfi')
process.load('EventFilter.Phase2TrackerRawToDigi.Phase2TrackerCommissioningDigiProducer_cfi')
process.load('EventFilter.Phase2TrackerRawToDigi.Phase2TrackerDigiProducer_cfi')
#process.Phase2TrackerDigiProducer.ProductLabel = cms.InputTag("Phase2TrackerDigiToRawProducer")
process.Phase2TrackerDigiProducer.ProductLabel = cms.InputTag("rawDataCollector")
process.Phase2TrackerCommissioningDigiProducer.ProductLabel = cms.InputTag("rawDataCollector")
#process.load('EventFilter.Phase2TrackerRawToDigi.Phase2TrackerCommissioningDigiProducer_cfi')

process.out = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('~/unpacker_output/myOutputFile.root')
)


#process.out = cms.OutputModule(
#    "PoolOutputModule",
#    fileName = cms.untracked.string('rawtodigi.root'),
#    outputCommands = cms.untracked.vstring(
#      'drop *',
#      'keep *_Phase2TrackerDigiProducer_*_*'
#      )
#    )

#process.p = cms.Path()

process.p = cms.Path(process.Phase2TrackerDigiProducer*process.Phase2TrackerCommissioningDigiProducer)
#process.p = cms.Path(process.Phase2TrackerDigitestproducer*process.Phase2TrackerDigiCondDataproducer)

process.e = cms.EndPath(process.out)

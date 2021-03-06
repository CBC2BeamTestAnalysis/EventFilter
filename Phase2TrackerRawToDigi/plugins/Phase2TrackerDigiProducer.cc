#include "EventFilter/Phase2TrackerRawToDigi/plugins/Phase2TrackerDigiProducer.h"
#include "DataFormats/Common/interface/DetSet.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/src/fed_header.h"
#include "DataFormats/FEDRawData/src/fed_trailer.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/Phase2TrackerFEDBuffer.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/Phase2TrackerFEDChannel.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/Phase2TrackerFEDHeader.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/Phase2TrackerFEDRawChannelUnpacker.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/Phase2TrackerFEDZSChannelUnpacker.h"
#include "EventFilter/Phase2TrackerRawToDigi/interface/utils.h"
#include "CondFormats/DataRecord/interface/Phase2TrackerCablingRcd.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ext/algorithm>


#define EDM_ML_LOGDEBUG
#define EDM_ML_DEBUG
using namespace std;

namespace Phase2Tracker {
  SiPixelCluster makeSiPixelCluster(int x, int sizex, int y = 0)
  {
    uint16_t adcs[sizex];
    std::fill_n(adcs,sizex,255);
    uint16_t xpos[sizex];
    uint16_t ypos[sizex];
    for (int i=0; i<sizex; i++)
    {
      xpos[i] = x + i;
      ypos[i] = y;
    }
    SiPixelCluster cluster (sizex, adcs, xpos, ypos, x, y);
    return cluster;
  }

  typedef Phase2TrackerDigi DummyClusterDigi;

  Phase2TrackerDigiProducer::Phase2TrackerDigiProducer( const edm::ParameterSet& pset ) :
    runNumber_(0),
    cabling_(0),
    cacheId_(0)
  {
    // define product
    produces< edm::DetSetVector<Phase2TrackerDigi> >("Unsparsified");
    produces< edmNew::DetSetVector<SiPixelCluster> > ("Sparsified");
    token_ = consumes<FEDRawDataCollection>(pset.getParameter<edm::InputTag>("ProductLabel"));
  }
  
  Phase2TrackerDigiProducer::~Phase2TrackerDigiProducer()
  {
cout<<"deleted"<<endl;
  }
  
  void Phase2TrackerDigiProducer::_beginJob( )
  {
cout<<"sniff"<<endl;
  }
  
  void Phase2TrackerDigiProducer::beginRun( edm::Run const& run, edm::EventSetup const& es)
  {
    cout<<"Begin Job..."<<endl;
    // fetch cabling from event setup
    edm::ESHandle<Phase2TrackerCabling> c;
    es.get<Phase2TrackerCablingRcd>().get( c );
    cabling_ = c.product();
  }
  
  void Phase2TrackerDigiProducer::endJob()
  {
	cout<<"END JOB..."<<endl;
  }
  
  void Phase2TrackerDigiProducer::produce( edm::Event& event, const edm::EventSetup& es)
  {
   cout<<"Producing an event"<<endl;
    // empty vectors for the next event
    proc_work_registry_.clear();    
    proc_work_digis_.clear();
    zs_work_registry_.clear();    
    zs_work_digis_.clear();

    // NEW
    std::auto_ptr<edmNew::DetSetVector<SiPixelCluster>> clusters( new edmNew::DetSetVector<SiPixelCluster>() ); 

    // Retrieve FEDRawData collection
    edm::Handle<FEDRawDataCollection> buffers;
    event.getByToken( token_, buffers );

    // Analyze strip tracker FED buffers in data
    std::vector<int> feds = cabling_->listFeds();
    std::vector<int>::iterator fedIndex;
    for(fedIndex = feds.begin(); fedIndex != feds.end(); ++fedIndex)
    {
      const FEDRawData& fed = buffers->FEDData(*fedIndex);
      if(fed.size()==0) continue;
	  // construct buffer
	  Phase2Tracker::Phase2TrackerFEDBuffer buffer(fed.data(),fed.size());
      // Skip FED if buffer is not a valid tracker FEDBuffer
      if(buffer.isValid() == 0) 
      {
        LogTrace("Phase2TrackerDigiProducer") << "invalid buffer" << endl;
        //continue; //uncomment
      }

      #ifdef EDM_ML_DEBUG
      std::ostringstream ss;
	  ss << " -------------------------------------------- " << endl;
	  ss << " buffer debug ------------------------------- " << endl;
	  ss << " -------------------------------------------- " << endl;
	  ss << " buffer size : " << buffer.bufferSize() << endl;
	  ss << " fed id      : " << *fedIndex << endl;
	  ss << " -------------------------------------------- " << endl;
	  ss << " tracker header debug ------------------------" << endl;
	  ss << " -------------------------------------------- " << endl;
      LogTrace("Phase2TrackerDigiProducer") << ss.str(); ss.clear(); ss.str("");
      #endif 

	  Phase2TrackerFEDHeader tr_header = buffer.trackerHeader();

      #ifdef EDM_ML_DEBUG
	 cout<<"test"<<endl;
	  ss << " Version  : " << hex << setw(2) << (int) tr_header.getDataFormatVersion() << endl;
	  ss << " Mode     : " << hex << setw(2) << tr_header.getDebugMode() << endl;
	  ss << " Type     : " << hex << setw(2) << (int) tr_header.getEventType() << endl;
	  ss << " Readout  : " << hex << setw(2) << tr_header.getReadoutMode() << endl;
      ss << " Condition Data : " << ( tr_header.getConditionData() ? "Present" : "Absent") << "\n";
      ss << " Data Type      : " << ( tr_header.getDataType() ? "Real" : "Fake" ) << "\n";
	  ss << " Status   : " << hex << setw(16)<< (int) tr_header.getGlibStatusCode() << endl;
	  ss << " FE stat  : " ;
	  for(int i=MAX_FE_PER_FED-1; i>=0; i--)
	  {
	    if((tr_header.frontendStatus())[i])
	    {
	      ss << "1";
	    }
	    else
	    {
	      ss << "0";
	    }
	  }
	  ss << endl;
	  ss << " Nr CBC   : " << hex << setw(16)<< (int) tr_header.getNumberOfCBC() << endl;
	  ss << " CBC stat : ";
	  for(int i=0; i<tr_header.getNumberOfCBC(); i++)
	  {
	    ss << hex << setw(2) << (int) tr_header.CBCStatus()[i] << " ";
	  } 
	  ss << endl;
      LogTrace("Phase2TrackerDigiProducer") << ss.str(); ss.clear(); ss.str("");
	  ss << " -------------------------------------------- " << endl;
	  ss << " Payload  ----------------------------------- " << endl;
	  ss << " -------------------------------------------- " << endl;
	  #endif
      // check readout mode
      if(tr_header.getReadoutMode() == READOUT_MODE_PROC_RAW)
      {
	    // loop channels
	    int ichan = 0;
	    for ( int ife = 0; ife < MAX_FE_PER_FED; ife++ )
	    {
	      for ( int icbc = 0; icbc < MAX_CBC_PER_FE; icbc++ )
	      {
	        const Phase2TrackerFEDChannel& channel = buffer.channel(ichan);
	        if(channel.length() > 0)
	        {
              // get detid from cabling
              const Phase2TrackerModule mod = cabling_->findFedCh(std::make_pair(*fedIndex, ife));
              uint32_t detid = mod.getDetid();
              #ifdef EDM_ML_DEBUG
              ss << dec << " id from cabling : " << detid << endl;
              ss << dec << " reading channel : " << icbc << " on FE " << ife;
	          ss << dec << " with length  : " << (int) channel.length() << endl;
	          #endif

              // container for this channel's digis
              std::vector<Phase2TrackerDigi> stripsTop;
              std::vector<Phase2TrackerDigi> stripsBottom;

              // unpacking data
              // TODO : set Y position in digi as a function of the side of the CBC
	          Phase2TrackerFEDRawChannelUnpacker unpacker = Phase2TrackerFEDRawChannelUnpacker(channel);
	          while (unpacker.hasData())
	          {
		        if(unpacker.stripOn())
		        { 
                  if (unpacker.stripIndex()%2) 
                  {
		            stripsTop.push_back(Phase2TrackerDigi( (int) (STRIPS_PER_CBC*icbc + unpacker.stripIndex())/2, 0));
                    #ifdef EDM_ML_DEBUG
                    ss << "t";
	                #endif
                  }
                  else 
                  {
                    stripsBottom.push_back(Phase2TrackerDigi( (int) (STRIPS_PER_CBC*icbc + unpacker.stripIndex())/2, 0));
                    #ifdef EDM_ML_DEBUG
                    ss << "b";
	                #endif
                  }
		        } 
                else
		        {  
                  #ifdef EDM_ML_DEBUG
                  ss << "_";
	              #endif
		        }
		        unpacker++;
	          }
              #ifdef EDM_ML_DEBUG
	          ss << endl;
              LogTrace("Phase2TrackerDigiProducer") << ss.str(); ss.clear(); ss.str("");
	          #endif

              // store beginning and end of this digis for this detid and add this registry to the list
              // and store data
              Registry regItemTop(detid, STRIPS_PER_CBC*icbc/2, proc_work_digis_.size(), stripsTop.size());
              proc_work_registry_.push_back(regItemTop);
              proc_work_digis_.insert(proc_work_digis_.end(),stripsTop.begin(),stripsTop.end());
              Registry regItemBottom(detid+4, STRIPS_PER_CBC*icbc/2, proc_work_digis_.size(), stripsBottom.size());
              proc_work_registry_.push_back(regItemBottom);
              proc_work_digis_.insert(proc_work_digis_.end(),stripsBottom.begin(),stripsBottom.end());
	        }
	        ichan ++;
	      }
	    } // end loop on channels
      }
      else if (tr_header.getReadoutMode() == READOUT_MODE_ZERO_SUPPRESSED)
      {
        // loop channels
        int ichan = 0;
        for ( int ife = 0; ife < MAX_FE_PER_FED; ife++ )
        {
          // get fedid from cabling
          const Phase2TrackerModule mod = cabling_->findFedCh(std::make_pair(*fedIndex, ife));
          uint32_t detid = mod.getDetid();
          // container for this module's digis
          std::vector<SiPixelCluster> clustersTop;
          std::vector<SiPixelCluster> clustersBottom;
          // looping over concentrators (4 virtual concentrators in case of PS)
          for ( int iconc = 0; iconc < 4; iconc++ )
          {
            const Phase2TrackerFEDChannel& channel = buffer.channel(ichan);
            if(channel.length() > 0)
            {
              #ifdef EDM_ML_DEBUG
              ss << dec << " id from cabling : " << detid << endl;
              ss << dec << " reading channel : " << iconc << " on FE " << ife;
              ss << dec << " with length  : " << (int) channel.length() << endl;
              #endif
              // create appropriate unpacker
              if (channel.dettype() == DET_Son2S) 
              {
                Phase2TrackerFEDZSSon2SChannelUnpacker unpacker = Phase2TrackerFEDZSSon2SChannelUnpacker(channel);
                while (unpacker.hasData())
                {
                  #ifdef EDM_ML_DEBUG
                  ss << std::dec << "Son2S " << (int)unpacker.clusterX() << " " << (int)unpacker.clusterSize() << " " << unpacker.chipId() << endl;
                  #endif
                  if (unpacker.rawX()%2) 
                  {
	      	        clustersTop.push_back(makeSiPixelCluster(unpacker.clusterX(),unpacker.clusterSize(),unpacker.clusterY()));
                  }
                  else 
                  {
                    clustersBottom.push_back(makeSiPixelCluster(unpacker.clusterX(),unpacker.clusterSize(),unpacker.clusterY()));
                  }
                  unpacker++;
                }
              } 
              else if (channel.dettype() == DET_SonPS)
              {
                Phase2TrackerFEDZSSonPSChannelUnpacker unpacker = Phase2TrackerFEDZSSonPSChannelUnpacker(channel);
                while (unpacker.hasData())
                {
                  #ifdef EDM_ML_DEBUG
                  ss << std::dec << "SonPS " << (int)unpacker.clusterX() << " " << (int)unpacker.clusterSize() << " " << unpacker.chipId() << endl;
                  #endif
                  clustersTop.push_back(makeSiPixelCluster(unpacker.clusterX(),unpacker.clusterSize(),unpacker.clusterY()));
                  unpacker++;
                }
              }
              else if (channel.dettype() == DET_PonPS)
              {
                Phase2TrackerFEDZSPonPSChannelUnpacker unpacker = Phase2TrackerFEDZSPonPSChannelUnpacker(channel);
                while (unpacker.hasData())
                {
                  #ifdef EDM_ML_DEBUG
                  ss << std::dec << "PonPS " << (int)unpacker.clusterX() << " " << (int)unpacker.clusterSize() << " " << (int)unpacker.clusterY() << " " << unpacker.chipId() << endl;
                  #endif
                  clustersBottom.push_back(makeSiPixelCluster(unpacker.clusterX(),unpacker.clusterSize(),unpacker.clusterY()));
                  unpacker++;
                }
              }
            } // end reading CBC's channel
            ichan++;
          } // end loop on channels
          if(detid > 0)
          {
            std::vector<SiPixelCluster>::iterator it;
            {
              edmNew::DetSetVector<SiPixelCluster>::FastFiller spct(*clusters, detid+4);
              for(it=clustersTop.begin();it!=clustersTop.end();it++)
              {
                spct.push_back(*it);
              }
            }
            {
              edmNew::DetSetVector<SiPixelCluster>::FastFiller spcb(*clusters, detid);
              for(it=clustersBottom.begin();it!=clustersBottom.end();it++)
              {
                spcb.push_back(*it);
              }
            }
          }
        } // end loop on FE
          // store digis in edm collections
      }
      else
      {
        // readout modes are checked in getreadoutMode(), so we should never end up here
      }
    }   
    // sort and store Unsparsified digis
    std::sort( proc_work_registry_.begin(), proc_work_registry_.end() );
    std::vector< edm::DetSet<Phase2TrackerDigi> > sorted_and_merged;
    edm::DetSetVector<Phase2TrackerDigi>* pr = new edm::DetSetVector<Phase2TrackerDigi>();
    std::vector<Registry>::iterator it = proc_work_registry_.begin(), it2 = it+1, end = proc_work_registry_.end();
    while (it < end) 
    {
      sorted_and_merged.push_back( edm::DetSet<Phase2TrackerDigi>(it->detid) );
      std::vector<Phase2TrackerDigi> & digis = sorted_and_merged.back().data;
      // first count how many digis we have
      size_t len = it->length;
      for (it2 = it+1; (it2 != end) && (it2->detid == it->detid); ++it2) { len += it2->length; }
      // reserve memory 
      digis.reserve(len);
      // push them in
      for (it2 = it+0; (it2 != end) && (it2->detid == it->detid); ++it2) 
      {
        digis.insert( digis.end(), & proc_work_digis_[it2->index], & proc_work_digis_[it2->index + it2->length] );
      }
      it = it2;
    }
    edm::DetSetVector<Phase2TrackerDigi> proc_raw_dsv( sorted_and_merged, true );
    pr->swap( proc_raw_dsv );
    std::auto_ptr< edm::DetSetVector<Phase2TrackerDigi> > pr_dsv(pr);
    event.put( pr_dsv, "Unsparsified" );

    // store Sparsified Digis
    event.put(clusters, "Sparsified" );
  } 
}

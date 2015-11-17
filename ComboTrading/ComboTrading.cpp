/************************************************************************
 * Copyright(c) 2015, One Unified. All rights reserved.                 *
 * email: info@oneunified.net                                           *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

// started 2015/11/08

#include <algorithm>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/phoenix/bind/bind_member_function.hpp>
#include <boost/lexical_cast.hpp>

#include <TFTrading/InstrumentManager.h>
#include <TFTrading/AccountManager.h>
#include <TFTrading/OrderManager.h>

#include <TFIQFeed/BuildSymbolName.h>
#include <TFIQFeed/BuildInstrument.h>

#include "ComboTrading.h"

/*
 * 20151109 To Do:
 *   based upon Day Trading Options by Jeff Augen
 *   read cboe file and sort symbols - done
 *   run the pivot scanner, filter by volume and volatility (install in library of scanners)
 *     or select based upon the c-c, o-c, c-o profiles and volatility surfaces
 *   sort top and pick 10 symbols (watch daily while building remaining code)
 *   watch with underlying plus 2 or 3 strike call/puts around at the money (option management)
 *   select combo and trade/watch the options watched from open (position/portfolio)
 *   track p/l over the day (portfolio)
 *   exit at day end, or some minimum profit level with trailing parabolic stop
 *   build up the volatility indicators mentioned in the boot 
 *   live chart for each symbol and associated combo 
 *   save the values at session end
 * 
 *  implied volatility surface: pg 84
 *  historical volatility calcs:  pg 91
 *  c-c, c-o, o-c hist. vol. calcs: pg 94
 *  calcs by weekday: pg 99
 *  strangles better than straddles: pg 108
 * 
 * then chapter 4 working with price spike charts
 * 
 * 20151112 - important steps:
 *  historical volatility
 *  track stocks at 20:00 'this all has a familiar ring to it'
 *  underlying and three strikes at atm.  trade them on opening
 *  track profit over day and exit same day or next day?
 *   es, gc, 
 *  hard code for now, build up infrastructure, and automate over time
 *  manually pick the symbols, 
 *  run a process for each and watch
 *  save at end of day
 *  wait for flag to exit.
 * 
 * 20151113 most important thing:
 *  track atm for a series of instruments
 *  keep a series of trades prepared
 *  show the intraday price spike charts
 *   
 * 20151115 other stuff to do:
 *  use iqfeed market symbol file, 
 *  build gui for selecting symbols given name, type, yr, mn, day, strike, exch, desc, ...
 */

IMPLEMENT_APP(AppComboTrading)

const std::string sFileNameMarketSymbolSubset( "../combotrading.ser" );

bool AppComboTrading::OnInit() {

  //bool bExit = GetExitOnFrameDelete();
  //SetExitOnFrameDelete( true );

  m_pFrameMain = new FrameMain( 0, wxID_ANY, "Combo Trading" );
  wxWindowID idFrameMain = m_pFrameMain->GetId();
  //m_pFrameMain->Bind( wxEVT_SIZE, &AppStrategy1::HandleFrameMainSize, this, idFrameMain );
  //m_pFrameMain->Bind( wxEVT_MOVE, &AppStrategy1::HandleFrameMainMove, this, idFrameMain );
  //m_pFrameMain->Center();
//  m_pFrameMain->Move( -2500, 50 );
  m_pFrameMain->SetSize( 800, 500 );
  SetTopWindow( m_pFrameMain );

  wxBoxSizer* m_sizerMain;
  m_sizerMain = new wxBoxSizer(wxVERTICAL);
  m_pFrameMain->SetSizer(m_sizerMain);

  wxBoxSizer* m_sizerControls;
  m_sizerControls = new wxBoxSizer( wxHORIZONTAL );
  m_sizerMain->Add( m_sizerControls, 0, wxLEFT|wxTOP|wxRIGHT, 5 );

  // populate variable in FrameWork01
  m_pPanelProviderControl = new ou::tf::PanelProviderControl( m_pFrameMain, wxID_ANY );
  m_sizerControls->Add( m_pPanelProviderControl, 0, wxEXPAND|wxALIGN_LEFT|wxRIGHT, 5);
  m_pPanelProviderControl->Show( true );

  m_tws->SetClientId( 1 );

  LinkToPanelProviderControl();

//  m_pPanelManualOrder = new ou::tf::PanelManualOrder( m_pFrameMain, wxID_ANY );
//  m_sizerControls->Add( m_pPanelManualOrder, 0, wxEXPAND|wxALIGN_LEFT|wxRIGHT, 5);
//  m_pPanelManualOrder->Enable( false );  // portfolio isn't working properly with manual order instrument field
//  m_pPanelManualOrder->Show( true );

/*
  m_pPanelOptionsParameters = new PanelOptionsParameters( m_pFrameMain, wxID_ANY );
  m_sizerControls->Add( m_pPanelOptionsParameters, 1, wxEXPAND|wxALIGN_LEFT, 0);
  m_pPanelOptionsParameters->Show( true );
  m_pPanelOptionsParameters->SetOnStart( MakeDelegate( this, &AppStrategyRunner::HandleBtnStart ) );
  m_pPanelOptionsParameters->SetOnStop( MakeDelegate( this, &AppStrategyRunner::HandleBtnStop ) );
  m_pPanelOptionsParameters->SetOnSave( MakeDelegate( this, &AppStrategyRunner::HandleBtnSave ) );
  m_pPanelOptionsParameters->SetOptionNearDate( boost::gregorian::date( 2012, 4, 20 ) );
  m_pPanelOptionsParameters->SetOptionFarDate( boost::gregorian::date( 2012, 6, 15 ) );
*/

  wxBoxSizer* m_sizerStatus = new wxBoxSizer( wxHORIZONTAL );
  m_sizerMain->Add( m_sizerStatus, 1, wxEXPAND|wxALL, 5 );

  m_pPanelLogging = new ou::tf::PanelLogging( m_pFrameMain, wxID_ANY );
  //m_sizerStatus->Add( m_pPanelLogging, 1, wxALL | wxEXPAND|wxALIGN_LEFT|wxALIGN_RIGHT|wxALIGN_TOP|wxALIGN_BOTTOM, 0);
  m_sizerStatus->Add( m_pPanelLogging, 1, wxALL | wxEXPAND, 0);
  m_pPanelLogging->Show( true );

  m_pFrameMain->Show( true );

  m_db.OnRegisterTables.Add( MakeDelegate( this, &AppComboTrading::HandleRegisterTables ) );
  m_db.OnRegisterRows.Add( MakeDelegate( this, &AppComboTrading::HandleRegisterRows ) );
  m_db.SetOnPopulateDatabaseHandler( MakeDelegate( this, &AppComboTrading::HandlePopulateDatabase ) );
  m_db.SetOnLoadDatabaseHandler( MakeDelegate( this, &AppComboTrading::HandleLoadDatabase ) );

  // maybe set scenario with database and with in memory data structure
//  m_idPortfolio = boost::gregorian::to_iso_string( boost::gregorian::day_clock::local_day() ) + "StickShift";
  m_idPortfolioMaster = "master";  // keeps name constant over multiple days

  m_sDbName = "ComboTrading.db";
  if ( boost::filesystem::exists( m_sDbName ) ) {
//    boost::filesystem::remove( sDbName );
  }

  m_bData1Connected = false;
  m_bExecConnected = false;
  m_bStarted = false;

  m_dblMinPL = m_dblMaxPL = 0.0;

  m_pIQFeedSymbolListOps = new ou::tf::IQFeedSymbolListOps( m_listIQFeedSymbols ); 

  FrameMain::vpItems_t vItems;
  typedef FrameMain::structMenuItem mi;  // vxWidgets takes ownership of the objects
  vItems.push_back( new mi( "a1 New Symbol List Remote", MakeDelegate( m_pIQFeedSymbolListOps, &ou::tf::IQFeedSymbolListOps::ObtainNewIQFeedSymbolListRemote ) ) );
  vItems.push_back( new mi( "a2 New Symbol List Local", MakeDelegate( m_pIQFeedSymbolListOps, &ou::tf::IQFeedSymbolListOps::ObtainNewIQFeedSymbolListLocal ) ) );
  vItems.push_back( new mi( "a3 Load Symbol List", MakeDelegate( m_pIQFeedSymbolListOps, &ou::tf::IQFeedSymbolListOps::LoadIQFeedSymbolList ) ) );
  vItems.push_back( new mi( "a4 Save Symbol Subset", MakeDelegate( this, &AppComboTrading::HandleMenuActionSaveSymbolSubset ) ) );
  vItems.push_back( new mi( "a5 Load Symbol Subset", MakeDelegate( this, &AppComboTrading::HandleMenuActionLoadSymbolSubset ) ) );
  m_pFrameMain->AddDynamicMenu( "Symbol List", vItems );
  
  vItems.clear();
  vItems.push_back( new mi( "load weeklies", MakeDelegate( &m_process, &Process::LoadWeeklies ) ) );
  m_pFrameMain->AddDynamicMenu( "Process", vItems );

  m_timerGuiRefresh.SetOwner( this );

  Bind( wxEVT_TIMER, &AppComboTrading::HandleGuiRefresh, this, m_timerGuiRefresh.GetId() );
  m_timerGuiRefresh.Start( 250 );

  m_pFrameMain->Bind( wxEVT_CLOSE_WINDOW, &AppComboTrading::OnClose, this );  // start close of windows and controls

//  m_pPanelManualOrder->SetOnNewOrderHandler( MakeDelegate( this, &AppComboTrading::HandlePanelNewOrder ) );
//  m_pPanelManualOrder->SetOnSymbolTextUpdated( MakeDelegate( this, &AppComboTrading::HandlePanelSymbolText ) );
//  m_pPanelManualOrder->SetOnFocusPropogate( MakeDelegate( this, &AppComboTrading::HandlePanelFocusPropogate ) );

  m_pFPPOE = new FrameMain( m_pFrameMain, wxID_ANY, "Portfolio Management", wxDefaultPosition, wxSize( 900, 500 ),  
    wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
    );

  m_sizerPM = new wxBoxSizer(wxVERTICAL);
  m_pFPPOE->SetSizer(m_sizerPM);

  //m_scrollPM = new wxScrolledWindow( m_pFPPOE, -1, wxDefaultPosition, wxSize(200, 400), wxVSCROLL );
  m_scrollPM = new wxScrolledWindow( m_pFPPOE, -1, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
  m_sizerPM->Add(m_scrollPM, 1, wxGROW|wxALL, 5);
  m_scrollPM->SetScrollbars(1, 1, 0, 0);

  m_sizerScrollPM = new wxBoxSizer(wxVERTICAL);
  m_scrollPM->SetSizer( m_sizerScrollPM );

  Bind( EVENT_IB_INSTRUMENT, &AppComboTrading::HandleIBInstrument, this );
  
  m_pFrameMain->SetAutoLayout( true );
  m_pFrameMain->Layout();

  m_pFPPOE->SetAutoLayout( true );
  m_pFPPOE->Layout();
  wxPoint point = m_pFPPOE->GetPosition();
  point.x += 500;
  m_pFPPOE->SetPosition( point );
  m_pFPPOE->Show();
  
  return 1;

}

void AppComboTrading::Start( void ) {
  // need iqfeed marketsymbols to be loaded before this can start
  if ( 0 == m_listIQFeedSymbols.Size() )
    throw std::runtime_error( "iqfeed market symbols need to be loaded first" );
  if ( !m_bStarted ) {
    try {
      // new stuff
      
      struct Underlying {
	std::string sName;
	std::string sIq;
	std::string sIb;
	Underlying( const std::string& sName_ ): sName( sName_ ), sIq( sName_ ), sIb( sName_ ) {};
	Underlying( const std::string& sName_, const std::string& sIq_, const std::string& sIb_ ):
	sName( sName_ ), sIq( sIq_ ), sIb( sIb_ ) {};
      };
      
      typedef std::vector<Underlying> vUnderlying_t;
      vUnderlying_t vUnderlying;
      
      // list of equities to monitor
      vUnderlying.push_back( Underlying( "GLD" ) );
      
      // create generic symbol: futures and equities, and possibly from weeklies
      struct Future {
	std::string sName;
	std::string sIqBaseName;
	boost::uint16_t nYear;
	boost::uint8_t nMonth;
	std::string sIbBaseName;
	Future( 
	  const std::string& sName_, const std::string& sIqBaseName_, 
	  boost::uint16_t nYear_, boost::uint8_t nMonth_, 
	  const std::string& sIbBaseName_ ):
		sName( sName_ ), sIqBaseName( sIqBaseName_ ), 
		nYear( nYear_ ), nMonth( nMonth_ ), 
		sIbBaseName( sIbBaseName_ ) 
	{}
      };
      
      typedef std::vector<Future> vFuture_t;
      vFuture_t vFuture;
      
      // list of futures to monitor
      vFuture.push_back( Future( "GC-2015-12", "QGC", 2015, 12, "GC" ) );
      
      // build futures and add to vUnderlying
      // ie, create the iqfeed name for the future, then pass off the name for instrument building
      for ( vFuture_t::const_iterator iter = vFuture.begin(); vFuture.end() != iter; ++iter ) {
	std::string sName = ou::tf::iqfeed::BuildFuturesName( iter->sIqBaseName, iter->nYear, iter->nMonth );
	vUnderlying.push_back( Underlying( iter->sName, sName, iter->sIqBaseName ) );
      }
      
      struct BuildInstrument {
	typedef ou::tf::Instrument::pInstrument_t pInstrument_t;
	typedef ou::tf::iqfeed::InMemoryMktSymbolList list_t;
	typedef list_t::trd_t trd_t;
	const list_t& list;
	BuildInstrument( const list_t& list_ ): list( list_ ) {};
	void operator()( const Underlying& underlying ) {
	  pInstrument_t pInstrument;
	  const trd_t& trd( list.GetTrd( underlying.sIq ) );
	  switch ( trd.sc ) {
	    case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::Equity:
	    case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::Future: 	  
	      pInstrument = ou::tf::iqfeed::BuildInstrument( underlying.sName, trd );
	      // now hand it off to the IB for contract insertion
	      break;
	    default:
	      throw std::runtime_error( "can't process the buildinstrument" );
	  }
	}
      };
      
      // build instruments.. equities and futures, then pass to the bundle to fill in the options
      
      std::for_each( vUnderlying.begin(), vUnderlying.end(), BuildInstrument( m_listIQFeedSymbols ) );
      
      // might as well bypass this and do the instrument directly and get it going through the cycle
      // the cycle being: 
      //  build the iqfeed instrument
      //  send it off to ib to get the contract
      //  then get it into multi-expiry 
      //  then start building the options
      // calculate expiry and flesh out bundle
      
      
      
      
      BundleTracking::BundleDetails details( "GC", "QGC" );
      // need to figure out proper expiry time
      // also need to figure if more options can be watched in order to provide volatility surfaces
      // I think I have a day calculator (which doesn't know holidays though )
      // and will need to convert to utc
      // if there are many symbols, strikes, and expiries, may need to run a different thread for calculating the greeks and IV
      boost::gregorian::date expiry2( boost::gregorian::date( 2015, 12, 28 ) );
      details.vOptionExpiryDay.push_back( expiry2 );
      boost::gregorian::date expiry3( boost::gregorian::date( 2016,  1, 26 ) );
      details.vOptionExpiryDay.push_back( expiry3 );
      
      m_vBundleTracking.push_back( BundleTracking( "GC" ) );  // can I do some sort of move semantics here?
      m_vBundleTracking.back().SetBundleParameters( details );
      
      // old stuff
      ou::tf::PortfolioManager& pm( ou::tf::PortfolioManager::GlobalInstance() );
      pm.OnPortfolioLoaded.Add( MakeDelegate( this, &AppComboTrading::HandlePortfolioLoad ) );
      pm.OnPositionLoaded.Add( MakeDelegate( this, &AppComboTrading::HandlePositionLoad ) );
      m_db.Open( m_sDbName );
      m_pFPPOE->Update();
      //m_pFPPOE->Refresh();
      //m_pFPPOE->SetAutoLayout( true );
      m_pFPPOE->Layout();  
      m_bStarted = true;
    }
    catch (...) {
      std::cout << "problems with AppStickShift::Start" << std::endl;
    }
  }
}

void AppComboTrading::HandleMenuActionSaveSymbolSubset( void ) {

  m_vExchanges.clear();
  m_vExchanges.insert( "NYSE" );
  //m_vExchanges.push_back( "NYSE_AMEX" );
  m_vExchanges.insert( "NYSE,NYSE_ARCA" );
  m_vExchanges.insert( "NASDAQ,NGSM" );
  m_vExchanges.insert( "NASDAQ,NGM" );
  m_vExchanges.insert( "OPRA" );
  m_vExchanges.insert( "TSE" );
  //m_vExchanges.push_back( "NASDAQ,NMS" );
  //m_vExchanges.push_back( "NASDAQ,SMCAP" );
  //m_vExchanges.push_back( "NASDAQ,OTCBB" );
  //m_vExchanges.push_back( "NASDAQ,OTC" );
  //m_vExchanges.insert( "CANADIAN,TSE" );  // don't do yet, simplifies contract creation for IB

  m_vClassifiers.clear();
  m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::Equity );
  m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::IEOption );

  std::cout << "Subsetting symbols ... " << std::endl;
  ou::tf::iqfeed::InMemoryMktSymbolList listIQFeedSymbols;
  ou::tf::IQFeedSymbolListOps::SelectSymbols selection( m_vClassifiers, listIQFeedSymbols );
  m_listIQFeedSymbols.SelectSymbolsByExchange( m_vExchanges.begin(), m_vExchanges.end(), selection );
  std::cout << "  " << listIQFeedSymbols.Size() << " symbols in subset." << std::endl;

  //std::string sFileName( sFileNameMarketSymbolSubset );
  std::cout << "Saving subset to " << sFileNameMarketSymbolSubset << " ..." << std::endl;
//  listIQFeedSymbols.HandleParsedStructure( m_listIQFeedSymbols.GetTrd( m_sNameUnderlying ) );
//  m_listIQFeedSymbols.SelectOptionsByUnderlying( m_sNameOptionUnderlying, listIQFeedSymbols );
  listIQFeedSymbols.SaveToFile( sFileNameMarketSymbolSubset );  // __.ser
  std::cout << " ... done." << std::endl;

  // next step will be to add in the options for the underlyings selected.
}

void AppComboTrading::HandleMenuActionLoadSymbolSubset( void ) {
  //std::string sFileName( sFileNameMarketSymbolSubset );
  std::cout << "Loading From " << sFileNameMarketSymbolSubset << " ..." << std::endl;
  m_listIQFeedSymbols.LoadFromFile( sFileNameMarketSymbolSubset );  // __.ser
  std::cout << "  " << m_listIQFeedSymbols.Size() << " symbols loaded." << std::endl;
}

void AppComboTrading::HandlePortfolioLoad( pPortfolio_t& pPortfolio ) {
  //ou::tf::PortfolioManager& pm( ou::tf::PortfolioManager::GlobalInstance() );
  m_pLastPPP = new ou::tf::PanelPortfolioPosition( m_scrollPM );
  m_sizerScrollPM->Add( m_pLastPPP, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 0);
  //pPPP->SetPortfolio( pm.GetPortfolio( idPortfolio ) );
  m_pLastPPP->SetPortfolio( pPortfolio );
  m_pLastPPP->SetNameLookup( MakeDelegate( this, &AppComboTrading::LookupDescription ) );
  m_pLastPPP->SetConstructPosition( MakeDelegate( this, &AppComboTrading::ConstructEquityPosition0 ) );
  m_pLastPPP->SetConstructPortfolio( MakeDelegate( this, &AppComboTrading::HandleConstructPortfolio ) );
  m_mapPortfolios.insert( mapPortfolios_t::value_type( pPortfolio->Id(), structPortfolio( m_pLastPPP ) ) );
}

void AppComboTrading::HandlePositionLoad( pPosition_t& pPosition ) {
  m_pLastPPP->AddPosition( pPosition );
}

void AppComboTrading::HandleGuiRefresh( wxTimerEvent& event ) {
  for ( mapPortfolios_t::iterator iter = m_mapPortfolios.begin(); m_mapPortfolios.end() != iter; ++iter ) {
    iter->second.pPPP->UpdateGui();
  }
}

void AppComboTrading::LookupDescription( const std::string& sSymbolName, std::string& sDescription ) {
  sDescription = "";
  try {
    const ou::tf::iqfeed::InMemoryMktSymbolList::trd_t& trd( m_listIQFeedSymbols.GetTrd( sSymbolName ) );
    sDescription = trd.sDescription;
  }
  catch ( std::runtime_error& e ) {
  }
}

void AppComboTrading::ConstructEquityPosition0( const std::string& sName, pPortfolio_t pPortfolio, DelegateAddPosition_t function ) {

  m_EquityPositionCallbackInfo.pPortfolio = pPortfolio;
  m_EquityPositionCallbackInfo.function = function;
  
  // QGC# for quote monitor
  // test symbol:  QGCZ15P1200
  // test symbol:  GLD1531X120

  ou::tf::InstrumentManager& im( ou::tf::InstrumentManager::GlobalInstance().Instance() );
  ou::tf::Instrument::pInstrument_t pInstrument;  //empty instrument
  
  // 20151025
  // can pull equity, future, option out of iqfeed marketsymbols file
  // construct basic instrument based upon iqfeed info 
  // then redo the following to obtain the contract info from ib and add to the instrument
  
  // 20151026 
  // maybe get rid of the whole underlying instrument requirement in an option symbol (similar to fact that futures option doesn't have underlying tie in )
  // requires a bunch of rewrite, but probably worth it
  //  20151115: maybe not, it provides a mechanism for getting at underlying's alternate names
  
  // 20151115 
  // need to build state builder to get contracts for a series of underlying symbols, then run through 
  //  associated options, and obtain their ib contract numbers
  // and feed the results into the multiexpirybundle
  // then stuff into a library for re-use
  
  // 

  // sName is going to be an IQFeed name from the MarketSymbols file, so needs to be in main as well as alternatesymbolsnames list
  if ( im.Exists( sName, pInstrument ) ) {  // the call will supply instrument if it exists
    ConstructEquityPosition1( pInstrument );
  }
  else {
    // this is going to have to be changed to reflect various symbols types recovered from the IQF Market Symbols file
    // which might be simplified if IB already has the code for interpreting a pInstrument_t

    bool bConstructed( false );
    try {
      const ou::tf::iqfeed::InMemoryMktSymbolList::trd_t& trd( m_listIQFeedSymbols.GetTrd( sName ) );
      
      switch ( trd.sc ) {
	case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::Equity:
	case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::Future: 
	case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::FOption:
//	  pInstrument = ou::tf::iqfeed::BuildInstrument( trd );
	  break;
	case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::IEOption: 
	{
	  ou::tf::Instrument::pInstrument_t pInstrumentUnderlying;
	  if ( im.Exists( trd.sUnderlying, pInstrumentUnderlying ) ) { // change called name to IfExistsSupplyInstrument
	  }
	  else {
	    // otherwise build instrument
	    const ou::tf::iqfeed::InMemoryMktSymbolList::trd_t& trdUnderlying( m_listIQFeedSymbols.GetTrd( trd.sUnderlying ) );
	    switch (trdUnderlying.sc ) {
	      case ou::tf::iqfeed::MarketSymbol::enumSymbolClassifier::Equity:
//		pInstrumentUnderlying = ou::tf::iqfeed::BuildInstrument( trdUnderlying );  // build the underlying in preparation for the option
		im.Register( pInstrumentUnderlying );
		// 20151115 this instrument should also obtain ib contract details, as it will land in multi-expiry bundle and be used in various option delta scenarios
		break;
	      default:
		throw std::runtime_error( "ConstructEquityPosition0: no applicable instrument type for underlying" );
	    }
	  }
//          pInstrument = ou::tf::iqfeed::BuildInstrument( trd, pInstrumentUnderlying );  // build an option
	}  // end case
	  break;
	default:
          throw std::runtime_error( "ConstructEquityPosition0: no applicable instrument type" );
      } // end switch
      bConstructed = true;
    } // end try
    catch ( std::runtime_error& e ) {
      std::cout << "Couldn't find symbol: " + sName << std::endl;
    }
    if ( bConstructed ) {
      std::cout << "Requesting IB: " << sName << std::endl;
      m_tws->RequestContractDetails( 
	pInstrument,
	MakeDelegate( this, &AppComboTrading::HandleIBContractDetails ), MakeDelegate( this, &AppComboTrading::HandleIBContractDetailsDone ) );
    }
  }
}

// futures expire:  17:15 est
// foption expire: 13:30 est
// option expire: 16:00 est
// holiday expire: 13:00 est 2015/11/27 - weekly option

void AppComboTrading::HandleIBContractDetails( const ou::tf::IBTWS::ContractDetails& details, pInstrument_t& pInstrument ) {
  QueueEvent( new EventIBInstrument( EVENT_IB_INSTRUMENT, -1, pInstrument ) );
  // this is in another thread, and should be handled in a thread safe manner.
}

void AppComboTrading::HandleIBContractDetailsDone( void ) {
}

void AppComboTrading::HandleIBInstrument( EventIBInstrument& event ) {
  ou::tf::InstrumentManager& im( ou::tf::InstrumentManager::GlobalInstance().Instance() );
  im.Register( event.GetInstrument() ); // by stint of being here, should be new, and therefore registerable (not true, as IB info might be added to iqfeed info)
  ConstructEquityPosition1( event.GetInstrument() );
}

// 20151025 problem with portfolio is 0 in m_EquityPositionCallbackInfo
void AppComboTrading::ConstructEquityPosition1( pInstrument_t& pInstrument ) {
  ou::tf::PortfolioManager& pm( ou::tf::PortfolioManager::GlobalInstance().Instance() );
  try {
    pPosition_t pPosition( pm.ConstructPosition( 
      m_EquityPositionCallbackInfo.pPortfolio->GetRow().idPortfolio,
      pInstrument->GetInstrumentName(),
      "",
      this->m_pExecutionProvider->GetName(),
      this->m_pData1Provider->GetName(),
      //"ib01", "iq01", 
      this->m_pExecutionProvider, this->m_pData1Provider,
      pInstrument ) 
      );
    if ( 0 != m_EquityPositionCallbackInfo.function ) {
      m_EquityPositionCallbackInfo.function( pPosition );
    }
  }
  catch ( std::runtime_error& e ) {
    std::cout << "Position Construction Error:  " << e.what() << std::endl;
  }
}

void AppComboTrading::HandleConstructPortfolio( ou::tf::PanelPortfolioPosition& ppp,const std::string& sPortfolioId, const std::string& sDescription ) {
  // check if portfolio exists
  if ( ou::tf::PortfolioManager::Instance().PortfolioExists( sPortfolioId ) ) {
    std::cout << "PortfolioId " << sPortfolioId << " already exists." << std::endl;
  }
  else {
    ou::tf::PortfolioManager::Instance().ConstructPortfolio( 
      sPortfolioId, "aoRay", ppp.GetPortfolio()->Id(),ou::tf::Portfolio::Standard, ppp.GetPortfolio()->GetRow().sCurrency, sDescription );
  }
}

void AppComboTrading::HandleRegisterTables(  ou::db::Session& session ) {
}

void AppComboTrading::HandleRegisterRows(  ou::db::Session& session ) {
}

void AppComboTrading::HandlePopulateDatabase( void ) {

  ou::tf::AccountManager::pAccountAdvisor_t pAccountAdvisor 
    = ou::tf::AccountManager::Instance().ConstructAccountAdvisor( "aaRay", "Raymond Burkholder", "One Unified" );

  ou::tf::AccountManager::pAccountOwner_t pAccountOwner
    = ou::tf::AccountManager::Instance().ConstructAccountOwner( "aoRay", "aaRay", "Raymond", "Burkholder" );

  ou::tf::AccountManager::pAccount_t pAccountIB
    = ou::tf::AccountManager::Instance().ConstructAccount( "ib01", "aoRay", "Raymond Burkholder", ou::tf::keytypes::EProviderIB, "Interactive Brokers", "acctid", "login", "password" );

  ou::tf::AccountManager::pAccount_t pAccountIQFeed
    = ou::tf::AccountManager::Instance().ConstructAccount( "iq01", "aoRay", "Raymond Burkholder", ou::tf::keytypes::EProviderIQF, "IQFeed", "acctid", "login", "password" );

  ou::tf::AccountManager::pAccount_t pAccountSimulator
    = ou::tf::AccountManager::Instance().ConstructAccount( "sim01", "aoRay", "Raymond Burkholder", ou::tf::keytypes::EProviderSimulator, "Sim", "acctid", "login", "password" );

  std::string sNull;

  m_pPortfolioMaster
    = ou::tf::PortfolioManager::Instance().ConstructPortfolio( 
      m_idPortfolioMaster, "aoRay", sNull, ou::tf::Portfolio::Master, ou::tf::Currency::Name[ ou::tf::Currency::USD ], "StickShift" );

  ou::tf::PortfolioManager::Instance().ConstructPortfolio(
    ou::tf::Currency::Name[ ou::tf::Currency::USD ], "aoRay", m_idPortfolioMaster, ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::USD ], "Currency Monitor" );
//  ou::tf::PortfolioManager::Instance().ConstructPortfolio(
//    ou::tf::Currency::Name[ ou::tf::Currency::CAD ], "aoRay", m_idPortfolio, ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::CAD ], "Currency Monitor" );
//  ou::tf::PortfolioManager::Instance().ConstructPortfolio(
//    ou::tf::Currency::Name[ ou::tf::Currency::EUR ], "aoRay", m_idPortfolio, ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::EUR ], "Currency Monitor" );
//  ou::tf::PortfolioManager::Instance().ConstructPortfolio(
//    ou::tf::Currency::Name[ ou::tf::Currency::AUD ], "aoRay", m_idPortfolio, ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::AUD ], "Currency Monitor" );
//  ou::tf::PortfolioManager::Instance().ConstructPortfolio(
//    ou::tf::Currency::Name[ ou::tf::Currency::GBP ], "aoRay", m_idPortfolio, ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::GBP ], "Currency Monitor" );
    
}

void AppComboTrading::HandleLoadDatabase( void ) {
    ou::tf::PortfolioManager& pm( ou::tf::PortfolioManager::GlobalInstance() );
    pm.LoadActivePortfolios();
}

/*
void AppComboTrading::HandlePanelNewOrder( const ou::tf::PanelManualOrder::Order_t& order ) {
  try {
    ou::tf::InstrumentManager& mgr( ou::tf::InstrumentManager::Instance() );
    //pInstrument_t pInstrument = m_vManualOrders[ m_curDialogManualOrder ].pInstrument;
    pInstrument_t pInstrument = m_IBInstrumentInfo.pInstrument;
    if ( !mgr.Exists( pInstrument ) ) {
      mgr.Register( pInstrument );
    }
//    if ( 0 == m_pPosition.get() ) {
//      m_pPosition = ou::tf::PortfolioManager::Instance().ConstructPosition( 
//        m_idPortfolioMaster, pInstrument->GetInstrumentName(), "manual", "ib01", "ib01", m_pExecutionProvider, m_pData1Provider, pInstrument );
//    }
    ou::tf::OrderManager& om( ou::tf::OrderManager::Instance() );
    ou::tf::OrderManager::pOrder_t pOrder;
    switch ( order.eOrderType ) {
    case ou::tf::OrderType::Market: 
      //pOrder = om.ConstructOrder( pInstrument, order.eOrderType, order.eOrderSide, order.nQuantity );
//      m_pPosition->PlaceOrder( ou::tf::OrderType::Market, order.eOrderSide, order.nQuantity );
      break;
    case ou::tf::OrderType::Limit:
      //pOrder = om.ConstructOrder( pInstrument, order.eOrderType, order.eOrderSide, order.nQuantity, order.dblPrice1 );
//      m_pPosition->PlaceOrder( ou::tf::OrderType::Limit, order.eOrderSide, order.nQuantity, order.dblPrice1 );
      break;
    case ou::tf::OrderType::Stop:
      //pOrder = om.ConstructOrder( pInstrument, order.eOrderType, order.eOrderSide, order.nQuantity, order.dblPrice1 );
//      m_pPosition->PlaceOrder( ou::tf::OrderType::Stop, order.eOrderSide, order.nQuantity, order.dblPrice1 );
      break;
    }
    //ou::tf::OrderManager::pOrder_t pOrder = om.ConstructOrder( pInstrument, order.eOrderType, order.eOrderSide, order.nQuantity, order.dblPrice1, order.dblPrice2 );
    //om.PlaceOrder( m_tws.get(), pOrder );
  }
  catch (...) {
    int i = 1;
  }
}
*/

void AppComboTrading::HandlePanelSymbolText( const std::string& sName ) {
  // need to fix to handle equity, option, future, etc.  merge with code from above so common code usage
  // 2014/09/30 maybe need to disable this panel, as the order doesn't land in an appropriate portfolio or position.
  ou::tf::IBTWS::Contract contract;
  contract.currency = "USD";
  contract.exchange = "SMART";
  contract.secType = "STK";
  contract.symbol = sName;
  // IB responds only when symbol is found, bad symbols will not illicit a response
//  m_pPanelManualOrder->SetInstrumentDescription( "" );
  m_tws->RequestContractDetails( 
    contract, 
    MakeDelegate( this, &AppComboTrading::HandleIBContractDetails ), MakeDelegate( this, &AppComboTrading::HandleIBContractDetailsDone ) );
}

void AppComboTrading::HandlePanelFocusPropogate( unsigned int ix ) {
}

void AppComboTrading::OnClose( wxCloseEvent& event ) {
//  pm.OnPortfolioLoaded.Remove( MakeDelegate( this, &AppStickShift::HandlePortfolioLoad ) );
//  pm.OnPositionLoaded.Remove( MakeDelegate( this, &AppStickShift::HandlePositionLoaded ) );
  m_timerGuiRefresh.Stop();
  DelinkFromPanelProviderControl();
//  if ( 0 != OnPanelClosing ) OnPanelClosing();
  // event.Veto();  // possible call, if needed
  // event.CanVeto(); // if not a 
  event.Skip();  // auto followed by Destroy();
}

int AppComboTrading::OnExit() {
    
  // called after destroying all application windows

  ou::tf::PortfolioManager& pm( ou::tf::PortfolioManager::GlobalInstance() );
  pm.OnPortfolioLoaded.Remove( MakeDelegate( this, &AppComboTrading::HandlePortfolioLoad ) );

//  DelinkFromPanelProviderControl();  generates stack errors
  m_timerGuiRefresh.Stop();
  if ( m_db.IsOpen() ) m_db.Close();

//  delete m_pCPPOE;
//  m_pCPPOE = 0;

  return wxApp::OnExit();
}

void AppComboTrading::OnData1Connected( int ) {
  m_bData1Connected = true;
  if ( m_bData1Connected & m_bExecConnected ) {
    // set start to enabled
    Start();
  }
}

void AppComboTrading::OnExecConnected( int ) {
  m_bExecConnected = true;
  if ( m_bData1Connected & m_bExecConnected ) {
    // set start to enabled
    Start();
  }
}

void AppComboTrading::OnData1Disconnected( int ) {
  m_bData1Connected = false;
}

void AppComboTrading::OnExecDisconnected( int ) {
  m_bExecConnected = false;
}

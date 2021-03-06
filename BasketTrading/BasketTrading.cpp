/************************************************************************
 * Copyright(c) 2012, One Unified. All rights reserved.                 *
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

// CAV.cpp : Defines the entry point for the application.

// needs daily bars downloaded via IQFeedGetHistory
// rebuild after changing date in Worker.cpp.

#include "stdafx.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/phoenix/bind/bind_member_function.hpp>
#include <boost/lexical_cast.hpp>

#include <TFTrading/InstrumentManager.h>
#include <TFTrading/AccountManager.h>
#include <TFTrading/OrderManager.h>

#include "BasketTrading.h"

IMPLEMENT_APP(AppBasketTrading)

namespace {  
 const std::string sFileNameMarketSymbolSubset( "BasketTrading.ser" );
}

bool AppBasketTrading::OnInit() {

  m_pFrameMain = new FrameMain( 0, wxID_ANY, "Basket Trading" );
  wxWindowID idFrameMain = m_pFrameMain->GetId();
  //m_pFrameMain->Bind( wxEVT_SIZE, &AppStrategy1::HandleFrameMainSize, this, idFrameMain );
  //m_pFrameMain->Bind( wxEVT_MOVE, &AppStrategy1::HandleFrameMainMove, this, idFrameMain );
  //m_pFrameMain->Center();
//  m_pFrameMain->Move( -2500, 50 );
  m_pFrameMain->SetSize( 500, 600 );
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

  m_pPanelBasketTradingMain = new PanelBasketTradingMain( m_pFrameMain, wxID_ANY );
  m_sizerControls->Add( m_pPanelBasketTradingMain, 0, wxEXPAND|wxALIGN_LEFT|wxRIGHT, 5);
  m_pPanelBasketTradingMain->Show( true );

  LinkToPanelProviderControl();
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
  m_pPanelPortfolioStats = new PanelPortfolioStats( m_pFrameMain, wxID_ANY );
  //m_sizerMain->Add( m_pPanelPortfolioStats, 1, wxEXPAND|wxALIGN_LEFT|wxRIGHT, 5);
  m_sizerMain->Add( m_pPanelPortfolioStats, 0, wxLEFT|wxTOP|wxRIGHT, 5);
  m_pPanelPortfolioStats->Show( true );

  wxBoxSizer* m_sizerStatus = new wxBoxSizer( wxHORIZONTAL );
  m_sizerMain->Add( m_sizerStatus, 1, wxEXPAND|wxALL, 5 );

  m_pPanelLogging = new ou::tf::PanelLogging( m_pFrameMain, wxID_ANY );
  m_sizerStatus->Add( m_pPanelLogging, 1, wxALL | wxEXPAND|wxALIGN_LEFT|wxALIGN_RIGHT|wxALIGN_TOP|wxALIGN_BOTTOM, 0);
  m_pPanelLogging->Show( true );

  m_pFrameMain->Show( true );

  m_db.OnRegisterTables.Add( MakeDelegate( this, &AppBasketTrading::HandleRegisterTables ) );
  m_db.OnRegisterRows.Add( MakeDelegate( this, &AppBasketTrading::HandleRegisterRows ) );
  m_db.SetOnPopulateDatabaseHandler( MakeDelegate( this, &AppBasketTrading::HandlePopulateDatabase ) );

  m_pWorker = 0;
  m_bData1Connected = false;
  m_bExecConnected = false;

  m_dblMinPL = m_dblMaxPL = 0.0;

  m_timerGuiRefresh.SetOwner( this );

  Bind( wxEVT_TIMER, &AppBasketTrading::HandleGuiRefresh, this, m_timerGuiRefresh.GetId() );

  m_pFrameMain->Bind( wxEVT_CLOSE_WINDOW, &AppBasketTrading::OnClose, this );  // start close of windows and controls

  // maybe set scenario with database and with in memory data structure
  m_sDbPortfolioName = boost::gregorian::to_iso_string( boost::gregorian::day_clock::local_day() ) + "Basket";
  m_db.Open( "BasketTrading.db" );
  
  m_pIQFeedSymbolListOps = new ou::tf::IQFeedSymbolListOps( m_listIQFeedSymbols ); 
  m_pIQFeedSymbolListOps->Status.connect( [this]( const std::string sStatus ){
    CallAfter( [sStatus](){ 
      std::cout << sStatus << std::endl; 
    });
  });  

  FrameMain::vpItems_t vItems;
  typedef FrameMain::structMenuItem mi;  // vxWidgets takes ownership of the objects
  vItems.push_back( new mi( "a1 Test Selection", MakeDelegate( this, &AppBasketTrading::HandleMenuActionTestSelection ) ) );
  vItems.push_back( new mi( "b1 Load Symbol List", MakeDelegate( m_pIQFeedSymbolListOps, &ou::tf::IQFeedSymbolListOps::LoadIQFeedSymbolList ) ) );
  vItems.push_back( new mi( "b2 Save Symbol Subset", MakeDelegate( this, &AppBasketTrading::HandleMenuActionSaveSymbolSubset ) ) );
  vItems.push_back( new mi( "b3 Load Symbol Subset", MakeDelegate( this, &AppBasketTrading::HandleMenuActionLoadSymbolSubset ) ) );
  m_pFrameMain->AddDynamicMenu( "Actions", vItems );

  m_pPanelBasketTradingMain->m_OnBtnLoad = MakeDelegate( this, &AppBasketTrading::HandleLoadButton );
  m_pPanelBasketTradingMain->m_OnBtnStart = MakeDelegate( this, &AppBasketTrading::HandleStartButton );
  m_pPanelBasketTradingMain->m_OnBtnExitPositions = MakeDelegate( this, &AppBasketTrading::HandleExitPositionsButton );
  m_pPanelBasketTradingMain->m_OnBtnStop = MakeDelegate( this, &AppBasketTrading::HandleStopButton );
  m_pPanelBasketTradingMain->m_OnBtnSave = MakeDelegate( this, &AppBasketTrading::HandleSaveButton );
  
  m_pMasterPortfolio.reset( new MasterPortfolio( 
    m_pExecutionProvider, m_pData1Provider, m_pData2Provider,
    [this](const std::string& sUnderlying, MasterPortfolio::fOptionDefinition_t f){
      m_listIQFeedSymbols.SelectOptionsByUnderlying( sUnderlying, f );
    },
    [this](const std::string& sIQFeedSymbolName)->const MasterPortfolio::trd_t& {
      return m_listIQFeedSymbols.GetTrd( sIQFeedSymbolName );
    },
    m_pPortfolio
    ) );

  return 1;

}

void AppBasketTrading::HandleGuiRefresh( wxTimerEvent& event ) {
  // update portfolio results and tracker timeseries for portfolio value
  double dblUnRealized;
  double dblRealized;
  double dblCommissionsPaid;
  double dblCurrent;
  m_pPortfolio->QueryStats( dblUnRealized, dblRealized, dblCommissionsPaid, dblCurrent );
  //double dblCurrent = dblUnRealized + dblRealized - dblCommissionsPaid;
  m_dblMaxPL = std::max<double>( m_dblMaxPL, dblCurrent );
  m_dblMinPL = std::min<double>( m_dblMinPL, dblCurrent );
  m_pPanelPortfolioStats->SetStats( 
    boost::lexical_cast<std::string>( m_dblMinPL ),
    boost::lexical_cast<std::string>( dblCurrent ),
    boost::lexical_cast<std::string>( m_dblMaxPL )
    );
}

void AppBasketTrading::HandleLoadButton() {
  CallAfter( // eliminates debug session lock up when gui/menu is not yet finished
    [this](){
      if ( 0 == m_pPortfolio.get() ) {  // if not newly created below, then load previously created portfolio
        // code currently does not allow a restart of session
        std::cout << "Cannot create new portfolio: " << m_sDbPortfolioName << std::endl;
        //m_pPortfolio = ou::tf::PortfolioManager::Instance().GetPortfolio( sDbPortfolioName );
        // this may create issues on mid-trading session restart.  most logic in the basket relies on newly created positions.
      }
      else {
        // need to change this later.... only start up once providers have been started
        // worker will change depending upon provider type
        // big worker when going live, hdf5 worker when simulating
        std::cout << "Starting Symbol Evaluation ... " << std::endl;
        // TODO: convert worker to something informative and use 
        //   established wx based threading arrangements
        m_pWorker = new Worker( MakeDelegate( this, &AppBasketTrading::HandleWorkerCompletion ) );
      }
    });
}

void AppBasketTrading::HandleStartButton(void) {
  CallAfter( 
    [this](){
      m_pMasterPortfolio->Start();
      m_timerGuiRefresh.Start( 250 );
    } );
}

void AppBasketTrading::HandleMenuActionTestSelection( void ) {
  CallAfter( 
    [this](){
      std::cout << "Starting Symbol Test ... " << std::endl;
      m_pWorker = new Worker( MakeDelegate( this, &AppBasketTrading::HandleMenuActionTestSelectionDone ) );
    });
}

void AppBasketTrading::HandleMenuActionTestSelectionDone( void ) {
  std::cout << "Selection Test Done" << std::endl;
}

void AppBasketTrading::HandleStopButton(void) {
  CallAfter( 
    [this](){
      m_pMasterPortfolio->Stop();
    });
}

void AppBasketTrading::HandleExitPositionsButton(void) {
  // to implement
}

void AppBasketTrading::HandleSaveButton(void) {
  CallAfter(
    [this](){
      m_pMasterPortfolio->SaveSeries( "/app/BasketTrading/" );
    });
}

void AppBasketTrading::HandleWorkerCompletion( void ) {  // called in worker thread, start processing in gui thread with CallAfter
  CallAfter([this](){
    m_pWorker->IterateInstrumentList( 
      boost::phoenix::bind( &MasterPortfolio::AddSymbol, m_pMasterPortfolio.get(), boost::phoenix::arg_names::arg1, boost::phoenix::arg_names::arg2, boost::phoenix::arg_names::arg3 ) );
    m_pWorker->Join();
    delete m_pWorker;
    m_pWorker = 0;
  });
}

void AppBasketTrading::OnData1Connected( int ) {
  m_bData1Connected = true;
  if ( m_bData1Connected & m_bExecConnected ) {
    // set start to enabled
  }
}

void AppBasketTrading::OnExecConnected( int ) {
  m_bExecConnected = true;
  if ( m_bData1Connected & m_bExecConnected ) {
    // set start to enabled
  }
}

void AppBasketTrading::OnData1Disconnected( int ) {
  m_bData1Connected = false;
}

void AppBasketTrading::OnExecDisconnected( int ) {
  m_bExecConnected = false;
}

void AppBasketTrading::OnClose( wxCloseEvent& event ) {
  m_timerGuiRefresh.Stop();
  DelinkFromPanelProviderControl();
//  if ( 0 != OnPanelClosing ) OnPanelClosing();
  // event.Veto();  // possible call, if needed
  // event.CanVeto(); // if not a 
  event.Skip();  // auto followed by Destroy();
}

int AppBasketTrading::OnExit() {

//  DelinkFromPanelProviderControl();  generates stack errors
  //m_timerGuiRefresh.Stop();
  if ( 0 != m_pWorker ) {
    delete m_pWorker;
    m_pWorker = 0; 
  }
  if ( m_db.IsOpen() ) m_db.Close();

  return 0;
}

void AppBasketTrading::HandleRegisterTables(  ou::db::Session& session ) {
}

void AppBasketTrading::HandleRegisterRows(  ou::db::Session& session ) {
}

void AppBasketTrading::HandlePopulateDatabase( void ) {

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

  m_pPortfolioMaster
    = ou::tf::PortfolioManager::Instance().ConstructPortfolio( 
    "Master", "aoRay", "", ou::tf::Portfolio::Master, ou::tf::Currency::Name[ ou::tf::Currency::USD ], "Basket of Equities" );

  m_pPortfolioCurrencyUSD
    = ou::tf::PortfolioManager::Instance().ConstructPortfolio( 
    "USD", "aoRay", "Master", ou::tf::Portfolio::CurrencySummary, ou::tf::Currency::Name[ ou::tf::Currency::USD ], "Basket of Equities" );

  m_pPortfolio
    = ou::tf::PortfolioManager::Instance().ConstructPortfolio( 
    m_sDbPortfolioName, "aoRay", "USD", ou::tf::Portfolio::MultiLeggedPosition, ou::tf::Currency::Name[ ou::tf::Currency::USD ], "Basket of Strategies" );

}

// maybe put this into background thread
void AppBasketTrading::HandleMenuActionSaveSymbolSubset( void ) {

  m_vExchanges.clear();
  m_vExchanges.insert( "NYSE" );
  //m_vExchanges.push_back( "NYSE_AMEX" );
  m_vExchanges.insert( "NYSE,NYSE_ARCA" );
  m_vExchanges.insert( "NASDAQ,NGSM" );
  m_vExchanges.insert( "NASDAQ,NGM" );
  //m_vExchanges.push_back( "NASDAQ,NMS" );
  //m_vExchanges.push_back( "NASDAQ,SMCAP" );
  //m_vExchanges.push_back( "NASDAQ,OTCBB" );
  //m_vExchanges.push_back( "NASDAQ,OTC" );
  m_vExchanges.insert( "OPRA" );
  //m_vExchanges.insert( "COMEX" );
  //m_vExchanges.insert( "COMEX,COMEX_GBX" );
  //m_vExchanges.insert( "TSE" );
  //m_vExchanges.insert( "CANADIAN,TSE" );  // don't do yet, simplifies contract creation for IB

  m_vClassifiers.clear();
  m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::Equity );
  m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::IEOption );
  //m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::Future );
  //m_vClassifiers.insert( ou::tf::IQFeedSymbolListOps::classifier_t::FOption );

  std::cout << "Subsetting symbols ... " << std::endl;
  ou::tf::iqfeed::InMemoryMktSymbolList listIQFeedSymbols;
  ou::tf::IQFeedSymbolListOps::SelectSymbols selection( m_vClassifiers, listIQFeedSymbols );
  m_listIQFeedSymbols.SelectSymbolsByExchange( m_vExchanges.begin(), m_vExchanges.end(), selection );
  std::cout << "  " << listIQFeedSymbols.Size() << " symbols in subset." << std::endl;

  std::cout << "Saving subset to " << sFileNameMarketSymbolSubset << " ..." << std::endl;
  listIQFeedSymbols.SaveToFile( sFileNameMarketSymbolSubset );  // __.ser
  std::cout << " ... done." << std::endl;

}

// TODO: set flag to only load once?  Otherwise, is the structure cleared first?
void AppBasketTrading::HandleMenuActionLoadSymbolSubset( void ) {
  std::cout << "Loading From " << sFileNameMarketSymbolSubset << " ..." << std::endl;
  m_listIQFeedSymbols.LoadFromFile( sFileNameMarketSymbolSubset );  // __.ser
  std::cout << "  " << m_listIQFeedSymbols.Size() << " symbols loaded." << std::endl;
}


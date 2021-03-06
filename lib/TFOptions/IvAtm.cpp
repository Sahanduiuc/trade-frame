/************************************************************************
 * Copyright(c) 2018, One Unified. All rights reserved.                 *
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

/* 
 * File:   AtmIv.cpp
 * Author: raymond@burkholder.net
 * 
 * Created on September 10, 2018, 10:43 PM
 */

#include <algorithm>

#include <TFHDF5TimeSeries/HDF5DataManager.h>
#include <TFHDF5TimeSeries/HDF5WriteTimeSeries.h>
#include <TFHDF5TimeSeries/HDF5Attribute.h>

#include "IvAtm.h"

namespace ou { // One Unified
namespace tf { // TradeFrame
namespace option { // options

IvAtm::IvAtm( pWatch_t pWatchUnderlying, fConstructOption_t fConstructOption, fStartCalc_t fStartCalc, fStopCalc_t fStopCalc ) 
:
  m_pWatchUnderlying( pWatchUnderlying ),
  m_fConstructOption( std::move( fConstructOption ) ),
  m_fStartCalc( std::move( fStartCalc ) ),
  m_fStopCalc( std::move( fStopCalc ) )
{ 
  assert( 0 != m_pWatchUnderlying.use_count() );
  assert( nullptr != m_pWatchUnderlying.get() );
  assert( nullptr != m_fConstructOption );
  assert( nullptr != m_fStartCalc );
  assert( nullptr != m_fStopCalc );
}

IvAtm::IvAtm( IvAtm&& rhs  )
  : m_pWatchUnderlying( rhs.m_pWatchUnderlying ),
    m_fConstructOption( std::move( rhs.m_fConstructOption ) ),
    m_fStartCalc( std::move( rhs.m_fStartCalc ) ),
    m_fStopCalc( std::move( rhs.m_fStopCalc ) ) 
{}

IvAtm::~IvAtm( ) { }

double IvAtm::Put_Itm( double value ) {
  mapChain_t::iterator iter = std::upper_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](double value, mapChain_t::value_type& vt)->bool{ return value < vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Put_Itm not found" );
  return iter->first;
}

double IvAtm::Put_ItmAtm( double value ) {
  mapChain_t::iterator iter = std::lower_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](const mapChain_t::value_type& vt, double value)->bool{ return value < vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Put_ItmAtm not found" );
  return iter->first;
}

double IvAtm::Put_Atm( double value ) {
  mapChain_t::iterator iter = FindStrike( value );
  return iter->first;
}

double IvAtm::Put_OtmAtm( double value ) {
  mapChain_t::iterator iter = std::lower_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](const mapChain_t::value_type& vt, double value)->bool{ return value > vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Put_OtmAtm not found" );
  return iter->first;
}

double IvAtm::Put_Otm( double value ) {
  mapChain_t::iterator iter = std::upper_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](double value, const mapChain_t::value_type& vt)->bool{ return value > vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Put_Otm not found" );
  return iter->first;
}
  
double IvAtm::Call_Itm( double value ) {
  mapChain_t::iterator iter = std::upper_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](double value, const mapChain_t::value_type& vt)->bool{ return value > vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Call_Itm not found" );
  return iter->first;
}

double IvAtm::Call_ItmAtm( double value ) {
  mapChain_t::iterator iter = std::lower_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](const mapChain_t::value_type& vt, double value)->bool{ return value > vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Call_ItmAtm not found" );
  return iter->first;
}

double IvAtm::Call_Atm( double value ) {
  mapChain_t::iterator iter = FindStrike( value );
  return iter->first;
}

double IvAtm::Call_OtmAtm( double value ) {
  mapChain_t::iterator iter = std::lower_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](const mapChain_t::value_type& vt, double value)->bool{ return value < vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Call_OtmAtm not found" );
  return iter->first;
}

double IvAtm::Call_Otm( double value ) {
  mapChain_t::iterator iter = std::upper_bound( 
    m_mapChain.begin(), m_mapChain.end(), value, 
    [](double value, const mapChain_t::value_type& vt)->bool{ return value < vt.first; } );
  if ( m_mapChain.end() == iter ) throw std::runtime_error( "Call_Otm not found" );
  return iter->first;
}
 
void IvAtm::SetIQFeedNameCall( double dblStrike, const std::string& sIQFeedSymbolName ) {
  mapChain_t::iterator iter = m_mapChain.find( dblStrike );
  if ( m_mapChain.end() == iter ) {
    iter = m_mapChain.insert( m_mapChain.begin(), mapChain_t::value_type( dblStrike, OptionsAtStrike() ) );
  }
  assert( 0 == iter->second.sCall.size() );
  iter->second.sCall = sIQFeedSymbolName;
}

void IvAtm::SetIQFeedNamePut( double dblStrike, const std::string& sIQFeedSymbolName ) {
  mapChain_t::iterator iter = m_mapChain.find( dblStrike );
  if ( m_mapChain.end() == iter ) {
    iter = m_mapChain.insert( m_mapChain.begin(), mapChain_t::value_type( dblStrike, OptionsAtStrike() ) );
  }
  assert( 0 == iter->second.sPut.size() );
  iter->second.sPut = sIQFeedSymbolName;
}

const std::string IvAtm::GetIQFeedNameCall( double dblStrike ) {
  mapChain_t::iterator iter = FindStrike( dblStrike );
  return iter->second.sCall;
}

const std::string IvAtm::GetIQFeedNamePut( double dblStrike ) {
  mapChain_t::iterator iter = FindStrike( dblStrike );
  return iter->second.sPut;
}
  
IvAtm::mapChain_t::const_iterator IvAtm::FindStrike( const double strike ) const {
  mapChain_t::const_iterator iter = m_mapChain.find( strike );
  if ( m_mapChain.end() == iter ) {
    throw std::runtime_error( "IvAtm::FindStrike const: can't find strike" );
  }
  return iter;
}

IvAtm::mapChain_t::iterator IvAtm::FindStrike( const double strike ) {
  mapChain_t::iterator iter = m_mapChain.find( strike );
  if ( m_mapChain.end() == iter ) {
    throw std::runtime_error( "IvAtm::FindStrike: can't find strike" );
  }
  return iter;
}

// lower_bound: key value eq or gt than query
// upper_bound: key value gt than query
IvAtm::tupleAdjacentStrikes_t IvAtm::FindAdjacentStrikes() const {
  
  tupleAdjacentStrikes_t strikes;
  auto& dblStrikeUpper( std::get<StrikeUpper>( strikes ) );
  auto& dblStrikeLower( std::get<StrikeLower>( strikes ) );
  
  dblStrikeUpper = 0.0;
  dblStrikeLower = 0.0;
  
  double dblUnderlying = CurrentUnderlying();

  mapChain_t::const_iterator iter = m_mapChain.lower_bound( dblUnderlying ); 
  if ( m_mapChain.end() == iter ) {
    throw std::runtime_error( "IvAtm::FindAdjacentStrikes: no upper strike available" );
  }
  dblStrikeUpper = iter->first;
  if ( dblUnderlying == dblStrikeUpper ) {
    dblStrikeLower = dblStrikeUpper;
  }
  else {
    if ( m_mapChain.begin() == iter ) {
      throw std::runtime_error( "IvAtm::FindAdjacentStrikes: already at lower lower end of strikes" );
    }
    --iter;
    dblStrikeLower = iter->first;
  }
  
  return strikes;
}

// called by UpdateATMWatch
void IvAtm::RecalcATMWatch( double dblUnderlying ) { // keeps underlying consistent with caller
  // uses a 25% edge hysterisis level to force recalc of three containing options 
  //   ie when underlying is within 25% of upper strike or within 25% of lower strike
  // uses a 50% hysterisis level to select new set of three containing options
  //   ie underlying has to be within +/- 50% of mid strike to choose midstrike and corresponding upper/lower strikes
  
  mapChain_t::iterator iterUpper;
  mapChain_t::iterator iterLower;
  
  auto& sUnderlying( m_pWatchUnderlying->GetInstrument()->GetInstrumentName() );
  
  iterUpper = m_mapChain.lower_bound( dblUnderlying ); 
  
  if ( m_mapChain.end() == iterUpper ) {
    std::cout << sUnderlying << ": IvAtm::RecalcATMWatch - no upper strike available" << std::endl; // stay in no watch state
    m_stateOptionWatch = EOWSNoWatch;
  }
  else {
    iterLower = iterUpper;
    if ( m_mapChain.begin() == iterLower ) {
      std::cout << sUnderlying << ": IvAtm::RecalcATMWatch - no lower strike available" << std::endl;  // stay in no watch state
      m_stateOptionWatch = EOWSNoWatch;
    }
    else {
      --iterLower;
      double dblMidPoint = ( iterUpper->first + iterLower->first ) * 0.5;
      if ( dblUnderlying >= dblMidPoint ) { // third strike is above
        m_iterUpper = iterUpper;
        ++m_iterUpper;
        if ( m_mapChain.end() == m_iterUpper ) {
          std::cout << sUnderlying << ": IvAtm::RecalcATMWatch - no upper upper strike available" << std::endl;  // stay in no watch state
          m_stateOptionWatch = EOWSNoWatch;
        }
        else {
          m_iterMid = iterUpper;
          m_iterLower = iterLower;
          m_stateOptionWatch = EOWSWatching;
        }
      }
      else { // third strike is below
        m_iterLower = iterLower;
        if ( m_mapChain.begin() == m_iterLower ) {
          std::cout << sUnderlying << ": IvAtm::RecalcATMWatch - no lower lower strike available" << std::endl;  // stay in no watch state
          m_stateOptionWatch = EOWSNoWatch;
        }
        else {
          --m_iterLower;
          m_iterMid = iterLower;
          m_iterUpper = iterUpper;
          m_stateOptionWatch = EOWSWatching;
        }
      }
      if ( EOWSWatching == m_stateOptionWatch ) {
        m_dblUpperTrigger = m_iterUpper->first - ( m_iterUpper->first - m_iterMid->first ) * 0.25;
        m_dblLowerTrigger = m_iterLower->first + ( m_iterMid->first - m_iterLower->first ) * 0.25;
        std::cout << m_dblLowerTrigger << " < " << dblUnderlying << " < " << m_dblUpperTrigger << std::endl;
      }
    }
  }
}

// 2018/09/12 care is needed with UpdateATMWatch/CalcAtmIv combo, as no values may be available on that transition
//   more complicated:  make transition when data is available
void IvAtm::UpdateATMWatch( double dblUnderlying ) {
  
  EOptionWatchState stateBefore( m_stateOptionWatch );
  EOptionWatchState stateAfter;
  
  switch ( stateBefore ) {
  case EOWSNoWatch:
    RecalcATMWatch( dblUnderlying );
    stateAfter = m_stateOptionWatch;
    switch ( stateAfter ) {
      case EOWSNoWatch:
        break;
      case EOWSWatching:
        m_iterUpper->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
        m_iterMid  ->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
        m_iterLower->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
        break;
    }
    break;
  case EOWSWatching:
    if ( ( dblUnderlying > m_dblUpperTrigger ) || ( dblUnderlying < m_dblLowerTrigger ) ) {
      mapChain_t::iterator iterUpper( m_iterUpper );
      mapChain_t::iterator iterMid( m_iterMid );
      mapChain_t::iterator iterLower( m_iterLower );
      RecalcATMWatch( dblUnderlying );
      stateAfter = m_stateOptionWatch;
      switch ( stateAfter ) {
        case EOWSNoWatch: 
          break;
        case EOWSWatching:
          m_iterUpper->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
          m_iterMid  ->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
          m_iterLower->second.Start( m_fStartCalc, m_pWatchUnderlying, m_fConstructOption );
          break;
      }
      iterUpper->second.Stop( m_fStopCalc, m_pWatchUnderlying );
      iterMid  ->second.Stop( m_fStopCalc, m_pWatchUnderlying );
      iterLower->second.Stop( m_fStopCalc, m_pWatchUnderlying );
    }
    break;
  }
}

void IvAtm::CalcIvAtm( ptime dtNow, fOnPriceIV_t& fOnPriceIV ) {
  
  double dblIvCall = 0.0;
  double dblIvPut = 0.0;
  
  double dblUnderlying = CurrentUnderlying();
  
  UpdateATMWatch( dblUnderlying );

  switch ( m_stateOptionWatch ) {
    case EOWSWatching:
      if ( dblUnderlying == m_iterMid->first ) {
        dblIvCall = m_iterMid->second.pCall->ImpliedVolatility();
        dblIvPut = m_iterMid->second.pPut->ImpliedVolatility();
      }
      else {
        if ( dblUnderlying > m_iterMid->first ) { // linear interpolation
          double ratio = ( dblUnderlying - m_iterMid->first ) / ( m_iterUpper->first - m_iterMid->first );

          double iv1, iv2;
          iv1 = m_iterMid->second.pCall->ImpliedVolatility();
          iv2 = m_iterUpper->second.pCall->ImpliedVolatility();
          dblIvCall = iv1 + ( iv2 - iv1 ) * ratio; 

          iv1 = m_iterMid->second.pPut->ImpliedVolatility();
          iv2 = m_iterUpper->second.pPut->ImpliedVolatility();
          dblIvPut = iv1 + ( iv2 - iv1 ) * ratio; 
        }
        else { // linear interpolation
          double ratio = ( dblUnderlying - m_iterLower->first ) / ( m_iterMid->first - m_iterLower->first );

          double iv1, iv2;
          iv1 = m_iterLower->second.pCall->ImpliedVolatility();
          iv2 = m_iterMid->second.pCall->ImpliedVolatility();
          dblIvCall = iv1 + ( iv2 - iv1 ) * ratio; 

          iv1 = m_iterLower->second.pPut->ImpliedVolatility();
          iv2 = m_iterMid->second.pPut->ImpliedVolatility();
          dblIvPut = iv1 + ( iv2 - iv1 ) * ratio; 
        }
      }
      PriceIV ivATM( dtNow, dblUnderlying, dblIvCall, dblIvPut);
      m_tsIvAtm.Append( ivATM );
      //m_bfIVUnderlyingCall.Add( now, dblIvCall, 0 );
      //m_bfIVUnderlyingPut.Add( now, dblIvPut, 0 );
      //OnIvAtmCalc( ivATM );
      
      if ( nullptr != fOnPriceIV ) fOnPriceIV( ivATM );

      //  std::cout << "AtmIV " << now << "" << m_dtExpiry << " " << dblUnderlying << "," << dblIvCall << "," << dblIvPut << std::endl;
      break;
  }

//return iv;
  
}

//std::ostream& operator<<( std::ostream& os, const ExpiryBundle& eb ) {
//  os << eb.m_dtExpiry << ": #strikes=" << eb.m_mapStrikes.size();
//  return os;
//}

//void Chain::EmitValues( void ) {
//  for ( mapChain_t::iterator iter = m_mapStrikes.begin(); m_mapStrikes.end() != iter; ++iter ) {
//    iter->second.EmitValues();
//  }
//}


void IvAtm::SaveIvAtm( const std::string& sPrefix, const std::string& sPrefix86400sec ) {

  std::string sPathName;

  ou::tf::HDF5DataManager dm( ou::tf::HDF5DataManager::RDWR );

  if ( 0 != m_tsIvAtm.Size() ) {

    std::stringstream ss;
    //ss.str( "" );
    // http://www.boost.org/doc/libs/1_54_0/doc/html/date_time/date_time_io.html
    //boost::posix_time::time_facet* pFacet( new boost::posix_time::time_facet( "%Y-%m-%d" ) );
    //boost::gregorian::date_facet* pFacet( new boost::gregorian::date_facet( "%Y-%m-%d" ) );
    //ss.imbue( std::locale( ss.getloc(), pFacet ) );
    //ss << m_dtExpiry.date();
  
    sPathName = sPrefix + "/atmiv/";
    HDF5WriteTimeSeries<ou::tf::PriceIVs> wtsAtmIv( dm, true, true, 5, 256 );
    wtsAtmIv.Write( sPathName, &m_tsIvAtm );
    HDF5Attributes attrAtmIv( dm, sPathName );
    attrAtmIv.SetSignature( ou::tf::PriceIV::Signature() );

//    {
//      sPathName = sPrefix86400sec + "/" + ss.str() + "/call";
//      ou::tf::Bars bars( 1 );
//      bars.Append( m_bfIVUnderlyingCall.getCurrentBar() );
//      HDF5WriteTimeSeries<ou::tf::Bars> wtsBar( dm, true, true, 5, 16 );
//      wtsBar.Write( sPathName, &bars );
//      HDF5Attributes attrBar( dm, sPathName );
//      try {
//        attrBar.SetSignature( ou::tf::Bar::Signature() );
//      }
//        catch (...) {  // may already exist
//      }
//    }
  
//    {
//      sPathName = sPrefix86400sec + "/" + ss.str() + "/put";
//      ou::tf::Bars bars( 1 );
//      bars.Append( m_bfIVUnderlyingPut.getCurrentBar() );
//      HDF5WriteTimeSeries<ou::tf::Bars> wtsBar( dm, true, true, 5, 16 );
//      wtsBar.Write( sPathName, &bars );
//      HDF5Attributes attrBar( dm, sPathName );
//      try {
//        attrBar.SetSignature( ou::tf::Bar::Signature() );
//      }
//      catch (...) {  // may already exist
//      }
//    }
  }
  
}

void IvAtm::SaveSeries( const std::string& sPrefix, const std::string& sPrefix86400sec ) {
  
  std::for_each( m_mapChain.begin(), m_mapChain.end(), [&sPrefix](mapChain_t::value_type& vt){
    vt.second.SaveSeries( sPrefix );
  } );
  
  SaveIvAtm( sPrefix, sPrefix86400sec );
}

} // namespace option
} // namespace tf
} // namespace ou


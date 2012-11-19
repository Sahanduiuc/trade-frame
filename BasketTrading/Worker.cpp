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
//

#include "StdAfx.h"

#include <boost/ref.hpp>

#include "Worker.h"

#include "SymbolSelection.h"

Worker::Worker(void) {
  m_pThread = new boost::thread( boost::ref( *this ) );
}

Worker::~Worker(void) {
  delete m_pThread; 
}

void Worker::operator()( void ) {

  SymbolSelection selector( ptime( date( 2012, 11, 16 ), time_duration( 0, 0, 0 ) ) );

}

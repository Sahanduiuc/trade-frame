/************************************************************************
 * Copyright(c) 2010, One Unified. All rights reserved.                 *
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

#pragma once

#include <string>
#include <stdexcept>

#include <LibSqlite/sqlite3.h>
#include <LibSqlite/DbSession.h>

class CTradingDb: public CDbSession
{
public:

  CTradingDb( const char* szDbFileName );
  ~CTradingDb(void);

protected:
private:
};

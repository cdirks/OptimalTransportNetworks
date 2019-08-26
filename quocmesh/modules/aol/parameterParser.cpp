/*
 * Copyright (c) 2001-2014 AG Rumpf, INS, Universitaet Bonn                      *
 *                                                                               *
 * The contents of this file are subject to the terms of the Common Development  *
 * and Distribution License Version 1.0 (the "License"); you may not use       *
 * this file except in compliance with the License. You may obtain a copy of     *
 * the License at http://www.opensource.org/licenses/CDDL-1.0                    *
 *                                                                               *
 * Software distributed under the License is distributed on an "AS IS" basis,  *
 * WITHOUT WARRANTY OF ANY KIND, either expressed or implied.                    *
 */

#include <vec.h>
#include <multiVector.h>
#include <qmException.h>
#include <parameterParser.h>

namespace {

// Helper function to implement aol::VariableField::write via recursion (hidden in a nameless namespace).
void writeRecursionHelper ( const aol::VariableField &VarField, int Depth, int &count, ostream &Off ) {
  Off << "{ ";
  for ( int j = 0; j < VarField.getDimSize(Depth); ++j ) {
    if ( Depth == VarField.getNumDim()-1 )
    {
      aol::Variable tempVar;
      VarField.getVariable ( count++, tempVar );
      tempVar.dump ( Off );
    }
    else
      writeRecursionHelper ( VarField, Depth+1, count, Off );
    Off << " ";
  }
  Off << "}";
}

}

namespace aol {

void VariableField::write ( ostream &Off ) const {
  Off << name << " ";
  Variable tempVar;
  if ( isSingleField() ) {
    getVariable ( tempVar );
    tempVar.dump ( Off );
  }
  else {
    int count = 0;
    writeRecursionHelper ( *this, 0, count, Off );
  }
  Off << endl;
}

ParameterParser::ParameterParser ( int argc, char **argv, const char *DefaultParFilename )
    : _echo ( false ),
    _outStream ( std::cout ) {
  char parameterfilename[1024];

  if ( argc > 2 ) {
    string errorMessage = "USAGE: ";
    errorMessage += argv[0];
    errorMessage += " <parameterfile>\n";
    throw Exception ( errorMessage.c_str(), __FILE__, __LINE__ );
  }
  if ( argc == 2 ) {
    strncpy ( parameterfilename, argv[1],  1023 );
  }
  if ( argc == 1 ) {
    strncpy ( parameterfilename, DefaultParFilename,  1023 );
  }
  parameterfilename[1023] = 0;
  cerr << "Reading parameters from " << parameterfilename << endl;
  initialize ( parameterfilename );
}

void ParameterParser::initialize ( const std::string & ParFilename ) {
  in.open ( ParFilename.c_str() );
  if ( in.fail() ) {
    string errorMessage = "ParameterParser: Can't open file \"";
    errorMessage += ParFilename;
    errorMessage += "\".";
    throw Exception ( errorMessage.c_str(), __FILE__, __LINE__  );
  }
  parse();
  in.close();
  _parameterFileName = ParFilename;
}

ParameterParser::ParameterParser ( const ParameterParser & other )
: varFields          ( other.varFields.size() ),
  _echo              ( other._echo ),
  _outStream          ( other._outStream ),
  _parameterFileName ( other._parameterFileName ) {
  for ( unsigned int i = 0; i < other.varFields.size(); ++i )
    varFields[i] = new VariableField ( *(other.varFields[i]) );
}

void ParameterParser::readField ( VariableField &VarField ) {
  char varStr[ 80 ];
  // vl denotes how many chars we already wrote to varStr.
  int vl = 0;
  int cd = 0;
  int maxd = -1;
  int i, p;

  int dimSizes[ 16 ], prevDimSizes[ 16 ];
  for ( i = 0; i < 16; i++ ) {
    dimSizes[ i ] = 0;
    prevDimSizes[ i ] = -1;
  }

  ignoreWS();

  do {
    p = in.get();
    if ( ws ( p ) || p == '}' ) {
      // End of the current entry found, terminate varStr.
      varStr[ vl ] = 0;
      // We wrote something to varStr, give the string to VarField and increase dimSizes[ cd ].
      if ( vl > 0 ) {
        VarField.append ( varStr );
        dimSizes[ cd ]++;
      }
      vl = 0;
      if ( p == '}' ) {
        if ( maxd < 0 ) {
          maxd = cd;
        }
        // After closing a bracket, check if the bracket has the same size as the previous in this depth.
        if ( dimSizes[ cd ] != 0 ) {
          if ( prevDimSizes[ cd ] == -1 )
            prevDimSizes[ cd ] = dimSizes[ cd ];
          else if ( dimSizes[ cd ] != prevDimSizes[ cd ] ) {
            string errorMessage = strprintf ( "ERROR sizes in depth %d not constant! prevDim = %d, dim = %d\n", cd, prevDimSizes[ cd ], dimSizes[ cd ] );
            throw Exception ( errorMessage, __FILE__, __LINE__ );
          }
        }
        cd--;
        dimSizes[ cd ]++;
      }
      ignoreWS();
    } else if ( p == '{' ) {
      if ( maxd > 0 && cd == maxd ) {
        throw Exception ( "ERROR in parsing: exceeding maximum depth\n", __FILE__, __LINE__ );
      }
      cd++;
      dimSizes[ cd ] = 0;
      ignoreWS();
    } else {
      varStr[ vl++ ] = p;
    }
  } while ( cd > 0 && !in.eof() );

  if ( cd != 0 ) {
    cerr << "ERROR in parsing file: missing closing \'}\'\n";
  }

  /*cerr << "maxd = " << maxd << endl;
  for ( i = 0; i <= maxd; i++ ) {
    cerr << "dimSizes[ " << i << " ]= " << dimSizes[ i ] << endl;
  }*/

  VarField.setNumDim ( maxd );
  for ( i = 0; i < maxd; i++ ) {
    VarField.setDimSize ( i, dimSizes[ i + 1 ] );
  }
}

void ParameterParser::parse() {
  char name[ 80 ];
  char Var[ 256 ];

  while ( !in.eof() ) {

    ignoreWS();

    while ( in.peek() == '#' || ( !in.eof() && ws ( in.peek() ) ) || in.peek() == '\n' ) {
      if ( ws ( in.peek() ) )
        ignoreWS();
      else
        ignoreLine();
    }

    if ( !in.eof() ) {
      in >> name;

      // Don't allow any variable to be defined more than once.
      if ( hasVariable ( name ) )
        throw Exception ( aol::strprintf ( "Variable \"%s\" already defined", name ), __FILE__, __LINE__  );

      VariableField *varField = new VariableField ( name );

      ignoreWS();

      if ( in.eof() || ( in.peek() == '\n' ) )
        throw Exception ( aol::strprintf ( "Unexpected end of line while parsing variable %s", name ), __FILE__, __LINE__  );
      else if ( in.peek() == '{' )
        readField ( *varField );
      else {
        varField->setNumDim ( 0 );
        // If enclosed by quotation marks, allow the variable value to contain spaces.
        if ( in.peek() == '"' ) {
          in.ignore();
          string fullVar;
          while ( !in.eof() && ( in.peek() != '"' ) && ( in.peek() != '\n' ) ) {
            fullVar += in.get();
          }
          if ( in.peek() == '"' )
            in.ignore();
          else
            throw Exception ( aol::strprintf ( "Unexpected end of line while parsing quotation mark enclosed variable %s", name ), __FILE__, __LINE__  );
          varField->append ( fullVar.c_str() );
        }
        else {
          in >> Var;
          varField->append ( Var );
        }
      }
      varFields.push_back ( varField );
      //varField->dump( );

      // Ignore any trailing white space and make sure that nothing but white space is left in this line.
      ignoreWS();
      if ( !in.eof() && ( in.peek() != '\n' ) )
        throw Exception ( aol::strprintf ( "Error while parsing variable \"%s\": Unexpected data after the variable value found", name ), __FILE__, __LINE__  );
    }
  }
}

VariableField* ParameterParser::findFirstVariableField ( const char *VarName ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) ) {
      return (*it);
    }
  }
  string errorMessage = strprintf ( "No match found for %s.\n", VarName );
  throw Exception ( errorMessage, __FILE__, __LINE__ );
  return NULL;
}

int ParameterParser::getDimSize ( const char *VarName, int I /*= 0*/ ) const {
  VariableField* var = findFirstVariableField ( VarName );
  return var->getDimSize ( I );
}

int ParameterParser::getNumDim ( const char *VarName ) const {
  VariableField* var = findFirstVariableField ( VarName );
  return var->getNumDim ();
}

double ParameterParser::getDouble ( const char *VarName ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->isSingleField() ) {
      Variable Var;
      ( *it )->getVariable ( Var );
      if ( Var.type() == Variable::VAR_DOUBLE || Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getDouble() << std::endl;
        return Var.getDouble();
      }
    }
  }

  string err;
  err = "No match found for double ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0.0;
}

double ParameterParser::getDouble ( const char *VarName, int I ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->getNumDim() == 1 ) {
      Variable Var;
      ( *it )->getVariable ( I, Var );
      if ( Var.type() == Variable::VAR_DOUBLE || Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getDouble() << std::endl;
        return Var.getDouble();
      }
    }
  }
  string err;
  err = "No match found for double ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0.0;
}

double ParameterParser::getDouble ( const char *VarName, int I1, int I2 ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->getNumDim() == 2 ) {
      Variable Var;
      ( *it )->getVariable ( I1, I2, Var );
      if ( Var.type() == Variable::VAR_DOUBLE || Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getDouble() << std::endl;
        return Var.getDouble();
      }
    }
  }
  string err;
  err = "No match found for double ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0.0;
}


double ParameterParser::getDoubleOrDefault ( const char *VarName, double Default ) const {
  return hasVariable ( VarName ) ? getDouble ( VarName ) : Default;
}

int ParameterParser::getInt ( const char *VarName ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->isSingleField() ) {
      Variable Var;
      ( *it )->getVariable ( Var );
      if ( Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getInt() << std::endl;
        return Var.getInt();
      }
    }
  }
  string err;
  err = "No match found for integer ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0;
}

int ParameterParser::getInt ( const char *VarName, int I ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->getNumDim() == 1 ) {
      Variable Var;
      ( *it )->getVariable ( I, Var );
      if ( Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getInt() << std::endl;
        return Var.getInt();
      }
    }
  }
  string err;
  err = "No match found for integer ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0;
}

int ParameterParser::getInt ( const char *VarName, int I1, int I2 ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->getNumDim() == 2 ) {
      Variable Var;
      ( *it )->getVariable ( I1, I2, Var );
      if ( Var.type() == Variable::VAR_INT ) {
        if ( _echo ) _outStream << VarName << " = " << Var.getInt() << std::endl;
        return Var.getInt();
      }
    }
  }
  string err;
  err = "No match found for integer ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
  return 0;
}

int ParameterParser::getIntOrDefault ( const char *VarName, int Default ) const {
  return hasVariable ( VarName ) ? getInt ( VarName ) : Default;
}

void ParameterParser::getString ( const char *VarName, char *DestStr ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->isSingleField() ) {
      Variable Var;
      ( *it )->getVariable ( Var );
      strcpy ( DestStr, Var.getVarStr() );
      if ( _echo ) _outStream << VarName << " = " << DestStr << std::endl;
      return;
    }
  }
  string err;
  err = "No match found for ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
}

void ParameterParser::getString ( const char *VarName, char *DestStr, int I ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    if ( ( strcmp ( ( *it )->getName(), VarName ) == 0 ) && ( *it )->getNumDim() == 1 ) {
      Variable Var;
      ( *it )->getVariable ( I, Var );
      strcpy ( DestStr, Var.getVarStr() );
      if ( _echo ) _outStream << VarName << " = " << DestStr << std::endl;
      return;
    }
  }
  string err;
  err = "No match found for ";
  err += VarName;
  throw Exception ( err.c_str(), __FILE__, __LINE__ );
}

string ParameterParser::getString ( const char *VarName ) const {
  char tempCString[1024];
  getString ( VarName, tempCString );
  return tempCString;
}

string ParameterParser::getString ( const char *VarName, const int I ) const {
  char tempCString[1024];
  getString ( VarName, tempCString, I );
  return tempCString;
}

string ParameterParser::getStringOrDefault ( const char *VarName, string Default ) const {
  return hasVariable ( VarName ) ? getString ( VarName ) : Default;
}

bool ParameterParser::hasVariable ( const char *VarName ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it )
    if ( strcmp ( ( *it )->getName(), VarName ) == 0 )
      return true;

  // variable name not found:
  return false;
}

bool ParameterParser::checkVariable ( const char * VarName ) const {
  if ( hasVariable ( VarName ) )
    return true;

  cerr << "Parameter file \"" << _parameterFileName << "\" is supposed "
          "to contain field \"" << VarName << "\"." << endl;
  return false;
}

bool ParameterParser::checkAndGetBool ( const char *VarName ) const {
  return ( hasVariable ( VarName ) && ( getInt ( VarName ) == 1 ) );
}

void ParameterParser::dump ( ostream &Off ) const {
  vector<VariableField*>::const_iterator it;
  for ( it = varFields.begin(); it != varFields.end(); ++it ) {
    ( *it )->write ( Off );
  }
}

void ParameterParser::dumpToFile ( const char *FileName, const char *Directory ) const {
  string outFileName;
  if ( Directory )
    outFileName += Directory;
  outFileName += FileName;
  ofstream out ( outFileName.c_str() );
  dump ( out );
  out.close();
}

void ParameterParser::getIntVec ( const char *VarName, aol::Vector<int> &vec ) const {
  const int size = this->getDimSize ( VarName );
  vec.resize ( size ); vec.setZero();
  for ( int i = 0; i < size; ++i ) {
    vec[i] = this->getInt ( VarName, i );
  }
}

void addCounterToSaveDirectory ( ParameterParser& parser, const string counterFileName ) {
  if ( aol::fileExists ( counterFileName ) == false ) {
    std::ofstream out ( counterFileName.c_str() );
    out << 0 << endl;
    out.close ( );
  }
  std::fstream counterFile;
  counterFile.open ( counterFileName.c_str () );
  if ( counterFile.is_open () ) {
    string temp;
    std::getline ( counterFile, temp );
    int counter = atoi ( temp.c_str () );
    counterFile.seekg ( ios::beg );
    counterFile << ++counter;
    parser.changeVariableValue ( "saveDirectory", aol::strprintf ( "%s-%d", parser.getString ( "saveDirectory" ).c_str (), counter ).c_str () );
  }
  else
    throw aol::Exception ( "Cannot open parameter file for writing!", __FILE__, __LINE__ );
  counterFile.close ();
}

} // end namespace

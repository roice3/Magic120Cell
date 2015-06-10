#include <stdafx.h>
#include "loader.h"

#include "vectorND.h"

using namespace System::Collections;
using System::Char;
using System::String;
using System::IO::File;

#define VERSION_STRING	"Version1"


namespace
{

void 
saveTwists( System::IO::StreamWriter ^ sw, const std::vector<STwist> & twists ) 
{
	String ^ line = "";
	int count = 0;
	for( uint t=0; t<twists.size(); t++ )
	{
		String ^ twistString = System::Convert::ToString( twists[t].getTwistHash() );
		line += twistString;
		count++;

		if( count >= 10 || t == twists.size()-1 )
		{
			sw->WriteLine( line );
			line = "";
			count = 0;
		}
		else
		{
			line += "\t";
		}
	}
}

bool 
loadTwists( System::IO::StreamReader ^ sr, std::vector<STwist> & twists, int numLines ) 
{
	twists.clear();

	// Delimiters (just tabs).
	array<Char> ^ delimiter = { '\t' };

	String ^ line = "";
	int linesRead = 0;
    while( sr->Peek() >= 0 ) 
    {
		// Were there a specific number of lines specified to read?
		if( -1 != numLines )
			if( linesRead == numLines )
				return true;

		line = sr->ReadLine();
		linesRead++;
		array<String ^> ^ split = line->Split( delimiter );
		IEnumerator ^ myEnum = split->GetEnumerator();

		while( myEnum->MoveNext() )
		{
			String ^ tempString = safe_cast<String ^>( myEnum->Current );
			STwist twist;
			int twistHash = System::Convert::ToInt32( tempString );
			twist.decodeTwistHash( twistHash );
			twists.push_back( twist );
		}
    }

	return true;
}

bool 
loadFromFile( String ^ fileName, int & puzzle, CState & state, std::vector<STwist> & twists ) 
{
	System::IO::StreamReader ^ sr = gcnew System::IO::StreamReader( fileName );

	// Delimiters (just tabs).
	array<Char> ^ delimiter = { '\t' };

	/* Read the header.
	   Fail if unexpected format.
	   Example is:
	   Magic120Cell	Version1	4	1	1000
	   last 3 columns are: puzzle type, whether we've scrambled, and the number of twists.
	*/
	String ^ line = sr->ReadLine();
	array<String ^> ^ split = line->Split( delimiter );
	if( 5 != split->Length )
		return false;

	if( !split[0]->Equals( "Magic120Cell" ) )
		return false;
	if( !split[1]->Equals( VERSION_STRING ) )
		return false;

	puzzle = System::Convert::ToInt32( split[2] );

	// NOTE: State needs to be reset before setting scrambled flag.
	state.reset();
	state.setScrambled( 1 == System::Convert::ToInt32( split[3] ) );

	int numTwists = System::Convert::ToInt32( split[4] );

	// Now read the state.
	for( int c=0; c<state.nCells(); c++ )
	{
		line = sr->ReadLine();
		assert( 2*state.nStickers() == line->Length );
		for( int s=0; s<state.nStickers(); s++ )
			state.setStickerColorIndex( c, s, System::Convert::ToInt32( line->Substring( 2*s, 2 ), 16 ) );
		state.commitChanges( c );
	}

	// Now read all the twists.
	loadTwists( sr, twists, -1 );

	// Close the file.
	sr->Close();

	return true;
}

void 
saveToFile( String ^ fileName, int puzzle, const CState & state, const std::vector<STwist> & twists, bool saveas ) 
{
	System::IO::StreamWriter ^ sw = File::CreateText( fileName );

	// Write the header.
	String ^ header = "Magic120Cell\t" + VERSION_STRING + "\t" +
		System::Convert::ToString( puzzle ) + "\t" +
		System::Convert::ToString( state.getScrambled() ? 1 : 0 ) + "\t" +
		System::Convert::ToString( twists.size() );
	sw->WriteLine( header );

	// Now write out the state.
	String ^ line;
	for( int c=0; c<state.nCells(); c++ )
	{
		line = "";
		for( int s=0; s<state.nStickers(); s++ )
		{
			int colorInt = state.getStickerColorIndex( c, s );
			if( colorInt < 16 )
				line += "0";
			line += System::Convert::ToString( colorInt, 16 );
		}
		sw->WriteLine( line );
	}

	// Now write out all the twists.
	saveTwists( sw, twists );

	// Close the file.
	sw->Close();
}

}

CLoader::CLoader()
{
}

void 
CLoader::saveToFile( int puzzle, const CState & state, const std::vector<STwist> & twists, bool saveas ) 
{
	// Get the filename to save to.
	String ^ fileName = getSaveFileName( saveas );
	if( "" == fileName )
		return;

	::saveToFile( fileName, puzzle, state, twists, saveas );
}

void 
CLoader::saveScrambledFile( const CState & state, const std::vector<STwist> & twists )
{
	::saveToFile( "full_scramble.log", 4, state, twists, false );
}

bool 
CLoader::loadFromFile( int & puzzle, CState & state, std::vector<STwist> & twists ) 
{
	String ^ fileName = getLoadFileName();
	if( ! File::Exists( fileName ) )
		return false;

	return ::loadFromFile( fileName, puzzle, state, twists );
}

bool 
CLoader::loadScrambledFile( CState & state, std::vector<STwist> & twists )
{
	String ^ fileName = "full_scramble.log";
	if( ! File::Exists( fileName ) )
		return false;

	int dummy;
	return ::loadFromFile( fileName, dummy, state, twists );
}

namespace
{
	std::string systemStringToStdString( System::String ^ input )
	{
		// XXX - There has got to be a better way to do this!
		std::string returnString;
		for( int i=0; i<input->Length; i++ )
			returnString.push_back( (char)input[i] );
		return returnString;
	}
}

#define FILTER "MagicCube5D log files (*.log)|*.log|All files (*.*)|*.*"

System::String ^ 
CLoader::getSaveFileName( bool forcePrompt ) 
{
	if( 0 == m_filename.length() || forcePrompt )
	{
		System::Windows::Forms::SaveFileDialog ^ dlg = gcnew System::Windows::Forms::SaveFileDialog();
		dlg->AddExtension = true;
		dlg->OverwritePrompt = true;
		dlg->Filter = FILTER;
		if( System::Windows::Forms::DialogResult::OK != dlg->ShowDialog() )
			return "";
		m_filename = systemStringToStdString( dlg->FileName );
	}

	return gcnew String( m_filename.c_str() );
}

System::String ^ 
CLoader::getLoadFileName() 
{
	System::Windows::Forms::OpenFileDialog ^ dlg = gcnew System::Windows::Forms::OpenFileDialog();
	dlg->CheckFileExists = true;
	dlg->Filter = FILTER;
	if( System::Windows::Forms::DialogResult::OK != dlg->ShowDialog() )
		return "";
	m_filename = systemStringToStdString( dlg->FileName );
	return dlg->FileName;
}

#undef FILTER
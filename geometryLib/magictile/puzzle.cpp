#include <stdafx.h>
#include "puzzle.h"

#include <fstream>
#include <sstream>

namespace magictile
{

#define DEBUG_OUT( s )							\
{												\
   std::ostringstream os_;						\
   os_ << s;									\
   os_ << "\n";									\
   OutputDebugStringA( os_.str().c_str() );		\
}

namespace
{
	void doInitialEvenSlicing( bool oddDigon, 
		const CPolygon & tile, std::vector<CPolygon> & slicees )
	{
		bool evenSidesPerFace = even( tile.numSides() );
		for( uint i=0; i<tile.numSides(); i++ )
		{
			uint index1 = i;
			uint index2 = i+1;
			if( index2 == tile.numSides() )
				index2 = 0;

			const CSegment & s1 = tile.m_segments[index1];
			const CSegment & s2 = tile.m_segments[index2];

			CSegment new1 = s1;
			CSegment new2 = s2;
			CSegment new3, new4, new5;
			new1.m_p1 = new4.m_p2 = s1.midpoint();
			new2.m_p2 = new3.m_p1 = s2.midpoint();
			new5.m_p1 = s1.m_p2;

			// Special coloring for these slices.
			float t = .5;
			CColor c = oddDigon ? CColor( 0,0,0,0 ) : CColor( 0,0,t,1 );
			new3.m_color = new4.m_color = new5.m_color = c;

			if( evenSidesPerFace )
			{
				CPolygon newPoly;
				newPoly.m_segments.push_back( new1 );
				newPoly.m_segments.push_back( new2 );
				newPoly.m_segments.push_back( new3 );
				newPoly.m_segments.push_back( new4 );
				slicees.push_back( newPoly );
			}
			else
			{
				CPolygon newPoly1;
				newPoly1.m_segments.push_back( new1 );
				newPoly1.m_segments.push_back( new5 );
				newPoly1.m_segments.push_back( new4 );
				slicees.push_back( newPoly1 );

				CPolygon newPoly2;
				new5.reverse();
				newPoly2.m_segments.push_back( new2 );
				newPoly2.m_segments.push_back( new3 );
				newPoly2.m_segments.push_back( new5 );
				slicees.push_back( newPoly2 );
			}
		}
	}

	// Slices up a set of polygons using a set of circles.
	bool sliceRecursive( std::vector<CPolygon> & slicees, 
		std::vector<CCircle> & slicers, std::vector<CPolygon> & sliced )
	{
		// We're done if we've used up all the slicing circles.
		if( !slicers.size() )
		{
			sliced = slicees;
			return true;
		}

		// Use the next circle to slice it all up.
		CCircle slicer = slicers[slicers.size()-1];
		std::vector<CPolygon> tempSliced;
		for( uint i=0; i<slicees.size(); i++ )
		{
			CPolygon & slicee = slicees[i];
			if( !slicee.slice( slicer, tempSliced ) )
				return false;
		}

		// On to the next level...
		slicers.pop_back();
		return sliceRecursive( tempSliced, slicers, sliced );
	}	
}

CPuzzle::CPuzzle() : m_state( 0, 0 )
{
	m_rotating = false;
	m_rotation = 0;
	m_building = false;
}

bool 
CPuzzle::evenLength() const
{
	return even( m_settings.n );
}

void
CPuzzle::calcTwistDataArray()
{
	m_currentTwistDataArray.clear();

	int twistCell = m_currentTwist.m_cell;
	CTwistData twistData = getTwistData( twistCell, true );
	m_currentTwistDataArray.add( twistData );

	// Any slave cells?
	if( 0 != m_slaves.size() )
	{
		std::map<int,std::vector<int>>::const_iterator it;
		it = m_masterToSlave.find( twistCell );
		if( it != m_masterToSlave.end() )
		{
			const std::vector<int> & temp = it->second;
			for( uint i=0; i<temp.size(); i++ )
			{
				m_currentTwistDataArray.add(
					getTwistData( temp[i], false ) );
			}
		}
	}
}

CTwistData 
CPuzzle::getTwistData( int twistCell, bool masterCell ) const
{
	if( -1 == twistCell )
		return CTwistData();

	CTwistData twistData;
	const CCell & c = masterCell ? m_cells[twistCell] : m_slaves[twistCell];
	twistData.twisters = c.m_twisters;

	// NOTE: We can ignore masterCell input here,
	//		 since this only happens in Spherical case.
	if( -1 != c.m_opp )
		twistData.oppTwisters = m_cells[c.m_opp].m_twisters;

	twistData.fixedPlus = c.m_tile.m_center;
	twistData.slicemask = m_currentTwist.m_slicemask;
	twistData.preCalcItems( c.m_vertexCircle );

	// Even-length cube puzzles need their slice count reduced.
	twistData.reduceSliceCount = evenLength() && m_settings.p == 4 && !m_settings.hemi;	

	// Handle reversed twisting on hemi-puzzles.
	if( m_settings.hemi && !masterCell )
		twistData.m_twistingReversed = !twistData.m_twistingReversed;

	// If our cell center is at infinity (spherical geometries),
	// we also want to reverse the twisting, because the projection has inverted the cell.
	if( isInfinite( c.m_tile.m_center ) )
		twistData.m_twistingReversed = !twistData.m_twistingReversed;

	return twistData;
}

double 
CPuzzle::getCurrentRotation( bool rotating ) const
{
	double rotation = rotating ?
		getSmoothedRotation( m_rotation, getCurrentTwistMagnitude() ) :
		getCurrentTwistMagnitude();
	if( !m_currentTwist.m_leftClick )
		rotation *= -1;
	return rotation;
}

void 
CPuzzle::invalidateAllCells() 
{
	for( uint i=0; i<m_cells.size(); i++ )
		m_cells[i].invalidate();

	for( uint i=0; i<m_slaves.size(); i++ )
		m_slaves[i].invalidate();
}

void 
CPuzzle::render( const CVector3D & /*lookFrom*/, bool forPicking ) 
{
	if( m_building )
		return;

	// NOTES about this call.
	// - I added this when I upgraded to VS 2008.  On my old laptop, without this call, I got
	//	 crazy memory usage (>1GB on KQ puzzle) when display lists were getting generated.
	// - The interwebs discourage glFinish calls, but it looks to be behaving pretty harmlessly here.
	// - glFlush was not enough.
	// - Turned out to be required for both picking and normal rendering.
	glFinish();

	/* 
	// I used this block for debugging, to see what tiling configs "converged".
	static bool configDone = false;
	if( !configDone )
	{
		runThroughPuzzleTilingConfigs();
		configDone = true;
	}
	*/

	if( forPicking )
		renderForPicking();
	else
		render();
}

void 
CPuzzle::render() 
{
	// We're just going to keep things looking nice
	// by drawing in order for now.
	glDisable( GL_DEPTH_TEST );

	if( m_settings.trippyMode )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );	// Very nice for debugging.
	else
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glLineWidth( (GLfloat)m_settings.lineThickness*10 );
	bool drawEdges = m_settings.lineThickness != 0;

	Geometry g = m_settings.geometry();
	double rotation = getCurrentRotation( m_rotating );

	for( uint i=0; i<m_cells.size(); i++ )
		m_cells[i].render( g, drawEdges, m_rotating, rotation, m_currentTwistDataArray, m_state, i );

	if( m_settings.showOnlyFundamental )
		return;

	const bool renderAll = true;
	if( renderAll )
	{
		for( uint i=0; i<m_slaves.size(); i++ )
		{
			int masterIndex = m_slaveToMaster[i];
			m_slaves[i].render( g, drawEdges, m_rotating, rotation, m_currentTwistDataArray, m_state, masterIndex );
		}
	}
	else
	{
		// Draws only slave cells involved in state calcs (for debugging).
		for( uint t=0; t<m_stateSlaves.size(); t++ )
		{
			int i = m_stateSlaves[t];
			int masterIndex = m_slaveToMaster[i];
			m_slaves[i].render( g, drawEdges, m_rotating, rotation, m_currentTwistDataArray, m_state, masterIndex );
		}
	}
}

void 
CPuzzle::renderForPicking() 
{
	if( m_dlPicking.call() )
		return;

	m_dlPicking.start();

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	Geometry g = m_settings.geometry();

	glEnable( GL_DEPTH_TEST );
	for( uint i=0; i<m_cells.size(); i++ )
	{
		// XXX - Only handles a limited number of cells, which could become a problem.
		// XXX - We could use the clicked sticker to control slice twisting.
		glColor3b( (GLbyte)i, 0, 0 );
		m_cells[i].renderForPicking( g );
	}

	for( uint i=0; i<m_slaves.size(); i++ )
	{
		int masterIndex = m_slaveToMaster[i];
		glColor3b( (GLbyte)masterIndex, 0, 0 );
		m_slaves[i].renderForPicking( g );
	}

	m_dlPicking.end();
}

void 
CPuzzle::setColor( uint i, CColor c ) 
{
	if( i >= m_cells.size() )
		return;
	m_state.setCellColor( i, c );
	invalidateAllCells();
}

void 
CPuzzle::startRotate( const STwist & twist ) 
{
	m_currentTwist = twist;
	calcTwistDataArray();
	m_rotation = 0;
	
	// Prepare for twisting by invalidating all the cells that will be affected.
	Geometry g = m_settings.geometry();
	for( uint i=0; i<m_cells.size(); i++ )
	{
		if( g == Spherical )
		{
			m_cells[i].preCalcTwistingSpherical( m_currentTwistDataArray );

			if( m_settings.hemi )
				m_slaves[i].preCalcTwistingSpherical( m_currentTwistDataArray );	

			continue;
		}

		bool invalidated = m_cells[i].preCalcTwisting( g, m_currentTwistDataArray, true );
		if( !invalidated )
			continue;

		// Invalidate all the slaves of this master cell.
		std::map<int,std::vector<int>>::iterator it;
		it = m_masterToSlave.find( i );
		if( it != m_masterToSlave.end() )
		{
			std::vector<int> & slaves = it->second;
			std::vector<int> dummy;
			for( uint j=0; j<slaves.size(); j++ )
			{
				m_slaves[slaves[j]].preCalcTwisting( g, m_currentTwistDataArray, false );

				// This is because we may still need to 
				// invalidate some stickers (on the edge of the puzzle).
				m_slaves[slaves[j]].invalidate();
			}
		}
	}

	m_rotating = true;
}

void 
CPuzzle::iterateRotate() 
{
	if( !m_rotating )
		return;

	m_rotation += deg2rad( getCurrentTwistStep() );
	double twistMag = getCurrentTwistMagnitude();
	if( m_rotation > twistMag )
		m_rotation = twistMag;
}

namespace
{
	CVector3D infinitySafe( const CVector3D & in )
	{
		if( isInfinite( in ) )
			return CVector3D( 666,666,666 );

		return in;
	}
}

void 
CPuzzle::finishRotate() 
{
	m_rotating = false;
	m_rotation = 0;
	double rotation = getCurrentRotation( false );

	//
	// Calculate the new state.
	// XXX - possible to move any of this into the state class itself?
	//
	
	// Maps from old sticker position to sticker hash.
	Vec3ToIntMap oldMap;

	// Maps from sticker hash to new sticker position.
	std::map<int,CVector3D> newMap;

	// Fill out our maps.
	Geometry g = m_settings.geometry();
	uint count = m_cells.size() + m_stateSlaves.size();
	for( uint c=0; c<count; c++ )
	{
		bool isSlave = c >= m_cells.size();
		int slaveIndex = isSlave ? m_stateSlaves[c-m_cells.size()] : -1;
		const CCell & cell = isSlave ? 
			m_slaves[slaveIndex] :
			m_cells[c];

		for( uint s=0; s<cell.m_stickers.size(); s++ )
		{
			const CPolygon & sticker = cell.m_stickers[s];
			int twist = m_currentTwistDataArray.twistSticker( g, sticker );
			if( -1 != twist )
			{
				const CTwistData & twistData = m_currentTwistDataArray.get( twist );

				int hash = getStickerHashEx( c, s );
				oldMap[infinitySafe( sticker.m_center )] = hash;

				CVector3D fixedPlus = twistData.fixedPlus;
				CVector3D fixedNeg = twistData.fixedNeg;

				// NOTE: We only need to twist the center here!
				CPolygon copy = sticker;
				copy.twistCenter( twistData.m_twistingReversed ? -rotation : rotation, fixedPlus, fixedNeg );

				newMap[hash] = infinitySafe( copy.m_center );
			}
		}
	}

	std::map<int,CVector3D>::iterator it1;
	for( it1 = newMap.begin(); it1 != newMap.end(); it1++ )
	{
		Vec3ToIntMap::iterator it2;
		it2 = oldMap.find( it1->second );
		if( it2 == oldMap.end() )
		{
			// XXX - This currenty happens with moves on the boundary of the puzzle for p>=6,
			// and for points at infinity.
			//assert( false );
			continue;
		}

		// Sticker has moved from hash1 -> hash2.
		int hash1 = it1->first;
		int hash2 = it2->second;
		int c1, c2, s1, s2;
		decodeStickerHashEx( hash1, c1, s1 );
		decodeStickerHashEx( hash2, c2, s2 );

		bool c1Slave = c1 >= (int)m_cells.size();
		bool c2Slave = c2 >= (int)m_cells.size();

		// We only need to update the masters.
		if( c2Slave )
			continue;

		int c1MasterIndex = c1Slave ? m_slaveToMaster[m_stateSlaves[c1-m_cells.size()]] : c1;
		int c2MasterIndex = c2Slave ? m_slaveToMaster[m_stateSlaves[c2-m_cells.size()]] : c2;

		m_state.setStickerColorIndex( c2MasterIndex, s2,
			m_state.getStickerColorIndex( c1MasterIndex, s1 ) );
	}
	m_state.commitChanges();

	m_twistHistory.update( m_currentTwist );

	m_currentTwistDataArray.clear();
	m_currentTwist = STwist();
}

const STwist & 
CPuzzle::getCurrentTwist() const
{
	return m_currentTwist;
}

double 
CPuzzle::getCurrentTwistStep() const
{
	return m_settings.rotationStep;
}

double 
CPuzzle::getCurrentTwistMagnitude() const
{
	// This only depends on our tiling right now,
	// since we only have cell-centered twists.
	// If we add vertex/edge/general centered twists,
	// this will change.
	return 2 * M_PI / m_settings.p;
}

void 
CPuzzle::scramble( int numTwists ) 
{
	srand( GetCurrentTime() );

	for( int i=0; i<numTwists; i++ )
	{
		m_currentTwist = STwist();
		m_currentTwist.m_leftClick = getRandomInt( 1 ) == 1;
		m_currentTwist.m_cell = getRandomInt( m_cells.size()-1 );
		int randomSlice = getRandomInt( m_settings.numTwistableSlices()-1 ) + 1;
		m_currentTwist.m_slicemask = sliceToMask( randomSlice );

		// Apply the twist.
		calcTwistDataArray();
		finishRotate();
	}
	
	invalidateAllCells();
}

void 
CPuzzle::reset() 
{
	m_state.reset();
	m_twistHistory.clear();
	invalidateAllCells();
}

void 
CPuzzle::markUnscrambled() 
{
}

void 
CPuzzle::resetView() 
{
}

bool 
CPuzzle::isSolved() const
{
	return m_state.isSolved();
}

namespace
{
	bool comparePointsCheckInfinity( const CVector3D & p1, const CVector3D & p2 )
	{
		// XXX - need a more standard way to check for infinity vectors!
		//		 I'm not consistent with this!
		if( !p1.valid() && !p2.valid() )
			return true;

		return p1.compare( p2 );
	}

	bool find( const CVector3D & center, const Vec3ToBoolMap & map )
	{
		Vec3ToBoolMap::const_iterator it;
		it = map.find( center );
		if( it == map.end() )
			return false;

		return true;
	}

	bool find( const CVector3D & center, int & val, const Vec3ToIntMap & map )
	{
		Vec3ToIntMap::const_iterator it;
		it = map.find( center );
		if( it == map.end() )
			return false;

		val = it->second;
		return true;
	}

	// Returns whether the item was already in the map.
	bool findOrAdd( const CVector3D & center, Vec3ToBoolMap & map )
	{
		if( !find( center, map ) )
		{
			map[center] = true;
			return false;
		}

		return true;
	}

	bool findOrAdd( const CVector3D & center, int newVal, Vec3ToIntMap & map )
	{
		int dummy;
		if( !find( center, dummy, map ) )
		{
			map[center] = newVal;
			return false;
		}

		return true;
	}
}

void 
CPuzzle::runThroughPuzzleTilingConfigs() 
{
	std::ofstream file( "puzzles.txt" );

	for( int p=7; p<16; p++ )
	{
		file << "\n----------------------------------- Puzzle " << p << "\n";

		for( int r=2; r<6; r++ )
		for( int e=2; e<=p/2; e++ )
		for( int l=3; l<6; l++ )
		{
			if( p>10 && l==5 )	// Too much for these large puzzles.
				continue;

			CSettings & settings = getSettings();
			settings.p = p;
			settings.n = 1;
			settings.levels = l;
			settings.tileConfig = STileConfig( r, e );
			settings.dirty = true;
			buildPuzzle();
			
			if( 3 == l )
				file << "\n";

			file << "(" << r << "," << e << ")\t" << l << "\t" << m_cells.size() << "\n";
			file.flush();
		}
	}
}

void 
CPuzzle::clear() 
{
	m_cells.clear();
	m_slaves.clear();
	m_slaveToMaster.clear();
	m_masterToSlave.clear();
	m_stateSlaves.clear();
	m_dlPicking.clear();

	reset();
}

void 
CPuzzle::buildPuzzle() 
{
	if( !m_settings.dirty )
		return;

	m_building = true;
	clear();

	// Our template cell.
	CCell tCell;

	// Create a standard tile.
	CPolygon tile;
	tile.createRegular( m_settings.p );
	tCell.m_tile = tile;

	// Subdivide and reflect an edge of our tile.
	// The reflected points will help us determine our slicing circle locations.
	const CSegment & firstSeg = tile.m_segments[0];
	const CSegment & lastSeg = tile.m_segments[tile.numSides()-1];
	std::vector<CVector3D> subPoints = firstSeg.subdivide( m_settings.n );
	for( uint i=0; i<subPoints.size(); i++ )
		subPoints[i] = CTransformable::reflectPoint( lastSeg, subPoints[i] );
	
	/* pyraminx crystal
	std::vector<CVector3D> subPoints;
	subPoints.push_back( CVector3D() );
	CPolygon temp = tile;
	temp.reflect( temp.m_segments[0] );
	subPoints.push_back( temp.m_center );
	*/

	for( uint i=0; i<subPoints.size(); i++ )
		subPoints[i] *= m_settings.slicingExpansion;

	// Create set of template slicing circles.
	// These are all cocentric.
	// They will also become our twisting circles.
	std::vector<CCircle> tSlicers;
	for( int i=1; i<=m_settings.n/2; i++ )
	{
		CCircle c;
		//c.m_radius = subPoints[i].abs();
		c.m_radius = subPoints[i].x();
		tSlicers.push_back( c );
	}
	tCell.m_twisters = tSlicers;
	if( evenLength() )
		tSlicers.pop_back();

	// Fill out the vertex circle as well.
	CCircle vCircle;
	vCircle.m_radius = m_settings.p < 4 ? 0.5 : 1;	// Avoid infinite vCircles on triangle/digon puzzles.
	tCell.m_vertexCircle = vCircle;

	// Generate the full set of slicing circles.
	std::vector<CCircle> slicers;
	for( uint i=0; i<tile.numSides(); i++ )
	{
		const CSegment & s = tile.m_segments[i];
		for( uint j=0; j<tSlicers.size(); j++ )
		{
			CCircle copy = tSlicers[j];
			copy.reflect( s );
			slicers.push_back( copy );
		}
	}

	// Slice up the tile on all sides.
	bool digon = 2 == m_settings.p;
	bool oddDigon = digon && !evenLength();
	std::vector<CPolygon> slicees;
	if( evenLength() || oddDigon )
		doInitialEvenSlicing( oddDigon, tile, slicees );
	else
		slicees.push_back( tile );
	std::vector<CPolygon> sliced;
	if( !sliceRecursive( slicees, slicers, sliced ) )
	{
		assert( false );
	}

	// Setup the sticker centers.
	for( uint i=0; i<sliced.size(); i++ )
		sliced[i].m_center = sliced[i].centroid();
	tCell.m_stickers = sliced;

	//
	// We're done setting up our template cell now.
	// We can add in all the layers.
	//

	// Calc which cells will be at which layer.
	Vec3ToIntMap levels;
	levels[tCell.m_tile.m_center] = 0;
	int numLayers = m_settings.levels;
	if( digon && numLayers > 2 )			// XXX - Temporary fix.
		numLayers = 2;
	if( m_settings.hemi && 4 == m_settings.p && even( m_settings.n ) )	// XXX - Temporary fix.
		tCell.m_twisters.pop_back();
	int level = 0;
	std::vector<CPolygon> start;
	start.push_back( tCell.m_tile );
	calcLevelsRecursive( start, levels, numLayers, level );

	// Now add all the cells.
	Vec3ToBoolMap centers;
	Vec3ToIntMap slaveCenters;
	findOrAdd( tCell.m_tile.m_center, centers );
	addMasterCell( tCell, levels, slaveCenters );
	std::vector<int> added;
	added.push_back( 0 );
	addCellsRecursive( added, levels, centers, slaveCenters );

	// Connect up opposites.
	markOppositeFaces();

	makeHemiPuzzle();

	DEBUG_OUT( "Number of colors: " << m_cells.size() );
	DEBUG_OUT( "Total number of cells: " << m_cells.size() + m_slaves.size() );
	DEBUG_OUT( "Stickers per cell: " << m_cells[0].m_stickers.size() );

	// Setup a new state handler.
	m_state = CState( m_cells.size(), tCell.m_stickers.size() );

	m_settings.dirty = false;
	m_building = false;
}

void 
CPuzzle::markOppositeFaces() 
{
	// Mark opposite cells for Rubik's Cube and Megaminx only.
	// This doesn't apply to any other puzzles.
	if( !( 4 == m_settings.p || 5 == m_settings.p ) )
		return;

	for( uint i=0; i<m_cells.size(); i++ )
	{
		CCell & c1 = m_cells[i];
		CCircle circle = c1.m_vertexCircle;
		for( uint j=i+1; j<m_cells.size(); j++ )
		{
			CCell & c2 = m_cells[j];
			if( comparePointsCheckInfinity(
				CTransformable::reflectPoint( circle, c1.m_tile.m_center ),
				c2.m_tile.m_center ) )
			{
				// Mark both.
				c1.m_opp = j;
				c2.m_opp = i;
			}
		}
	}
}

// This method is hardcoded for this version.
// I hope to handle non-orientable puzzles more generically in the future C# version.
void 
CPuzzle::makeHemiPuzzle() 
{
	if( !m_settings.hemi )
		return;

	bool cube = 4 == m_settings.p;

	// This only applies to Rubik's Cube and Megaminx.
	// We are going to manually make opposites slaves instead of masters,
	// and set them to rotate in reverse.
	int numMasterCells = cube ? 3 : 6;
	for( int i=0; i<numMasterCells; i++ )
	{
		CCell & c = m_cells[i];

		// No more opposites.
		// Copy over the slicing circles we'll need.
		CCell & o = m_cells[c.m_opp];
		for( int t=o.m_twisters.size()-1; t>=0; t-- )
			c.m_twisters.push_back( o.m_twisters[t] );
		c.m_opp = -1;

		// We'll create a new opposite with a hardcoded set of reflections.
		// The set is important because the final stickers need to be oriented correctly.
		CCell newOpposite = c;
		if( cube )
		{
			// Even number of reflections, but still works.
			newOpposite.reflect( newOpposite.m_tile.m_segments[0] );
			newOpposite.reflect( newOpposite.m_tile.m_segments[2] );
		}
		else
		{
			if( i!=4 )
			{
				newOpposite.reflect( newOpposite.m_tile.m_segments[0] );
				newOpposite.rotateOne();
				newOpposite.reflect( newOpposite.m_tile.m_segments[2] );
				newOpposite.reflect( newOpposite.m_tile.m_segments[4] );
			}
			else
			{
				// Avoid reflecting through infinity by reversing the order.
				newOpposite.reflect( newOpposite.m_tile.m_segments[4] );
				newOpposite.reflect( newOpposite.m_tile.m_segments[2] );
				newOpposite.rotateOne();
				newOpposite.reflect( newOpposite.m_tile.m_segments[0] );
			}

			// Not really sure why this is required for the cell inverted by the projection,
			// but it was to get the sticker orderings right for state calculations.
			if( isInfinite( newOpposite.m_tile.m_center ) )
			{
				newOpposite.rotateOne();
				newOpposite.rotateOne();
			}
		}

		// Put in slave array.
		m_slaves.push_back( newOpposite );
		int masterIndex = i;
		int slaveIndex = m_slaves.size()-1;

		m_slaveToMaster[slaveIndex] = masterIndex;

		std::vector<int> newArray;
		newArray.push_back( slaveIndex );
		m_masterToSlave[masterIndex] = newArray;

		// Include all the slaves in state calcs (Is this ok?)
		m_stateSlaves.push_back( slaveIndex );
	}

	// Now remove the items which are now slaves.
	for( int i=0; i<numMasterCells; i++ )
		m_cells.pop_back();
}

//
// Breadth-first recursion to mark the graph levels of each center.
// Maybe there is a better way to get the levels, but I couldn't think of one.
// (all cells at a given level aren't the same distance from the origin for instance)
//
void 
CPuzzle::calcLevelsRecursive( const std::vector<CPolygon> & tiles, 
	Vec3ToIntMap & levels, int numLayers, int level ) 
{
	if( level >= numLayers )
		return;
	level++;

	std::vector<CPolygon> added;

	for( uint t=0; t<tiles.size(); t++ )
	{
		const CPolygon & tile = tiles[t];
		
		// Don't continue recursing with invalid tiles.
		// (check is for spherical geometries)
		if( !tile.valid() )
			continue;

		for( uint i=0; i<tile.numSides(); i++ )
		{
			CPolygon copy = tile;
			const CSegment & s = copy.m_segments[i];
			copy.reflect( s );

			Vec3ToIntMap::iterator it;
			it = levels.find( copy.m_center );
			if( it == levels.end() )
			{
				levels[copy.m_center] = level;
				added.push_back( copy );
			}
		}
	}

	calcLevelsRecursive( added, levels, numLayers, level );
}

void 
CPuzzle::addCellsRecursive( const std::vector<int> & masterIndices, 
	const Vec3ToIntMap & levels, Vec3ToBoolMap & centers, Vec3ToIntMap & slaveCenters ) 
{
	if( !masterIndices.size() )
		return;

	std::vector<int> added;

	for( uint c=0; c<masterIndices.size(); c++ )
	{
		// NOTE: This must be a copy, not a reference.
		//		 Otherwise, it can become invalid when m_cells is grown below.
		CCell tCell = m_cells[masterIndices[c]];

		// Don't continue recursing with invalid tiles.
		// (check is for spherical geometries)
		if( !tCell.m_tile.valid() )
			continue;

		uint count = tCell.m_tile.numSides();
		for( uint i=0; i<count; i++ )
		{
			const CSegment & s = tCell.m_tile.m_segments[i];

			// NOTE: We don't reflect the whole cell until we know we're going to add it!
			CVector3D center = CTransformable::reflectPoint( s, tCell.m_tile.m_center );
			int lod;
			if( !find( center, lod, levels ) )
				continue;

			// Track if we need to use a slave for state calcs.
			int slaveIndex;
			if( find( center, slaveIndex, slaveCenters ) && -1 != slaveIndex )
			{
				// Don't add duplicate values.
				// XXX - is this section too slow?
				std::vector<int>::const_iterator it =
					find( m_stateSlaves.begin(), m_stateSlaves.end(), slaveIndex );
				if( it == m_stateSlaves.end() )
					m_stateSlaves.push_back( slaveIndex );

				continue;
			}

			// Track if we have already done this cell.
			if( !findOrAdd( center, centers ) )
			{
				CCell copy = tCell;
				copy.m_lod = lod;
				copy.reflect( s );

				addMasterCell( copy, levels, slaveCenters );
				added.push_back( m_cells.size()-1 );
			}
		}
	}

	addCellsRecursive( added, levels, centers, slaveCenters );
}

void 
CPuzzle::addSlavesRecursive( const STileConfig & config, const CCell & master,
	const std::vector<int> & slaveIndices, const Vec3ToIntMap & levels, Vec3ToIntMap & centers ) 
{
	if( !slaveIndices.size() )
		return;

	std::vector<int> added;

	for( uint c=0; c<slaveIndices.size(); c++ )
	for( uint mirror=0; mirror<=1; mirror++ )
	{
		// NOTE: This must be a copy, not a reference.
		//		 Otherwise, it can become invalid when m_slaveCells is grown below.
		int slaveIndex = slaveIndices[c];
		CCell tCell = -1 == slaveIndex ? 
			master :
			m_slaves[slaveIndex];

		int reflections = config.r;
		int edgeStride = config.e;

		// Handle mirroring the config.
		if( 1 == mirror )
		{
			// We don't need to mirror these.
			if( even( m_settings.p ) && edgeStride == m_settings.p/2 )
				continue;

			edgeStride = m_settings.p - edgeStride;
		}

		uint count = tCell.m_tile.numSides();
		for( uint i=0; i<count; i++ )
		{
			// Indexes for reflection segments.
			int s1 = i;
			int s2 = i + edgeStride;
			if( s2 >= m_settings.p )
				s2 -= m_settings.p;

			// Reflect about to see if we want to do this one.
			// NOTE: We don't reflect the whole cell until we know we're going to add it!
			//		 Doing so caused horrendous build performance on large puzzle's, e.g. length-9 heptagonal.
			// XXX - code duplication below.
			CPolygon tile = tCell.m_tile;
			{
				// Orientation correction when tile config has an odd number of reflections.
				// This reflection needs to swap edges defined by s1 and s2.
				if( odd( reflections ) )
				{
					CSegment reflectionSeg = CCell::reflectBySwappingSegments( tile, s1, s2 );
					tile.reflect( reflectionSeg );
				}

				int numReflections = 0;
				for( int j=0; j<reflections; j++ )
				{
					// NOTE: Each reflection will 2-cycle the index of the segment
					//		 we want to reflect about (between startingSeg and startingSeg + edgeStride).
					const CSegment & s = even( numReflections ) ?
						tile.m_segments[s1] :
						tile.m_segments[s2];

					tile.reflect( s );
					numReflections++;
				}
			}

			const CVector3D & center = tile.m_center;
			int lod;
			if( !find( center, lod, levels ) )
				continue;

			// Track if we have already done this cell.
			int slaveIndex = m_slaves.size();
			if( !findOrAdd( center, slaveIndex, centers ) )
			{
				// Now make a copy and do all the reflections again, this time on the full cell.
				CCell copy = tCell;
				copy.m_lod = lod;
				{
					// Orientation correction when tile config has an odd number of reflections.
					// This reflection needs to swap edges defined by s1 and s2.
					if( odd( reflections ) )
					{
						CSegment reflectionSeg = CCell::reflectBySwappingSegments( copy.m_tile, s1, s2 );
						copy.reflect( reflectionSeg );
					}
					
					int numReflections = 0;
					for( int j=0; j<reflections; j++ )
					{
						// NOTE: Each reflection will 2-cycle the index of the segment
						//		 we want to reflect about (between startingSeg and startingSeg + edgeStride).
						const CSegment & s = even( numReflections ) ?
							copy.m_tile.m_segments[s1] :
							copy.m_tile.m_segments[s2];

						copy.reflect( s );
						numReflections++;
					}
				}

				//
				// Add to slaves and our tracking variables.
				//

				m_slaves.push_back( copy );
				
				int masterIndex = m_cells.size()-1;
				added.push_back( slaveIndex );

				m_slaveToMaster[slaveIndex] = masterIndex;

				std::map<int,std::vector<int>>::iterator it;
				it = m_masterToSlave.find( masterIndex );
				if( it == m_masterToSlave.end() )
				{
					std::vector<int> newArray;
					newArray.push_back( slaveIndex );
					m_masterToSlave[masterIndex] = newArray;
				}
				else
				{
					it->second.push_back( slaveIndex );
				}
			}
		}
	}

	addSlavesRecursive( config, master, added, levels, centers );
}

void 
CPuzzle::addMasterCell( const CCell & master, 
	const Vec3ToIntMap & levels, Vec3ToIntMap & slaveCenters ) 
{
	m_cells.push_back( master );

	if( !m_settings.infiniteTiling() )
		return;

	// Add in all slaves of this cell at this point.
	STileConfig slaveConfig = m_settings.tileConfig;
	std::vector<int> added;
	added.push_back( -1 );
	slaveCenters[master.m_tile.m_center] = -1;
	addSlavesRecursive( slaveConfig, master, added, levels, slaveCenters );
}

}
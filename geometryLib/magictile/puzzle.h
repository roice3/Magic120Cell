#pragma once

#include "polygon.h"
#include "puzzle/state.h"
#include "puzzle/twist.h"
#include "puzzle/twistHistory.h"
#include "renderer.h"

namespace magictile
{

struct STileConfig
{
	STileConfig() { r = e = 1; }
	STileConfig( int _r, int _e )
		{ r = _r; e = _e; }
	int r;	// Number of cell reflections
	int e;	// Edge stride
};

class CSettings
{
public:

	CSettings()
	{
		// Default to Rubik's Cube.
		p = 4;
		n = 3;
		levels = 3;

		// Only simplex vertex figures
		// supported at this point.
		q = 3;

		hemi = false;

		rotationStep = 2.123;
		lineThickness = .2;
		trippyMode = showOnlyFundamental = false;
		slicingExpansion = 1.0;
		dirty = true;
	}

	// Schlafli Symbol.
	int p;
	int q;

	// Whether we are a hemi-puzzle.
	bool hemi;

	// Number per side.
	int n;

	// Slicing circles expansion factor;
	double slicingExpansion;

	// Number of levels.
	int levels;

	// Tiling config.
	STileConfig tileConfig;

	// Rotation step (in degrees).
	// This is used to control the rotation rate.
	double rotationStep;

	double lineThickness;
	bool trippyMode;
	bool showOnlyFundamental;

	// Whether we've changed.
	bool dirty;

	int numTwistableSlices() const
	{
		// Rubik's Cube and Megaminx are special, since they
		// have opposite faces.
		if( !hemi && (4 == p || 5 == p) )
			return n;

		if( geometry() == Spherical )
			return n/2 + 1;

		return n/2;
	}

	Geometry geometry() const
	{
		return ::geometry( p );
	}
	
	bool infiniteTiling() const
	{
		Geometry g = geometry();
		return g == Euclidean || g == Hyperbolic;
	}
};

class CPuzzle : public IRenderable
{
public:

	CPuzzle();

	void clear();

	// Build a puzzle using the current settings.
	void buildPuzzle();

	// IRenderable impl.
	virtual void render( const CVector3D & lookFrom, bool forPicking );

	//
	// Settings related methods.
	//

	CSettings & getSettings() { return m_settings; }
	void setColor( uint i, CColor c );

	//
	// Methods to control twists/rotations.
	//

	void startRotate( const STwist & twist );
	void iterateRotate();
	void finishRotate();
	const STwist & getCurrentTwist() const;
	double getCurrentTwistStep() const;

	// Access to twist history.
	CTwistHistory & twistHistory() { return m_twistHistory; }

	// The rotation angle magnitude to make this twist, in radians.
	virtual double getCurrentTwistMagnitude() const;

	//
	// Other methods.
	//

	// Scramble/reset the puzzle.
	virtual void scramble( int numTwists );
	void reset();

	// Mark us as not scrambled.
	void markUnscrambled();

	// Reset the view since it can get all goofy looking.
	virtual void resetView();

	// Are we solved?
	bool isSolved() const;

	void invalidateAllCells();

	//
	// Accessor's for persistence only.
	//

	CState & accessState() { return m_state; }
	CTwistHistory & accessTwistHistory() { return m_twistHistory; }

private:

	// For debugging.
	void runThroughPuzzleTilingConfigs();

	void render();
	void renderForPicking();

	void markOppositeFaces();

	void makeHemiPuzzle();

	// Helper methods for building up the puzzle from template info.
	void calcLevelsRecursive( const std::vector<CPolygon> & tiles,
		Vec3ToIntMap & levels, int numLayers, int level );
	void addCellsRecursive( const std::vector<int> & masterIndices,
		const Vec3ToIntMap & levels, Vec3ToBoolMap & centers, Vec3ToIntMap & slaveCenters );
	void addSlavesRecursive( const STileConfig & config, const CCell & master,
		const std::vector<int> & slaveIndices, const Vec3ToIntMap & levels, Vec3ToIntMap & centers );

	void addMasterCell( const CCell & master, 
		const Vec3ToIntMap & levels, Vec3ToIntMap & slaveCenters );

	// Whether we are even length or not.
	bool evenLength() const;

	// Get twist data from the current twist.
	void calcTwistDataArray();
	CTwistData getTwistData( int twistCell, bool masterCell ) const;

	// Get the current rotation.
	double getCurrentRotation( bool rotating ) const;

private:

	// Our settings.
	CSettings m_settings;

	// Our cells.
	std::vector<CCell> m_cells;
	std::vector<CCell> m_slaves;	// For infinite tilings.

	// Master/Slave info.
	std::map<int,int> m_slaveToMaster;
	std::map<int,std::vector<int>> m_masterToSlave;

	// Our state.
	CState m_state;
	std::vector<int> m_stateSlaves;	// Slaves touching master tiles, used for state calcs.

	// Rotation variables.
	STwist m_currentTwist;
	CTwistDataArray m_currentTwistDataArray;
	bool m_rotating;
	double m_rotation;

	// Our twist history.
	CTwistHistory m_twistHistory;

	// Display list for picking.
	CDisplayList m_dlPicking;

	bool m_building;
};

}
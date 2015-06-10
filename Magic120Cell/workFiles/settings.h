#pragma once
#pragma managed(push, off)

#include "vectorND.h"


// We make most of our settings public for easy access.
// These settings should be applicable to any 4D puzzle.
class CSettings
{
public:

	CSettings();

	// The puzzle type.
	int m_puzzle;

	// Projection parameters.
	double m_projection4Distance;

	// Spacing parameters.
	double m_cellDistance;
	double m_cellSize;
	double m_stickerSize;

	// Sticker shape ratios.
	// NOTE: For accurate Megaminx cuts, r3 determins r2 and r1.
	//		 I have a spreadsheet to do that calc at the moment.
	double m_r1;	// Ratio of 1C sticker to full cell
	double m_r2;	// Ratio of 2C stickers to full cell face
	double m_r3;	// Ratio of 3C stickers to full cell face edge

	// Other options.
	bool m_drawInvertedCells;
	int m_symmetry;

	// Not currently used.
	bool m_drawWireframe;		
	bool m_reverseInvertedCellColor;

	// Rotation step (in degrees).
	// This is used to control the rotation rate.
	double m_rotationStep;

	// The logical visibility settings.
	short m_logicalVisibility;

	// Physical cell visibility (meaning depends on symmetry setting).
	std::vector<bool> m_visibility;

	// Highlight settings.
	bool m_highlight1C;
	bool m_highlight2C;
	bool m_highlight3C;
	bool m_highlight4C;

	// Our configured colors.
	// NOTE: These mean different things depending on m_puzzle setting.
	//		 They are used to fill out the full colors array in the state class.
	CColor m_colors[12];

	// The index current cell to put in the "center" of the puzzle.
	// XXX - This seems like it should not be in the settings.
	int m_centerCell;
};

#pragma managed(pop)
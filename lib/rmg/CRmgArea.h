/*
 * CRmgArea.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../int3.h"

namespace Rmg
{
	using Tileset = std::set<int3>;
	Tileset toAbsolute(const Tileset & tiles, const int3 & position);
	Tileset toRelative(const Tileset & tiles, const int3 & position);
	
	class Area
	{
	public:
		Area() = default;
		Area(const Area &);
		Area(const Area &&);
		Area(const Tileset & tiles);
		Area(const Tileset & relative, const int3 & position); //create from relative positions
		Area & operator= (const Area &);
		
		const Tileset & getTiles() const;
		std::vector<int3> getTilesVector() const;
		const Tileset & getBorder() const; //lazy cache invalidation
		const Tileset & getBorderOutside() const; //lazy cache invalidation
		
		Area getSubarea(std::function<bool(const int3 &)> filter) const;
		
		Tileset relative(const int3 & position) const;
		
		bool connected() const; //is connected
		bool empty() const;
		bool contains(const int3 & tile) const;
		bool contains(const Area & area) const;
		bool overlap(const Area & area) const;
		int distanceSqr(const int3 & tile) const;
		int distanceSqr(const Area & area) const;
		int3 nearest(const int3 & tile) const;
		int3 nearest(const Area & area) const;
		
		void clear();
		void assign(const Tileset & tiles);
		void add(const int3 & tile);
		void erase(const int3 & tile);
		void unite(const Area & area);
		void intersect(const Area & area);
		void subtract(const Area & area);
		void translate(const int3 & shift);
		
		friend Area operator+ (const Area & l, const int3 & r); //translation
		friend Area operator- (const Area & l, const int3 & r); //translation
		friend Area operator+ (const Area & l, const Area & r); //union
		friend Area operator* (const Area & l, const Area & r); //intersection
		friend Area operator- (const Area & l, const Area & r); //AreaL reduced by tiles from AreaB
		friend bool operator== (const Area & l, const Area & r);
		friend std::list<Area> connectedAreas(const Area & area);
		
	private:
		void computeBorderCache();
		void computeBorderOutsideCache();
		
		std::set<int3> dTiles;
		mutable std::set<int3> dBorderCache;
		mutable std::set<int3> dBorderOutsideCache;
	};
}
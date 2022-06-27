//
//  WaterAdopter.cpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#include "WaterAdopter.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "CRmgPath.h"
#include "CRmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TileInfo.h"

WaterAdopter::WaterAdopter(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void WaterAdopter::process()
{
	createWater(map.getMapGenOptions().getWaterContent());
}

void WaterAdopter::createWater(EWaterContent::EWaterContent waterContent)
{
	if(waterContent == EWaterContent::NONE || zone.isUnderground())
		return; //do nothing
	
	Rmg::Area waterArea(collectDistantTiles(zone, (float)(zone.getSize() + 1)));
	
	//add border tiles as water for ISLANDS
	if(waterContent == EWaterContent::ISLANDS)
	{
		waterArea.unite(zone.area().getBorder());
	}
	
	distanceMap = zone.area().computeDistanceMap(reverseDistanceMap);
	
	//generating some irregularity of coast
	//coastTilesMap - reverseDistanceMap
	//tilesDist - distanceMap
	int coastIdMax = fmin(sqrt(zone.area().getBorder().size()), 7.f); //size of coastTilesMap shows the most distant tile from water
	assert(coastIdMax > 0);
	std::list<int3> tilesQueue;
	Rmg::Tileset tilesChecked;
	for(int coastId = coastIdMax; coastId >= 1; --coastId)
	{
		//amount of iterations shall be proportion of coast perimeter
		const int coastLength = reverseDistanceMap[coastId].size() / (coastId + 3);
		for(int coastIter = 0; coastIter < coastLength; ++coastIter)
		{
			int3 tile = *RandomGeneratorUtil::nextItem(reverseDistanceMap[coastId], generator.rand);
			if(tilesChecked.find(tile) != tilesChecked.end())
				continue;
			
			//if(gen->isUsed(tile) || gen->isFree(tile)) //prevent placing water nearby town
			///	continue;
			
			tilesQueue.push_back(tile);
			tilesChecked.insert(tile);
		}
	}
	
	//if tile is marked as water - connect it with "big" water
	while(!tilesQueue.empty())
	{
		int3 src = tilesQueue.front();
		tilesQueue.pop_front();
		
		if(waterArea.contains(src))
			continue;
		
		waterArea.add(src);
				
		map.foreach_neighbour(src, [&src, this, &tilesChecked, &tilesQueue](const int3 & dst)
		{
			if(tilesChecked.count(dst))
				return;
			
			if(distanceMap[dst] > 0 && distanceMap[src] - distanceMap[dst] == 1)
			{
				tilesQueue.push_back(dst);
				tilesChecked.insert(dst);
			}
		});
	}
	
	//start filtering of narrow places and coast atrifacts
	Rmg::Area waterAdd;
	for(int coastId = 1; coastId <= coastIdMax; ++coastId)
	{
		for(auto& tile : reverseDistanceMap[coastId])
		{
			//collect neighbout water tiles
			auto collectionLambda = [&waterArea, this](const int3 & t, std::set<int3> & outCollection)
			{
				if(waterArea.contains(t))
				{
					reverseDistanceMap[0].insert(t);
					outCollection.insert(t);
				}
			};
			std::set<int3> waterCoastDirect, waterCoastDiag;
			map.foreachDirectNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDirect)));
			map.foreachDiagonalNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDiag)));
			int waterCoastDirectNum = waterCoastDirect.size();
			int waterCoastDiagNum = waterCoastDiag.size();
			
			//remove tiles which are mostly covered by water
			if(waterCoastDirectNum >= 3)
			{
				waterAdd.add(tile);
				continue;
			}
			if(waterCoastDiagNum == 4 && waterCoastDirectNum == 2)
			{
				waterAdd.add(tile);
				continue;
			}
			if(waterCoastDirectNum == 2 && waterCoastDiagNum >= 2)
			{
				int3 diagSum, dirSum;
				for(auto & i : waterCoastDiag)
					diagSum += i - tile;
				for(auto & i : waterCoastDirect)
					dirSum += i - tile;
				if(diagSum == int3() || dirSum == int3())
				{
					waterAdd.add(tile);
					continue;
				}
				if(waterCoastDiagNum == 3 && diagSum != dirSum)
				{
					waterAdd.add(tile);
					continue;
				}
			}
		}
	}
	
	waterArea.unite(waterAdd);
	
	//filtering tiny "lakes"
	for(auto& tile : reverseDistanceMap[0]) //now it's only coast-water tiles
	{
		if(!waterArea.contains(tile)) //for ground tiles
			continue;
		
		std::vector<int3> groundCoast;
		map.foreachDirectNeighbour(tile, [this, &waterArea, &groundCoast](const int3 & t)
		{
			if(!waterArea.contains(t) && zone.area().contains(t)) //for ground tiles of same zone
			{
				groundCoast.push_back(t);
			}
		});
		
		if(groundCoast.size() >= 3)
		{
			waterArea.erase(tile);
		}
		else
		{
			if(groundCoast.size() == 2)
			{
				if(groundCoast[0] + groundCoast[1] == int3())
				{
					waterArea.erase(tile);
				}
			}
		}
	}
	
	map.getZones()[waterZoneId]->area().unite(waterArea);
	reinitWaterZone(*map.getZones()[waterZoneId]);
}

void WaterAdopter::reinitWaterZone(Zone & wzone)
{
	wzone.areaPossible() = wzone.area();
	zone.area().subtract(wzone.area());
	zone.areaPossible().subtract(wzone.area());
	
	for(auto & t : wzone.area().getTiles())
	{
		map.setZoneID(t, waterZoneId);
		map.setOccupied(t, ETileType::POSSIBLE);
	}
	
	paintZoneTerrain(wzone, generator.rand, map, wzone.getTerrainType());
}

void WaterAdopter::setWaterZone(TRmgTemplateZoneId water)
{
	waterZoneId = water;
}
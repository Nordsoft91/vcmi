/*
 * TreasurePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"
#include "../mapObjects/ObjectTemplate.h"

class CGObjectInstance;
class ObjectManager;
class RmgMap;
class CMapGenerator;

struct DLL_LINKAGE ObjectInfo
{
	ObjectTemplate templ;
	ui32 value = 0;
	ui16 probability = 0;
	ui32 maxPerZone = -1;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;
	
	void setTemplate(si32 type, si32 subtype, Terrain terrain);
	
	ObjectInfo();
	
	bool operator==(const ObjectInfo& oi) const { return (templ == oi.templ); }
};

class DLL_LINKAGE TreasurePlacer: public Modificator
{
public:
	MODIFICATOR(TreasurePlacer);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	void createTreasures(ObjectManager & manager);
	
	void setQuestArtZone(Zone * otherZone);
	void addAllPossibleObjects(); //add objects, including zone-specific, to possibleObjects
	
protected:
	bool isGuardNeededForTreasure(int value);
	
	ObjectInfo * getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects);
	std::vector<ObjectInfo> prepareTreasurePile(const CTreasureInfo & treasureInfo);
	rmg::Object constuctTreasurePile(const std::vector<ObjectInfo> & treasureInfos);
	
	
protected:	
	std::vector<ObjectInfo> possibleObjects;
	int minGuardedValue = 0;
	
	rmg::Area treasureArea;
	rmg::Area treasureBlockArea;
	rmg::Area guards;
	
	Zone * questArtZone = nullptr; //artifacts required for Seer Huts will be placed here - or not if null
};

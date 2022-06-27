//
//  TownPlacer.cpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#include "TownPlacer.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../spells/CSpellHandler.h" //for choosing random spells
#include "CRmgPath.h"
#include "CRmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TileInfo.h"

TownPlacer::TownPlacer(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator), totalTowns(0)
{
}

void TownPlacer::process()
{
	auto & manager = *zone.getModificator<ObjectManager>();
	
	placeTowns(manager);
	placeMines(manager);
}

void TownPlacer::placeTowns(ObjectManager & manager)
{
	if((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START))
	{
		//set zone types to player faction, generate main town
		logGlobal->info("Preparing playing zone");
		int player_id = *zone.getOwner() - 1;
		auto & playerInfo = map.map().players[player_id];
		PlayerColor player(player_id);
		if(playerInfo.canAnyonePlay())
		{
			player = PlayerColor(player_id);
			zone.setTownType(map.getMapGenOptions().getPlayersSettings().find(player)->second.getStartingTown());
			
			if(zone.getTownType() == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				zone.setTownType(getRandomTownType(true));
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			zone.setTownType(getRandomTownType());
		}
		
		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, zone.getTownType());
		
		CGTownInstance * town = (CGTownInstance *) townFactory->create(ObjectTemplate());
		town->tempOwner = player;
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		
		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		//towns are big objects and should be centered around visitable position
		Rmg::Object rmgObject(*town);
		rmgObject.setPosition(zone.getPos() + town->getVisitableOffset());
		manager.placeObject(rmgObject, false, true);
		cleanupBoundaries(rmgObject);
		zone.setPos(town->visitablePos()); //roads lead to main town
		
		totalTowns++;
		//register MAIN town of zone only
		map.registerZone(town->subID);
		
		if(playerInfo.canAnyonePlay()) //configure info for owning player
		{
			logGlobal->trace("Fill player info %d", player_id);
			
			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(zone.getTownType());
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos;
			playerInfo.generateHeroAtMainTown = true;
			
			//now create actual towns
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, player, manager);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, player, manager);
		}
		else
		{
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, PlayerColor::NEUTRAL, manager);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
		}
	}
	else //randomize town types for any other zones as well
	{
		zone.setTownType(getRandomTownType());
	}
	
	addNewTowns(zone.getNeutralTowns().getCastleCount(), true, PlayerColor::NEUTRAL, manager);
	addNewTowns(zone.getNeutralTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
	
	if(!totalTowns) //if there's no town present, get random faction for dwellings and pandoras
	{
		//25% chance for neutral
		if (generator.rand.nextInt(1, 100) <= 25)
		{
			zone.setTownType(ETownType::NEUTRAL);
		}
		else
		{
			if(zone.getTownTypes().size())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getTownTypes(), generator.rand));
			else if(zone.getMonsterTypes().size())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getMonsterTypes(), generator.rand)); //this happens in Clash of Dragons in treasure zones, where all towns are banned
			else //just in any case
				zone.setTownType(getRandomTownType());
		}
	}
}

bool TownPlacer::placeMines(ObjectManager & manager)
{
	using namespace Res;
	std::vector<CGMine*> createdMines;
	
	for(const auto & mineInfo : zone.getMinesInfo())
	{
		ERes res = (ERes)mineInfo.first;
		for(int i = 0; i < mineInfo.second; ++i)
		{
			auto mine = (CGMine*)VLC->objtypeh->getHandlerFor(Obj::MINE, res)->create(ObjectTemplate());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			createdMines.push_back(mine);
			
			if(!i && (res == ERes::WOOD || res == ERes::ORE))
				manager.addCloseObject(mine, generator.getConfig().mineValues.at(res)); //only first wood&ore mines are close
			else
				manager.addRequiredObject(mine, generator.getConfig().mineValues.at(res));
		}
	}
	
	//create extra resources
	if(int extraRes = generator.getConfig().mineExtraResources)
	{
		for(auto * mine : createdMines)
		{
			for(int rc = generator.rand.nextInt(1, extraRes); rc > 0; --rc)
			{
				auto resourse = (CGResource*) VLC->objtypeh->getHandlerFor(Obj::RESOURCE, mine->producedResource)->create(ObjectTemplate());
				resourse->amount = CGResource::RANDOM_AMOUNT;
				manager.addNearbyObject(resourse, mine);
			}
		}
	}
	
	return true;
}


void TownPlacer::cleanupBoundaries(const Rmg::Object & rmgObject)
{
	for(auto & t : rmgObject.getArea().getBorderOutside())
	{
		if(map.isOnMap(t))
		{
			map.setOccupied(t, ETileType::FREE);
			zone.areaPossible().erase(t);
			zone.freePaths().add(t);
		}
	}
}

void TownPlacer::addNewTowns(int count, bool hasFort, PlayerColor player, ObjectManager & manager)
{
	for(int i = 0; i < count; i++)
	{
		si32 subType = zone.getTownType();
		
		if(totalTowns>0)
		{
			if(!zone.areTownsSameType())
			{
				if (zone.getTownTypes().size())
					subType = *RandomGeneratorUtil::nextItem(zone.getTownTypes(), generator.rand);
				else
					subType = *RandomGeneratorUtil::nextItem(zone.getDefaultTownTypes(), generator.rand); //it is possible to have zone with no towns allowed
			}
		}
		
		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, subType);
		auto town = (CGTownInstance *) townFactory->create(ObjectTemplate());
		town->ID = Obj::TOWN;
		
		town->tempOwner = player;
		if (hasFort)
			town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		
		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		
		if(totalTowns <= 0)
		{
			//FIXME: discovered bug with small zones - getPos is close to map boarder and we have outOfMap exception
			//register MAIN town of zone
			map.registerZone(town->subID);
			//first town in zone goes in the middle
			Rmg::Object rmgObject(*town);
			rmgObject.setPosition(zone.getPos() + town->getVisitableOffset());
			manager.placeObject(rmgObject, false, true);
			cleanupBoundaries(rmgObject);
			zone.setPos(town->visitablePos()); //roads lead to main town
		}
		else
			manager.addRequiredObject(town);
		totalTowns++;
	}
}

si32 TownPlacer::getRandomTownType(bool matchUndergroundType)
{
	auto townTypesAllowed = (zone.getTownTypes().size() ? zone.getTownTypes() : zone.getDefaultTownTypes());
	if(matchUndergroundType)
	{
		std::set<TFaction> townTypesVerify;
		for(TFaction factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*VLC->townh)[factionIdx]->preferUndergroundPlacement;
			if(zone.isUnderground() ? preferUnderground : !preferUnderground)
			{
				townTypesVerify.insert(factionIdx);
			}
		}
		if(!townTypesVerify.empty())
			townTypesAllowed = townTypesVerify;
	}
	
	return *RandomGeneratorUtil::nextItem(townTypesAllowed, generator.rand);
}
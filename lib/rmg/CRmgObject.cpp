/*
 * CRmgObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CRmgObject.h"
#include "CMapGenerator.h"
#include "../mapObjects/CObjectHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h

using namespace Rmg;

Object::Instance::Instance(const Object& parent, CGObjectInstance & object): dParent(parent), dObject(object)
{
	dBlockedArea = dObject.getBlockedPos();
	if(!dBlockedArea.empty() && dBlockedArea.getTiles().begin()->z == -1)
		dBlockedArea.translate(int3(0, 0, 1));
}

Object::Instance::Instance(const Object& parent, CGObjectInstance & object, const int3 & position): Instance(parent, object)
{
	setPosition(position);
}

const Area & Object::Instance::getBlockedArea() const
{
	return dBlockedArea;
}

const int3 & Object::Instance::getPosition() const
{
	return dPosition;
}

int3 Object::Instance::getVisitablePosition() const
{
	return dObject.getVisitableOffset() + dPosition;
}

void Object::Instance::setPosition(const int3 & position)
{
	dBlockedArea.translate(position - dPosition);
	dPosition = position;
	dParent.dFullAreaCache.clear();
}

void Object::Instance::setTemplate(ETerrainType terrain)
{
	if(dObject.appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(dObject.ID, dObject.subID)->getTemplates(terrain);
		if(templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") % dObject.ID % dObject.subID % terrain.toString()));
		
		dObject.appearance = templates.front();
	}
	dBlockedArea = dObject.getBlockedPos();
	if(!dBlockedArea.empty() && dBlockedArea.getTiles().begin()->z == -1)
		dBlockedArea.translate(int3(0, 0, 1));
	dBlockedArea.translate(dPosition);
}

bool Object::Instance::isVisitableFrom(const int3 & position) const
{
	/*auto tilesBlockedByObject = dObject.getBlockedPos();
	auto visitable = getVisitablePosition();
	if(tilesBlockedByObject.count(visitable))
		return false;
	return dObject.appearance.isVisitableFrom(position.x - visitable.x, position.y - visitable.y);*/
	return isVisitableFrom(position, dPosition);
}

bool Object::Instance::isVisitableFrom(const int3 & position, const int3 & potential) const
{
	//auto blockedTiles = getBlockedArea() - dPosition + potential;
	auto visitable = getVisitablePosition() - dPosition + potential;
	//if(tilesBlockedByObject.count(visitable))
	//	return false;
	//auto visitable = getVisitablePosition();
	return dObject.appearance.isVisitableFrom(position.x - visitable.x, position.y - visitable.y);
}

CGObjectInstance & Object::Instance::object()
{
	return dObject;
}

const CGObjectInstance & Object::Instance::object() const
{
	return dObject;
}

Object::Object(CGObjectInstance & object, const int3 & position)
{
	addInstance(object, position);
}

Object::Object(CGObjectInstance & object)
{
	addInstance(object);
}

Object::Object(const Object & object)
{
	int3 dPosition = object.dPosition;
	ui32 dStrenght = object.dStrenght;
	for(auto & i : object.dInstances)
		addInstance(const_cast<CGObjectInstance &>(i.object()), i.getPosition());
}

std::list<Object::Instance*> Object::instances()
{
	std::list<Object::Instance*> result;
	for(auto & i : dInstances)
		result.push_back(&i);
	return result;
}

std::list<const Object::Instance*> Object::instances() const
{
	std::list<const Object::Instance*> result;
	for(const auto & i : dInstances)
		result.push_back(&i);
	return result;
}

void Object::addInstance(Instance & object)
{
	//assert(object.dParent == *this);
	dInstances.push_back(object);
	dFullAreaCache.clear();
}

Object::Instance & Object::addInstance(CGObjectInstance & object)
{
	dInstances.emplace_back(*this, object);
	dFullAreaCache.clear();
	return dInstances.back();
}

Object::Instance & Object::addInstance(CGObjectInstance & object, const int3 & position)
{
	dInstances.emplace_back(*this, object, position);
	dFullAreaCache.clear();
	return dInstances.back();
}

const int3 & Object::getPosition() const
{
	return dPosition;
}

int3 Object::getVisitablePosition() const
{
	return dInstances.front().getVisitablePosition();
}

void Object::setPosition(const int3 & position)
{
	for(auto& i : dInstances)
		i.setPosition(position - dPosition);
	dPosition = position;
}

void Object::setTemplate(ETerrainType terrain)
{
	for(auto& i : dInstances)
		i.setTemplate(terrain);
}

const Area & Object::getArea() const
{
	if(!dFullAreaCache.empty() || dInstances.empty())
		return dFullAreaCache;
	
	for(const auto & instance : dInstances)
	{
		dFullAreaCache.unite(instance.getBlockedArea());
	}
	
	return dFullAreaCache;
}

void Object::Instance::finalize(CMapGenerator & generator)
{
	if(!generator.map->isInTheMap(dPosition))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % dObject.id % dPosition.toString()));
	dObject.pos = dPosition;
	
	if (dObject.isVisitable() && !generator.map->isInTheMap(dObject.visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % dObject.visitablePos().toString() % dObject.id % dObject.pos.toString()));
	
	for (auto & tile : dObject.getBlockedPos())
	{
		if (!generator.map->isInTheMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % dObject.id % dObject.pos.toString()));
	}
	
	if (dObject.appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = generator.map->getTile(dPosition).terType;
		auto templates = VLC->objtypeh->getHandlerFor(dObject.ID, dObject.subID)->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % dObject.ID % dObject.subID % dPosition.toString() % terrainType));
		
		dObject.appearance = templates.front();
	}
	
	for(auto & tile : dBlockedArea.getTiles())
		generator.setOccupied(tile, ETileType::ETileType::USED);
	generator.setOccupied(getVisitablePosition(), ETileType::ETileType::USED);
	
	generator.getEditManager()->insertObject(&dObject);
}

void Object::finalize(CMapGenerator & generator)
{
	if(dInstances.empty())
		throw rmgException("Cannot finalize object without instances");
		
	Area a = dInstances.front().getBlockedArea();
	dInstances.front().finalize(generator);
	
	for(auto iter = std::next(dInstances.begin()); iter != dInstances.end(); ++iter)
	{
		if(a.overlap(iter->getBlockedArea()))
			throw rmgException("Cannot finalize object: overlapped instance");
		a.unite(iter->getBlockedArea());
		iter->finalize(generator);
	}
}

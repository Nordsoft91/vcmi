//
//  Zone.hpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#pragma once

#include "../GameConstants.h"
#include "float3.h"
#include "../int3.h"
#include "CRmgTemplate.h"
#include "CRmgArea.h"
#include "CRmgObject.h"

class RmgMap;
class CMapGenerator;

class Modificator
{
public:
	virtual void process() = 0;
	
	void setName(const std::string & n)
	{
		name = n;
	}
	
	const std::string & getName() const
	{
		return name;
	}
	
	virtual ~Modificator() {};
	
private:
	std::string name;
};

class DLL_LINKAGE Zone : public rmg::ZoneOptions
{
public:
	Zone(RmgMap & map, CMapGenerator & generator);
	
	void setOptions(const rmg::ZoneOptions & options);
	bool isUnderground() const;
	
	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);
	
	const Rmg::Area & getArea() const;
	Rmg::Area & area();
	Rmg::Area & areaPossible();
	Rmg::Area & freePaths();
	Rmg::Area & areaUsed();
	
	void initFreeTiles();
	void clearTiles();
	void fractalize();
	
	si32 getTownType() const;
	void setTownType(si32 town);
	const Terrain & getTerrainType() const;
	void setTerrainType(const Terrain & terrain);
	
	bool crunchPath(const int3 & src, const int3 & dst, bool onlyStraight, std::set<int3> * clearedTiles = nullptr);
	bool connectPath(const int3 & src, bool onlyStraight);
	bool connectPath(const Rmg::Area & src, bool onlyStraight);
	
	template<class T>
	T* getModificator()
	{
		for(auto & m : modificators)
			if(auto * mm = dynamic_cast<T*>(m.get()))
				return mm;
		return nullptr;
	}
	
	template<class T>
	void addModificator(const std::string & name = "") //name is used for debug purposes
	{
		modificators.push_back(std::make_unique<T>(*this, map, generator));
		modificators.back()->setName(name);
	}
	
	void processModificators();
	
protected:
	CMapGenerator & generator;
	RmgMap & map;
	std::list<std::unique_ptr<Modificator>> modificators;
	
	//placement info
	int3 pos;
	float3 center;
	Rmg::Area dArea; //irregular area assined to zone
	Rmg::Area dAreaPossible;
	Rmg::Area dAreaFree; //core paths of free tiles that all other objects will be linked to
	Rmg::Area dAreaUsed;
	
	//template info
	si32 townType;
	Terrain terrainType;
	
};

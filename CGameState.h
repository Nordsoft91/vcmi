#ifndef CGAMESTATE_H
#define CGAMESTATE_H

#include "mapHandler.h"
#include <set>
#include <tchar.h>

class CScriptCallback;
class CCallback;
class CLuaCallback;
class CCPPObjectScript;
class CCreatureSet;
class CStack;
class CGHeroInstance;
class CArmedInstance;
struct Mapa;

struct PlayerState
{
public:
	int color, serial;
	//std::vector<std::vector<std::vector<char> > > fogOfWarMap; //true - visible, false - hidden
	PseudoV< PseudoV< PseudoV<unsigned char> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<int> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	PlayerState():color(-1){};
};

struct BattleInfo
{
	int side1, side2;
	int round, activeStack;
	int siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	int3 tile; //for background and bonuses
	CGHeroInstance *hero1, *hero2;
	CCreatureSet * army1, * army2;
	std::vector<CStack*> stacks;
	bool stackActionPerformed; //true if current stack has been moved
};

class CStack
{
public:
	int ID; //unique ID of stack
	CCreature * creature;
	int amount;
	int firstHPleft; //HP of first creature in stack
	int owner;
	bool attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	int position; //position on battlefield
	bool alive; //true if it is alive
	CStack(CCreature * C, int A, int O, int I, bool AO):creature(C),amount(A),owner(O), alive(true), position(-1), ID(I), attackerOwned(AO), firstHPleft(C->hitPoints){};
	CStack() : creature(NULL),amount(-1),owner(255), alive(true), position(-1), ID(-1), attackerOwned(true), firstHPleft(-1){};
};

class CGameState
{
private:
	int currentPlayer; //ID of player currently having turn
	BattleInfo *curB; //current battle
	int day; //total number of days in game
	Mapa * map;
	std::map<int,PlayerState> players; //ID <-> playerstate
	std::set<CCPPObjectScript *> cppscripts; //C++ scripts
	std::map<int, std::map<std::string, CObjectScript*> > objscr; //non-C++ scripts 
	

	bool checkFunc(int obid, std::string name)
	{
		if (objscr.find(obid)!=objscr.end())
		{
			if(objscr[obid].find(name)!=objscr[obid].end())
			{
				return true;
			}
		}
		return false;
	}
	CGHeroInstance * getHero(int ID, int mode)
	{
		if (mode != 0)
			throw new std::exception("gs->getHero: This mode is not supported!");
		for ( std::map<int, PlayerState>::iterator i=players.begin() ; i!=players.end();i++)
		{
			for (int j=0;j<(*i).second.heroes.size();j++)
			{
				if (i->second.heroes[j]->subID == ID)
					return i->second.heroes[j];
			}
		}
		return NULL;
	}
	void battle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CArmedInstance *hero1, CArmedInstance *hero2);
	bool battleMoveCreatureStack(int ID, int dest);
	bool battleAttackCreatureStack(int ID, int dest);
	std::vector<int> battleGetRange(int ID); //called by std::vector<int> CCallback::battleGetAvailableHexes(int ID);
public:
	friend CCallback;
	friend CPathfinder;;
	friend CLuaCallback;
	friend int _tmain(int argc, _TCHAR* argv[]);
	friend void initGameState(Mapa * map, CGameInfo * cgi);
	friend CScriptCallback;
	friend void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script);
	friend SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, SDL_Rect * extRect); //todo: wywalic koniecznie, tylko do flag obecnie!!!!
};

#endif //CGAMESTATE_H

/*
 * CArtifactHolder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactHolder.h"

#include "../gui/CGuiHandler.h"

#include "CComponent.h"

#include "../windows/GUIClasses.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

void CArtPlace::setInternals(const CArtifactInstance * artInst)
{
	baseType = -1; // By default we don't store any component
	ourArt = artInst;
	if(!artInst)
	{
		image->disable();
		text.clear();
		hoverText = CGI->generaltexth->allTexts[507];
		return;
	}
	image->enable();
	image->setFrame(artInst->artType->getIconIndex());
	if(artInst->getTypeId() == ArtifactID::SPELL_SCROLL)
	{
		auto spellID = artInst->getScrollSpellID();
		if(spellID.num >= 0)
		{
			// Add spell component info (used to provide a pic in r-click popup)
			baseType = CComponent::spell;
			type = spellID;
			bonusValue = 0;
		}
	}
	else
	{
		baseType = CComponent::artifact;
		type = artInst->getTypeId();
		bonusValue = 0;
	}
	text = artInst->getDescription();
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * Art) 
	: ourArt(Art)
{
	image = nullptr;
	pos += position;
	pos.w = pos.h = 44;
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	LRClickableAreaWTextComp::clickLeft(down, previousState);
}

void CArtPlace::clickRight(tribool down, bool previousState)
{
	LRClickableAreaWTextComp::clickRight(down, previousState);
}

const CArtifactInstance * CArtPlace::getArt()
{
	return ourArt;
}

CCommanderArtPlace::CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * Art)
	: CArtPlace(position, Art),
	commanderOwner(commanderOwner),
	commanderSlotID(artSlot.num)
{
	createImage();
	setArtifact(Art);
}

void CCommanderArtPlace::createImage()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	int imageIndex = 0;
	if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>("artifact", imageIndex);
	if(!ourArt)
		image->disable();
}

void CCommanderArtPlace::returnArtToHeroCallback()
{
	ArtifactPosition artifactPos = commanderSlotID;
	ArtifactPosition freeSlot = ArtifactUtils::getArtBackpackPosition(commanderOwner, ourArt->getTypeId());
	if(freeSlot == ArtifactPosition::PRE_FIRST)
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
	}
	else
	{
		ArtifactLocation src(commanderOwner->commander.get(), artifactPos);
		ArtifactLocation dst(commanderOwner, freeSlot);

		if(ourArt->canBePutAt(dst, true))
		{
			LOCPLINT->cb->swapArtifacts(src, dst);
			setArtifact(nullptr);
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickLeft(tribool down, bool previousState)
{
	if(ourArt && text.size() && down)
		LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
}

void CCommanderArtPlace::clickRight(tribool down, bool previousState)
{
	if(ourArt && text.size() && down)
		CArtPlace::clickRight(down, previousState);
}

void CCommanderArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
}

CHeroArtPlace::CHeroArtPlace(Point position, const CArtifactInstance * Art)
	: CArtPlace(position, Art),
	locked(false),
	marked(false)
{
	createImage();
}

void CHeroArtPlace::lockSlot(bool on)
{
	if(locked == on)
		return;

	locked = on;

	if(on)
		image->setFrame(ArtifactID::ART_LOCK);
	else if(ourArt)
		image->setFrame(ourArt->artType->getIconIndex());
	else
		image->setFrame(0);
}

bool CHeroArtPlace::isLocked()
{
	return locked;
}

void CHeroArtPlace::selectSlot(bool on)
{
	if(marked == on)
		return;

	marked = on;
	if(on)
		selection->enable();
	else
		selection->disable();
}

bool CHeroArtPlace::isMarked() const
{
	return marked;
}

void CHeroArtPlace::clickLeft(tribool down, bool previousState)
{
	if(down || !previousState)
		return;

	if(leftClickCallback)
		leftClickCallback(*this);
}

void CHeroArtPlace::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(rightClickCallback)
			rightClickCallback(*this);
	}
}

void CHeroArtPlace::showAll(SDL_Surface* to)
{
	if(ourArt)
	{
		CIntObject::showAll(to);
	}

	if(marked && active)
	{
		// Draw vertical bars.
		for(int i = 0; i < pos.h; ++i)
		{
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x, pos.y + i, 240, 220, 120);
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + pos.w - 1, pos.y + i, 240, 220, 120);
		}

		// Draw horizontal bars.
		for(int i = 0; i < pos.w; ++i)
		{
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + i, pos.y, 240, 220, 120);
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + i, pos.y + pos.h - 1, 240, 220, 120);
		}
	}
}

void CHeroArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
	if(art)
	{
		image->setFrame(locked ? ArtifactID::ART_LOCK : art->artType->getIconIndex());

		if(locked) // Locks should appear as empty.
			hoverText = CGI->generaltexth->allTexts[507];
		else
			hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % ourArt->artType->getNameTranslated());
	}
	else
	{
		lockSlot(false);
	}
}

void CHeroArtPlace::addCombinedArtInfo(std::map<const CArtifact*, int> & arts)
{
	for(const auto & combinedArt : arts)
	{
		std::string artList;
		text += "\n\n";
		text += "{" + combinedArt.first->getNameTranslated() + "}";
		if(arts.size() == 1)
		{
			for(const auto part : *combinedArt.first->constituents)
				artList += "\n" + part->getNameTranslated();
		}
		text += " (" + boost::str(boost::format("%d") % combinedArt.second) + " / " +
			boost::str(boost::format("%d") % combinedArt.first->constituents->size()) + ")" + artList;
	}
}

void CHeroArtPlace::createImage()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	si32 imageIndex = 0;

	if(locked)
		imageIndex = ArtifactID::ART_LOCK;
	else if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>("artifact", imageIndex);
	if(!ourArt)
		image->disable();

	selection = std::make_shared<CAnimImage>("artifact", ArtifactID::ART_SELECTION);
	selection->disable();
}

bool ArtifactUtilsClient::askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);
	auto assemblyPossibilities = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId(), ArtifactUtils::isSlotEquipment(slot));

	for(const auto combinedArt : assemblyPossibilities)
	{
		LOCPLINT->showArtifactAssemblyDialog(
			art->artType,
			combinedArt,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, true, combinedArt->getId()));

		if(assemblyPossibilities.size() > 2)
			logGlobal->warn("More than one possibility of assembling on %s... taking only first", art->artType->getNameTranslated());
		return true;
	}
	return false;
}

bool ArtifactUtilsClient::askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(art->canBeDisassembled())
	{
		if(ArtifactUtils::isSlotBackpack(slot) && !ArtifactUtils::isBackpackFreeSlots(hero, art->artType->constituents->size() - 1))
			return false;

		LOCPLINT->showArtifactAssemblyDialog(
			art->artType,
			nullptr,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, false, ArtifactID()));
		return true;
	}
	return false;
}

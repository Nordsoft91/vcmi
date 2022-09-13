#include "StdInc.h"
#include "inspector.h"
#include "../lib/CArtHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/mapping/CMap.h"

#include "townbulidingswidget.h"
#include "armywidget.h"
#include "messagewidget.h"
#include "rewardswidget.h"

//===============IMPLEMENT OBJECT INITIALIZATION FUNCTIONS================
Initializer::Initializer(CGObjectInstance * o, const PlayerColor & pl) : defaultPlayer(pl)
{
///IMPORTANT! initialize order should be from base objects to derived objects
	INIT_OBJ_TYPE(CGResource);
	INIT_OBJ_TYPE(CGArtifact);
	INIT_OBJ_TYPE(CArmedInstance);
	INIT_OBJ_TYPE(CGShipyard);
	INIT_OBJ_TYPE(CGGarrison);
	INIT_OBJ_TYPE(CGMine);
	INIT_OBJ_TYPE(CGDwelling);
	INIT_OBJ_TYPE(CGTownInstance);
	INIT_OBJ_TYPE(CGCreature);
	INIT_OBJ_TYPE(CGHeroInstance);
	INIT_OBJ_TYPE(CGSignBottle);
	INIT_OBJ_TYPE(CGLighthouse);
	//INIT_OBJ_TYPE(CGPandoraBox);
}

bool stringToBool(const QString & s)
{
	if(s == "TRUE")
		return true;
	//if(s == "FALSE")
	return false;
}

void Initializer::initialize(CArmedInstance * o)
{
	if(!o) return;
}

void Initializer::initialize(CGSignBottle * o)
{
	if(!o) return;
}

void Initializer::initialize(CGCreature * o)
{
	if(!o) return;
	
	o->character = CGCreature::Character::HOSTILE;
	o->putStack(SlotID(0), new CStackInstance(CreatureID(o->subID), 0, false));
}

void Initializer::initialize(CGDwelling * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	
	switch(o->ID)
	{
		case Obj::RANDOM_DWELLING:
		case Obj::RANDOM_DWELLING_LVL:
		case Obj::RANDOM_DWELLING_FACTION:
			o->initRandomObjectInfo();
	}
}

void Initializer::initialize(CGGarrison * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	o->removableUnits = true;
}

void Initializer::initialize(CGShipyard * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
}

void Initializer::initialize(CGLighthouse * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
}

void Initializer::initialize(CGHeroInstance * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
}

void Initializer::initialize(CGTownInstance * o)
{
	if(!o) return;

	const std::vector<std::string> castleLevels{"village", "fort", "citadel", "castle", "capitol"};
	int lvl = vstd::find_pos(castleLevels, o->appearance->stringID);
	o->builtBuildings.insert(BuildingID::DEFAULT);
	if(lvl > -1) o->builtBuildings.insert(BuildingID::TAVERN);
	if(lvl > 0) o->builtBuildings.insert(BuildingID::FORT);
	if(lvl > 1) o->builtBuildings.insert(BuildingID::CITADEL);
	if(lvl > 2) o->builtBuildings.insert(BuildingID::CASTLE);
	if(lvl > 3) o->builtBuildings.insert(BuildingID::CAPITOL);

	for(auto spell : VLC->spellh->objects) //add all regular spells to town
	{
		if(!spell->isSpecial() && !spell->isCreatureAbility())
			o->possibleSpells.push_back(spell->id);
	}
}

void Initializer::initialize(CGArtifact * o)
{
	if(!o) return;
	
	if(o->ID == Obj::SPELL_SCROLL)
	{
		std::vector<SpellID> out;
		for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
		{
			//if(map->isAllowedSpell(spell->id))
			{
				out.push_back(spell->id);
			}
		}
		auto a = CArtifactInstance::createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
		o->storedArtifact = a;
	}
}

void Initializer::initialize(CGMine * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	o->producedResource = Res::ERes(o->subID);
	o->producedQuantity = o->defaultResProduction();
}

void Initializer::initialize(CGResource * o)
{
	if(!o) return;
	
	o->amount = CGResource::RANDOM_AMOUNT;
}

//===============IMPLEMENT PROPERTIES SETUP===============================
void Inspector::updateProperties(CArmedInstance * o)
{
	if(!o) return;
	
	auto * delegate = new ArmyDelegate(*o);
	addProperty("Army", PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGDwelling * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGLighthouse * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGGarrison * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
	addProperty("Removable units", o->removableUnits, InspectorDelegate::boolDelegate(), false);
}

void Inspector::updateProperties(CGShipyard * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGHeroInstance * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, o->ID == Obj::PRISON); //field is not editable for prison
}

void Inspector::updateProperties(CGTownInstance * o)
{
	if(!o) return;
	
	addProperty("Town name", o->name, false);
	
	auto * delegate = new TownBuildingsDelegate(*o);
	addProperty("Buildings", PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGArtifact * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, false);
	
	CArtifactInstance * instance = o->storedArtifact;
	if(instance)
	{
		SpellID spellId = instance->getGivenSpellID();
		if(spellId != -1)
		{
			auto * delegate = new InspectorDelegate;
			for(auto spell : VLC->spellh->objects)
			{
				//if(map->isAllowedSpell(spell->id))
				delegate->options << QObject::tr(spell->name.c_str());
			}
			addProperty("Spell", VLC->spellh->objects[spellId]->name, delegate, false);
		}
	}
}

void Inspector::updateProperties(CGMine * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
	addProperty("Resource", o->producedResource);
	addProperty("Productivity", o->producedQuantity, false);
}

void Inspector::updateProperties(CGResource * o)
{
	if(!o) return;
	
	addProperty("Amount", o->amount, false);
	addProperty("Message", o->message, false);
}

void Inspector::updateProperties(CGSignBottle * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGCreature * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, false);
	{
		auto * delegate = new InspectorDelegate;
		delegate->options << "COMPLIANT" << "FRIENDLY" << "AGRESSIVE" << "HOSTILE" << "SAVAGE";
		addProperty<CGCreature::Character>("Character", (CGCreature::Character)o->character, delegate, false);
	}
	addProperty("Never flees", o->neverFlees, InspectorDelegate::boolDelegate(), false);
	addProperty("Not growing", o->notGrowingTeam, InspectorDelegate::boolDelegate(), false);
	addProperty("Artifact reward", o->gainedArtifact); //TODO: implement in setProperty
	addProperty("Army", PropertyEditorPlaceholder(), true);
	addProperty("Amount", o->stacks[SlotID(0)]->count, false);
	//addProperty("Resources reward", o->resources); //TODO: implement in setProperty
}

void Inspector::updateProperties(CGPandoraBox * o)
{
	if(!o) return;
	
	auto * delegate = new RewardsPandoraDelegate(*o);
	addProperty("Reward", PropertyEditorPlaceholder(), delegate, false);
}


void Inspector::updateProperties()
{
	if(!obj)
		return;
	table->setRowCount(0); //cleanup table
	
	addProperty("Indentifier", obj);
	addProperty("ID", obj->ID.getNum());
	addProperty("SubID", obj->subID);
	addProperty("InstanceName", obj->instanceName);
	addProperty("TypeName", obj->typeName);
	addProperty("SubTypeName", obj->subTypeName);
	auto factory = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
	addProperty("IsStatic", factory->isStaticObject());
	
	auto * delegate = new InspectorDelegate();
	delegate->options << "NEUTRAL";
	for(int p = 0; p < map->players.size(); ++p)
		if(map->players[p].canAnyonePlay())
			delegate->options << QString("PLAYER %1").arg(p);
	addProperty("Owner", obj->tempOwner, delegate, true);
	
	UPDATE_OBJ_PROPERTIES(CArmedInstance);
	UPDATE_OBJ_PROPERTIES(CGResource);
	UPDATE_OBJ_PROPERTIES(CGArtifact);
	UPDATE_OBJ_PROPERTIES(CGMine);
	UPDATE_OBJ_PROPERTIES(CGGarrison);
	UPDATE_OBJ_PROPERTIES(CGShipyard);
	UPDATE_OBJ_PROPERTIES(CGDwelling);
	UPDATE_OBJ_PROPERTIES(CGTownInstance);
	UPDATE_OBJ_PROPERTIES(CGCreature);
	UPDATE_OBJ_PROPERTIES(CGHeroInstance);
	UPDATE_OBJ_PROPERTIES(CGSignBottle);
	UPDATE_OBJ_PROPERTIES(CGLighthouse);
	UPDATE_OBJ_PROPERTIES(CGPandoraBox);
	
	table->show();
}

//===============IMPLEMENT PROPERTY UPDATE================================
void Inspector::setProperty(const QString & key, const QVariant & value)
{
	if(!obj)
		return;
	
	if(key == "Owner")
	{
		PlayerColor owner(value.toString().mid(6).toInt()); //receiving PLAYER N, N has index 6
		if(value == "NEUTRAL")
			owner = PlayerColor::NEUTRAL;
		if(value == "UNFLAGGABLE")
			owner = PlayerColor::UNFLAGGABLE;
		obj->tempOwner = owner;
	}
	
	SET_PROPERTIES(CArmedInstance);
	SET_PROPERTIES(CGTownInstance);
	SET_PROPERTIES(CGArtifact);
	SET_PROPERTIES(CGMine);
	SET_PROPERTIES(CGResource);
	SET_PROPERTIES(CGDwelling);
	SET_PROPERTIES(CGGarrison);
	SET_PROPERTIES(CGCreature);
	SET_PROPERTIES(CGHeroInstance);
	SET_PROPERTIES(CGShipyard);
	SET_PROPERTIES(CGSignBottle);
	SET_PROPERTIES(CGLighthouse);
	SET_PROPERTIES(CGPandoraBox);
}

void Inspector::setProperty(CArmedInstance * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGLighthouse * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGPandoraBox * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGTownInstance * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Town name")
		object->name = value.toString().toStdString();
}

void Inspector::setProperty(CGSignBottle * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Message")
		object->message = value.toString().toStdString();
}

void Inspector::setProperty(CGMine * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Productivity")
		object->producedQuantity = value.toString().toInt();
}

void Inspector::setProperty(CGArtifact * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Message")
		object->message = value.toString().toStdString();
	
	if(object->storedArtifact && key == "Spell")
	{
		for(auto spell : VLC->spellh->objects)
		{
			if(spell->name == value.toString().toStdString())
			{
				object->storedArtifact = CArtifactInstance::createScroll(spell->getId());
				break;
			}
		}
	}
}

void Inspector::setProperty(CGDwelling * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGGarrison * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Removable units")
		object->removableUnits = stringToBool(value.toString());
}

void Inspector::setProperty(CGHeroInstance * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGShipyard * object, const QString & key, const QVariant & value)
{
	if(!object) return;
}

void Inspector::setProperty(CGResource * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Amount")
		object->amount = value.toString().toInt();
}

void Inspector::setProperty(CGCreature * object, const QString & key, const QVariant & value)
{
	if(!object) return;
	
	if(key == "Message")
		object->message = value.toString().toStdString();
	if(key == "Character")
	{
		//COMPLIANT = 0, FRIENDLY = 1, AGRESSIVE = 2, HOSTILE = 3, SAVAGE = 4
		if(value == "COMPLIANT")
			object->character = CGCreature::Character::COMPLIANT;
		if(value == "FRIENDLY")
			object->character = CGCreature::Character::FRIENDLY;
		if(value == "AGRESSIVE")
			object->character = CGCreature::Character::AGRESSIVE;
		if(value == "HOSTILE")
			object->character = CGCreature::Character::HOSTILE;
		if(value == "SAVAGE")
			object->character = CGCreature::Character::SAVAGE;
	}
	if(key == "Never flees")
		object->neverFlees = stringToBool(value.toString());
	if(key == "Not growing")
		object->notGrowingTeam = stringToBool(value.toString());
	if(key == "Amount")
		object->stacks[SlotID(0)]->count = value.toString().toInt();
}


//===============IMPLEMENT PROPERTY VALUE TYPE============================
QTableWidgetItem * Inspector::addProperty(CGObjectInstance * value)
{
	using NumericPointer = unsigned long long;
	static_assert(sizeof(CGObjectInstance *) == sizeof(NumericPointer),
				  "Compilied for 64 bit arcitecture. Use NumericPointer = unsigned int");
	return new QTableWidgetItem(QString::number(reinterpret_cast<NumericPointer>(value)));
}

QTableWidgetItem * Inspector::addProperty(Inspector::PropertyEditorPlaceholder value)
{
	auto item = new QTableWidgetItem("");
	item->setData(Qt::UserRole, QString("PropertyEditor"));
	return item;
}

QTableWidgetItem * Inspector::addProperty(unsigned int value)
{
	return new QTableWidgetItem(QString::number(value));
}

QTableWidgetItem * Inspector::addProperty(int value)
{
	return new QTableWidgetItem(QString::number(value));
}

QTableWidgetItem * Inspector::addProperty(bool value)
{
	return new QTableWidgetItem(value ? "TRUE" : "FALSE");
}

QTableWidgetItem * Inspector::addProperty(const std::string & value)
{
	return addProperty(QString::fromStdString(value));
}

QTableWidgetItem * Inspector::addProperty(const QString & value)
{
	return new QTableWidgetItem(value);
}

QTableWidgetItem * Inspector::addProperty(const int3 & value)
{
	return new QTableWidgetItem(QString("(%1, %2, %3)").arg(value.x, value.y, value.z));
}

QTableWidgetItem * Inspector::addProperty(const PlayerColor & value)
{
	auto str = QString("PLAYER %1").arg(value.getNum());
	if(value == PlayerColor::NEUTRAL)
		str = "NEUTRAL";
	if(value == PlayerColor::UNFLAGGABLE)
		str = "UNFLAGGABLE";
	return new QTableWidgetItem(str);
}

QTableWidgetItem * Inspector::addProperty(const Res::ERes & value)
{
	QString str;
	switch (value) {
		case Res::ERes::WOOD:
			str = "WOOD";
			break;
		case Res::ERes::ORE:
			str = "ORE";
			break;
		case Res::ERes::SULFUR:
			str = "SULFUR";
			break;
		case Res::ERes::GEMS:
			str = "GEMS";
			break;
		case Res::ERes::MERCURY:
			str = "MERCURY";
			break;
		case Res::ERes::CRYSTAL:
			str = "CRYSTAL";
			break;
		case Res::ERes::GOLD:
			str = "GOLD";
			break;
		default:
			break;
	}
	return new QTableWidgetItem(str);
}

QTableWidgetItem * Inspector::addProperty(CGCreature::Character value)
{
	QString str;
	switch (value) {
		case CGCreature::Character::COMPLIANT:
			str = "COMPLIANT";
			break;
		case CGCreature::Character::FRIENDLY:
			str = "FRIENDLY";
			break;
		case CGCreature::Character::AGRESSIVE:
			str = "AGRESSIVE";
			break;
		case CGCreature::Character::HOSTILE:
			str = "HOSTILE";
			break;
		case CGCreature::Character::SAVAGE:
			str = "SAVAGE";
			break;
		default:
			break;
	}
	return new QTableWidgetItem(str);
}

//========================================================================

Inspector::Inspector(CMap * m, CGObjectInstance * o, QTableWidget * t): obj(o), table(t), map(m)
{
}

/*
 * Delegates
 */

InspectorDelegate * InspectorDelegate::boolDelegate()
{
	auto * d = new InspectorDelegate;
	d->options << "TRUE" << "FALSE";
	return d;
}

QWidget * InspectorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new QComboBox(parent);
}

void InspectorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(QComboBox *ed = qobject_cast<QComboBox *>(editor))
	{
		ed->addItems(options);
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void InspectorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(QComboBox *ed = qobject_cast<QComboBox *>(editor))
	{
		QMap<int, QVariant> data;
		data[0] = options[ed->currentIndex()];
		model->setItemData(index, data);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}


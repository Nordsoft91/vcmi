#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include "../lib/int3.h"
#include "../lib/GameConstants.h"
#include "../lib/mapObjects/MapObjects.h"
#include "../lib/ResourceSet.h"

#define DECLARE_OBJ_TYPE(x) void initialize(x*);
#define DECLARE_OBJ_PROPERTY_METHODS(x) \
void updateProperties(x*); \
void setProperty(x*, const QString &, const QVariant &);

#define INIT_OBJ_TYPE(x) initialize(dynamic_cast<x*>(o))
#define UPDATE_OBJ_PROPERTIES(x) updateProperties(dynamic_cast<x*>(obj))
#define SET_PROPERTIES(x) setProperty(dynamic_cast<x*>(obj), key, value)

//===============DECLARE MAP OBJECTS======================================
DECLARE_OBJ_TYPE(CArmedInstance);
DECLARE_OBJ_TYPE(CGTownInstance);
DECLARE_OBJ_TYPE(CGArtifact);
DECLARE_OBJ_TYPE(CGMine);
DECLARE_OBJ_TYPE(CGResource);

class Inspector
{
protected:
//===============DECLARE PROPERTIES SETUP=================================
	DECLARE_OBJ_PROPERTY_METHODS(CArmedInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGTownInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGArtifact);
	DECLARE_OBJ_PROPERTY_METHODS(CGMine);
	DECLARE_OBJ_PROPERTY_METHODS(CGResource);

//===============DECLARE PROPERTY VALUE TYPE==============================
	QTableWidgetItem * addProperty(unsigned int value);
	QTableWidgetItem * addProperty(int value);
	QTableWidgetItem * addProperty(const std::string & value);
	QTableWidgetItem * addProperty(const QString & value);
	QTableWidgetItem * addProperty(const int3 & value);
	QTableWidgetItem * addProperty(const PlayerColor & value);
	QTableWidgetItem * addProperty(const Res::ERes & value);
	QTableWidgetItem * addProperty(bool value);
	QTableWidgetItem * addProperty(CGObjectInstance * value);
	
//===============END OF DECLARATION=======================================
public:
	Inspector(CGObjectInstance *, QTableWidget *);

	void setProperty(const QString & key, const QVariant & value);

	void updateProperties();
	
protected:

	template<class T>
	void addProperty(const QString & key, const T & value, bool restricted = true)
	{
		auto * itemValue = addProperty(value);
		if(restricted)
			itemValue->setFlags(Qt::NoItemFlags);
		
		QTableWidgetItem * itemKey = nullptr;
		if(keyItems.contains(key))
		{
			itemKey = keyItems[key];
			table->setItem(table->row(itemKey), 1, itemValue);
		}
		else
		{
			itemKey = new QTableWidgetItem(key);
			itemKey->setFlags(Qt::NoItemFlags);
			keyItems[key] = itemKey;
			
			table->setRowCount(row + 1);
			table->setItem(row, 0, itemKey);
			table->setItem(row, 1, itemValue);
			++row;
		}
	}

protected:
	int row = 0;
	QTableWidget * table;
	CGObjectInstance * obj;
	QMap<QString, QTableWidgetItem*> keyItems;
};


class Initializer
{
public:
	Initializer(CGObjectInstance *);
};

class PlayerColorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	//void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	//QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private slots:
	void commitAndCloseEditor(int);
};

#endif // INSPECTOR_H
// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "NodeGraphRuntimeContext.h"

#include <Schematyc/Script/IScriptGraph.h>

#include <QStringList>

namespace CrySchematycEditor {

const CItemModelAttribute CNodesDictionary::s_columnAttributes[] =
{
	CItemModelAttribute("Name",            eAttributeType_String, CItemModelAttribute::Visible,      false, ""),
	CItemModelAttribute("_filter_string_", eAttributeType_String, CItemModelAttribute::AlwaysHidden, false, ""),
	// TODO: This should be a guid string later.
	CItemModelAttribute("_identifier_",    eAttributeType_Int,    CItemModelAttribute::AlwaysHidden, false, "")
	// ~TODO
};

QVariant CNodesDictionaryNodeEntry::GetColumnValue(int32 columnIndex) const
{
	switch (columnIndex)
	{
	case CNodesDictionary::eColumn_Name:
		{
			return QVariant::fromValue(m_name);
		}
	case CNodesDictionary::eColumn_Filter:
		{
			return QVariant::fromValue(m_fullName);
		}
	case CNodesDictionary::eColumn_Identifier:
		{
			return GetIdentifier();
		}
	default:
		break;
	}

	return QVariant();
}

const CAbstractDictionaryEntry* CNodesDictionaryNodeEntry::GetParentEntry() const
{
	return static_cast<const CAbstractDictionaryEntry*>(m_pParent);
}

QVariant CNodesDictionaryNodeEntry::GetIdentifier() const
{
	// TODO: This should be just a guid.
	return QVariant::fromValue(reinterpret_cast<quintptr>(m_pCommand.get()));
	// ~TODO
}

QVariant CNodesDictionaryCategoryEntry::GetColumnValue(int32 columnIndex) const
{
	switch (columnIndex)
	{
	case CNodesDictionary::eColumn_Name:
		{
			return QVariant::fromValue(m_name);
		}
	case CNodesDictionary::eColumn_Filter:
		{
			return QVariant::fromValue(m_fullName);
		}
	case CNodesDictionary::eColumn_Identifier:
	default:
		break;
	}

	return QVariant();
}

const CAbstractDictionaryEntry* CNodesDictionaryCategoryEntry::GetChildEntry(int32 index) const
{
	if (index < m_categories.size())
	{
		return static_cast<const CAbstractDictionaryEntry*>(m_categories[index]);
	}
	const int32 nodeIndex = index - m_categories.size();
	if (nodeIndex < m_nodes.size())
	{
		return static_cast<const CAbstractDictionaryEntry*>(m_nodes[nodeIndex]);
	}
	return nullptr;
}

const CAbstractDictionaryEntry* CNodesDictionaryCategoryEntry::GetParentEntry() const
{
	return static_cast<const CAbstractDictionaryEntry*>(m_pParent);
}

CNodesDictionary::CNodesDictionary()
{

}

CNodesDictionary::~CNodesDictionary()
{

}

const CAbstractDictionaryEntry* CNodesDictionary::GetEntry(int32 index) const
{
	if (index < m_categories.size())
	{
		return static_cast<const CAbstractDictionaryEntry*>(m_categories[index]);
	}
	const int32 nodeIndex = index - m_categories.size();
	if (nodeIndex < m_nodes.size())
	{
		return static_cast<const CAbstractDictionaryEntry*>(m_nodes[nodeIndex]);
	}
	return nullptr;
}

const CItemModelAttribute* CNodesDictionary::GetColumnAttribute(int32 index) const
{
	if (index < eColumn_COUNT)
	{
		return &s_columnAttributes[index];
	}
	return nullptr;
}

class CNodesDictionaryCreator : public Schematyc::IScriptGraphNodeCreationMenu
{
public:
	CNodesDictionaryCreator(CNodesDictionary& dictionary)
		: m_dictionary(dictionary)
	{}

	virtual bool AddOption(const char* szLabel, const char* szDescription, const char* szWikiLink, const Schematyc::IScriptGraphNodeCreationMenuCommandPtr& pCommand)
	{
		m_fullName = szLabel;

		QStringList categories = m_fullName.split("::");
		const QString name = categories.back();
		categories.removeLast();

		if (pCommand)
		{
			m_ppCommand = &pCommand;
			AddRecursive(name, categories, nullptr);
			return true;
		}

		return false;
	}

	void AddRecursive(const QString& name, QStringList& categories, CNodesDictionaryCategoryEntry* pParentCategory)
	{
		if (categories.size() > 0)
		{
			std::vector<CNodesDictionaryCategoryEntry*>* pCategoryItems = pParentCategory ? &pParentCategory->m_categories : &m_dictionary.m_categories;

			for (CNodesDictionaryCategoryEntry* pCategoryItem : * pCategoryItems)
			{
				if (pCategoryItem->GetName() == categories.front())
				{
					categories.removeFirst();
					AddRecursive(name, categories, pCategoryItem);
					return;
				}
			}

			CNodesDictionaryCategoryEntry* pNewCategory = nullptr;
			for (const QString& categoryName : categories)
			{
				pNewCategory = new CNodesDictionaryCategoryEntry(categoryName, pParentCategory);
				pCategoryItems->push_back(pNewCategory);
				pCategoryItems = &pNewCategory->m_categories;
				pParentCategory = pNewCategory;
			}

			if (pNewCategory)
			{
				CNodesDictionaryNodeEntry* pNode = new CNodesDictionaryNodeEntry(name, m_fullName, *m_ppCommand, pNewCategory);
				pNewCategory->m_nodes.push_back(pNode);
			}
		}
		else
		{
			CNodesDictionaryNodeEntry* pNewNodeItem = new CNodesDictionaryNodeEntry(name, m_fullName, *m_ppCommand, pParentCategory);
			if (pParentCategory)
			{
				pParentCategory->m_nodes.push_back(pNewNodeItem);
			}
			else
			{
				m_dictionary.m_nodes.push_back(pNewNodeItem);
			}
		}
	}

private:
	CNodesDictionary& m_dictionary;
	QString           m_fullName;

	const Schematyc::IScriptGraphNodeCreationMenuCommandPtr* m_ppCommand;
};

void CNodesDictionary::LoadLoadsFromScriptGraph(Schematyc::IScriptGraph& scriptGraph)
{
	CNodesDictionaryCreator creator(*this);
	scriptGraph.PopulateNodeCreationMenu(creator);
}

CNodeGraphRuntimeContext::CNodeGraphRuntimeContext(Schematyc::IScriptGraph& scriptGraph)
{
	m_nodesDictionary.LoadLoadsFromScriptGraph(scriptGraph);
}

}

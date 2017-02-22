// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "GraphNodeItem.h"
#include "GraphPinItem.h"

#include <Schematyc/Script/IScriptGraph.h>

#include <NodeGraph/NodeWidget.h>
#include <NodeGraph/PinGridNodeContentWidget.h>

#pragma optimize("", off)

namespace CrySchematycEditor {

CNodeItem::CNodeItem(Schematyc::IScriptGraphNode& scriptNode, CryGraphEditor::CNodeGraphViewModel& model)
	: CAbstractNodeItem(model)
	, m_scriptNode(scriptNode)
	, m_isDirty(false)
{
	SetAcceptsRenaming(false);
	SetAcceptsDeactivation(false);

	LoadFromScriptElement();
}

CNodeItem::~CNodeItem()
{
	for (CryGraphEditor::CAbstractPinItem* pItem : m_pins)
		delete pItem;
}

CryGraphEditor::CNodeWidget* CNodeItem::CreateWidget(CryGraphEditor::CNodeGraphView& view)
{
	CryGraphEditor::CNodeWidget* pNodeWidget = new CryGraphEditor::CNodeWidget(*this, view);
	CryGraphEditor::CPinGridNodeContentWidget* pContent = new CryGraphEditor::CPinGridNodeContentWidget(*pNodeWidget, view);

	return pNodeWidget;
}

void CNodeItem::Serialize(Serialization::IArchive& archive)
{
	Schematyc::ESerializationPass serPass;
	if (archive.isEdit())
	{
		serPass = Schematyc::ESerializationPass::Edit;
	}
	else
	{
		serPass = archive.isInput() ? Schematyc::ESerializationPass::Load : Schematyc::ESerializationPass::Save;
	}

	Schematyc::SSerializationContextParams serializationContextParams(archive, serPass);
	Schematyc::ISerializationContextPtr pSerializationContext = GetSchematycCore().CreateSerializationContext(serializationContextParams);

	m_scriptNode.Serialize(archive);
	if (archive.isInput())
	{
		m_isDirty = true;

		// TODO: This should happen in Serialize(...) function not here.
		GetSchematycCore().GetScriptRegistry().ElementModified(m_scriptNode.GetGraph().GetElement());
		// ~TODO

		Validate();
	}
}

QPointF CNodeItem::GetPosition() const
{
	Vec2 pos = m_scriptNode.GetPos();
	return QPointF(pos.x, pos.y);
}

void CNodeItem::SetPosition(QPointF position)
{
	Vec2 pos(position.x(), position.y());
	m_scriptNode.SetPos(pos);

	CAbstractNodeItem::SetPosition(position);
}

QVariant CNodeItem::GetId() const
{
	return QVariant::fromValue(m_scriptNode.GetGUID());
}

bool CNodeItem::HasId(QVariant id) const
{
	return (id.value<Schematyc::SGUID>() == m_scriptNode.GetGUID());
}

QVariant CNodeItem::GetTypeId() const
{
	return QVariant::fromValue(m_scriptNode.GetTypeGUID());
}

CryGraphEditor::CAbstractPinItem* CNodeItem::GetPinItemById(QVariant id) const
{
	const CPinId pinId = id.value<CPinId>();
	return GetPinItemById(pinId);
}

CPinItem* CNodeItem::GetPinItemById(CPinId id) const
{
	if (id.IsInput())
	{
		for (CryGraphEditor::CAbstractPinItem* pAbstractPinItem : m_pins)
		{
			CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
			if (pPinItem->IsInputPin() && pPinItem->GetPortId() == id.GetPortId())
			{
				return pPinItem;
			}
		}
	}
	else
	{
		for (CryGraphEditor::CAbstractPinItem* pAbstractPinItem : m_pins)
		{
			CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
			if (pPinItem->IsOutputPin() && pPinItem->GetPortId() == id.GetPortId())
			{
				return pPinItem;
			}
		}
	}

	return nullptr;
}

Schematyc::SGUID CNodeItem::GetGUID() const
{
	return m_scriptNode.GetGUID();
}

void CNodeItem::Refresh(bool bForceRefresh)
{
	if (m_isDirty || bForceRefresh)
	{
		m_isDirty = false;

		// TODO: Just call RefreshLayout(...) here?!
		m_scriptNode.ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorRefresh));
		// ~TODO

		// We iterate through all pins and check what was removed/added.
		const uint32 numPins = m_scriptNode.GetInputCount() + m_scriptNode.GetOutputCount();

		CryGraphEditor::PinItemArray pins;
		pins.reserve(numPins);

		size_t numKeptPins = 0;

		const uint32 numInputPins = m_scriptNode.GetInputCount();
		for (uint32 i = 0; i < numInputPins; ++i)
		{
			CPinId pinId(m_scriptNode.GetInputId(i), CPinId::EType::Input);
			auto predicate = [pinId](CryGraphEditor::CAbstractPinItem* pAbstractPinItem) -> bool
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
				return (pPinItem && pinId == pPinItem->GetPinId());
			};

			auto result = std::find_if(m_pins.begin(), m_pins.end(), predicate);
			if (result == m_pins.end())
			{
				CPinItem* pPinItem = new CPinItem(i, EPinFlag::Input, *this, GetViewModel());
				pins.push_back(pPinItem);

				SignalPinAdded(*pPinItem);
			}
			else
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(*result);
				pPinItem->UpdateWithNewIndex(i);
				pins.push_back(pPinItem);

				*result = nullptr;
				++numKeptPins;
			}
		}

		const uint32 numOutputPins = m_scriptNode.GetOutputCount();
		for (uint32 i = 0; i < numOutputPins; ++i)
		{
			CPinId pinId(m_scriptNode.GetOutputId(i), CPinId::EType::Output);
			auto predicate = [pinId](CryGraphEditor::CAbstractPinItem* pAbstractPinItem) -> bool
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
				return (pPinItem && pinId == pPinItem->GetPinId());
			};

			auto result = std::find_if(m_pins.begin(), m_pins.end(), predicate);
			if (result == m_pins.end())
			{
				CPinItem* pPinItem = new CPinItem(i, EPinFlag::Output, *this, GetViewModel());
				pins.push_back(pPinItem);

				SignalPinAdded(*pPinItem);
			}
			else
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(*result);
				pPinItem->UpdateWithNewIndex(i);
				pins.push_back(pPinItem);

				*result = nullptr;
				++numKeptPins;
			}
		}

		m_pins.swap(pins);
		CRY_ASSERT(m_pins.size() == numPins);

		for (CryGraphEditor::CAbstractPinItem* pPinItem : pins)
		{
			if (pPinItem != nullptr)
			{
				SignalPinRemoved(*pPinItem);
				delete pPinItem;
			}
		}
	}
}

void CNodeItem::LoadFromScriptElement()
{
	// TODO: Just call RefreshLayout(...) here?!
	m_scriptNode.ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorRefresh)); // N.B. After the release of 5.3 we should figure out whether it's really necessary to refresh elements upon initial selection (as opposed to only refreshing when changes are made).
	// ~TODO

	m_fullQualifiedName = m_scriptNode.GetName();
	QStringList fullQualifiedName = m_fullQualifiedName.split("::");
	m_shortName = fullQualifiedName.back();

	if (m_shortName == "")
	{
		m_shortName = m_fullQualifiedName;
	}

	m_fullQualifiedName = QString("<b>FQNN: </b>%1").arg(m_fullQualifiedName);

	const Vec2 pos = m_scriptNode.GetPos();
	SetPosition(QPoint(pos.x, pos.y));

	const ColorB headerColor = m_scriptNode.GetColor();
	m_headerGradientColorL = QColor(headerColor.r, headerColor.g, headerColor.b, headerColor.a);
	m_headerGradientColorR = m_headerGradientColorL;

	const uint32 numInputs = m_scriptNode.GetInputCount();
	for (uint32 i = 0; i < numInputs; ++i)
	{
		CryGraphEditor::CAbstractPinItem* pPinItem = new CPinItem(i, EPinFlag::Input, *this, GetViewModel());
		m_pins.push_back(pPinItem);
	}

	const uint32 numOutputs = m_scriptNode.GetOutputCount();
	for (uint32 i = 0; i < numOutputs; ++i)
	{
		CryGraphEditor::CAbstractPinItem* pPinItem = new CPinItem(i, EPinFlag::Output, *this, GetViewModel());
		m_pins.push_back(pPinItem);
	}

	Validate();
}

void CNodeItem::Validate()
{
	uint32 warningCount = 0;
	uint32 errorCount = 0;

	auto validateScriptNode = [&warningCount, &errorCount](Schematyc::EValidatorMessageType messageType, const char* szMessage)
	{
		switch (messageType)
		{
		case Schematyc::EValidatorMessageType::Warning:
			{
				++warningCount;
				break;
			}
		case Schematyc::EValidatorMessageType::Error:
			{
				++errorCount;
				break;
			}
		}
	};
	m_scriptNode.Validate(Schematyc::Validator::FromLambda(validateScriptNode));

	CAbstractNodeItem::SetWarnings(warningCount > 0);
	CAbstractNodeItem::SetErrors(errorCount > 0);
}

}

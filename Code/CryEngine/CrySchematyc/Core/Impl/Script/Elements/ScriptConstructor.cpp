// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptConstructor.h"

#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

#include "Script/Graph/ScriptGraph.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"

namespace Schematyc
{
CScriptConstructor::CScriptConstructor()
	: CScriptElementBase({ EScriptElementFlags::CanOwnScript, EScriptElementFlags::FixedName })
{
	CreateGraph();
}

CScriptConstructor::CScriptConstructor(const SGUID& guid, const char* szName)
	: CScriptElementBase(guid, szName, { EScriptElementFlags::CanOwnScript, EScriptElementFlags::FixedName })
{
	CreateGraph();
}

void CScriptConstructor::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const
{
	CScriptElementBase::EnumerateDependencies(enumerator, type);
}

void CScriptConstructor::RemapDependencies(IGUIDRemapper& guidRemapper) {}

void CScriptConstructor::ProcessEvent(const SScriptEvent& event)
{
	CScriptElementBase::ProcessEvent(event);

	switch (event.id)
	{
	case EScriptEventId::EditorAdd:
	case EScriptEventId::EditorPaste:
		{
			m_userDocumentation.SetCurrentUserAsAuthor();
			break;
		}
	}
}

void CScriptConstructor::Serialize(Serialization::IArchive& archive)
{
	LOADING_TIME_PROFILE_SECTION;

	CScriptElementBase::Serialize(archive);

	switch (SerializationContext::GetPass(archive))
	{
	case ESerializationPass::LoadDependencies:
	case ESerializationPass::Save:
		{
			archive(m_userDocumentation, "userDocumentation");
			break;
		}
	case ESerializationPass::Edit:
		{
			archive(m_userDocumentation, "userDocumentation", "Documentation");
			break;
		}
	}

	CScriptElementBase::SerializeExtensions(archive);
}

void CScriptConstructor::CreateGraph()
{
	CScriptGraphPtr pScriptGraph = std::make_shared<CScriptGraph>(*this, EScriptGraphType::Construction);
	pScriptGraph->AddNode(std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphBeginNode>())); // #SchematycTODO : Shouldn't we be using CScriptGraphNodeFactory::CreateNode() instead of instantiating the node directly?!?
	CScriptElementBase::AddExtension(pScriptGraph);
}
} // Schematyc

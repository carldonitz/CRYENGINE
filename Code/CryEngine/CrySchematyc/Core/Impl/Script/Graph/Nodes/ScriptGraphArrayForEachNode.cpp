// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphArrayForEachNode.h"

#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/AnyArray.h>
#include <Schematyc/Utils/StackString.h>

#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphArrayForEachNode::SRuntimeData::SRuntimeData()
	: pos(InvalidIdx)
{}

CScriptGraphArrayForEachNode::SRuntimeData::SRuntimeData(const SRuntimeData& rhs)
	: pos(rhs.pos)
{}

SGUID CScriptGraphArrayForEachNode::SRuntimeData::ReflectSchematycType(CTypeInfo<SRuntimeData>& typeInfo)
{
	return "3270fe0e-fd07-46cb-9ac5-99ee7d03f55a"_schematyc_guid;
}

CScriptGraphArrayForEachNode::CScriptGraphArrayForEachNode(const SElementId& reference)
	: m_defaultValue(reference)
{}

SGUID CScriptGraphArrayForEachNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphArrayForEachNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("Array::ForEach");
	layout.SetColor(EScriptGraphColor::Purple);

	layout.AddInput("In", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	layout.AddOutput("Out", SGUID(), EScriptGraphPortFlags::Flow);
	layout.AddOutput("Loop", SGUID(), EScriptGraphPortFlags::Flow);

	if (!m_defaultValue.IsEmpty())
	{
		const SGUID typeGUID = m_defaultValue.GetTypeId().guid;
		layout.AddInput("Array", typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Array });
		layout.AddOutputWithData(m_defaultValue.GetTypeName(), typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, *m_defaultValue.GetValue());
	}
}

void CScriptGraphArrayForEachNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
	compiler.BindData(SRuntimeData());
}

void CScriptGraphArrayForEachNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayForEachNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayForEachNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	{
		ScriptVariableData::CScopedSerializationConfig serializationConfig(archive);

		const SGUID guid = CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID();
		serializationConfig.DeclareEnvDataTypes(guid);
		serializationConfig.DeclareScriptEnums(guid);
		serializationConfig.DeclareScriptStructs(guid);

		m_defaultValue.SerializeTypeId(archive);
	}
}

void CScriptGraphArrayForEachNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_defaultValue.RemapDependencies(guidRemapper);
}

void CScriptGraphArrayForEachNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			inline CNodeCreationMenuCommand(const SElementId& reference = SElementId())
				: m_reference(reference)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphArrayForEachNode>(m_reference), pos);
			}

			// ~IMenuCommand

		private:

			SElementId m_reference;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphArrayForEachNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphArrayForEachNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			nodeCreationMenu.AddOption("Array::ForEach", "Iterate through elements in array", "", std::make_shared<CNodeCreationMenuCommand>());

			const char* szLabel = "Array::ForEach";
			const char* szDescription = "Iterate through all elements in array";

			nodeCreationMenu.AddOption(szLabel, szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>());

			// #SchematycTODO : This code is duplicated in all array nodes, find a way to reduce the duplication.

			auto visitEnvDataType = [&nodeCreationMenu, &scriptView, szLabel, szDescription](const IEnvDataType& envDataType) -> EVisitStatus
			{
				CStackString label;
				scriptView.QualifyName(envDataType, label);
				label.append("::");
				label.append(szLabel);
				nodeCreationMenu.AddOption(label.c_str(), szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Env, envDataType.GetGUID())));
				return EVisitStatus::Continue;
			};
			scriptView.VisitEnvDataTypes(EnvDataTypeConstVisitor::FromLambda(visitEnvDataType));
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphArrayForEachNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	SRuntimeData& data = DynamicCast<SRuntimeData>(*context.node.GetData());
	if (activationParams.mode == EActivationMode::Input)
	{
		data.pos = 0;
	}

	CAnyArrayPtr pArray = DynamicCast<CAnyArrayPtr>(*context.node.GetInputData(EInputIdx::Array));
	if (data.pos < pArray->GetSize())
	{
		Any::CopyAssign(*context.node.GetOutputData(EOutputIdx::Value), (*pArray)[data.pos++]);
		return SRuntimeResult(ERuntimeStatus::ContinueAndRepeat, EOutputIdx::Loop);
	}
	else
	{
		return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
	}
}

const SGUID CScriptGraphArrayForEachNode::ms_typeGUID = "67348889-afb6-4926-9275-9cb95e507787"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphArrayForEachNode::Register)

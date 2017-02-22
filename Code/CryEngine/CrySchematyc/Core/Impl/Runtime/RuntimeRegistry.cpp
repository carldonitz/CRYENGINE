// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "RuntimeRegistry.h"

#include "Runtime/RuntimeClass.h"

#ifdef RegisterClass
#undef RegisterClass
#endif

namespace Schematyc
{
IRuntimeClassConstPtr CRuntimeRegistry::GetClass(const SGUID& guid) const
{
	Classes::const_iterator itClass = m_classes.find(guid);
	return itClass != m_classes.end() ? itClass->second : IRuntimeClassConstPtr();
}

void CRuntimeRegistry::RegisterClass(const CRuntimeClassConstPtr& pClass)
{
	SCHEMATYC_CORE_ASSERT(pClass);

	const SGUID guid = pClass->GetGUID();
	if (GUID::IsEmpty(guid) || GetClass(guid))
	{
		SCHEMATYC_CORE_CRITICAL_ERROR("Unable to register runtime class!");
		return;
	}

	m_classes.insert(Classes::value_type(guid, pClass));
}

void CRuntimeRegistry::ReleaseClass(const SGUID& guid)
{
	m_classes.erase(guid);
}

CRuntimeClassConstPtr CRuntimeRegistry::GetClassImpl(const SGUID& guid) const
{
	Classes::const_iterator itClass = m_classes.find(guid);
	return itClass != m_classes.end() ? itClass->second : CRuntimeClassConstPtr();
}

void CRuntimeRegistry::Reset()
{
	m_classes.clear();
}
} // Schematyc

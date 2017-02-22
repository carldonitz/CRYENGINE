// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/ICore.h"
#include "Schematyc/Utils/Assert.h"
#include "CrySystem/ISystem.h"

namespace Schematyc
{
// Forward declare interfaces.
struct ILogOutput;
// Forward declare classes.
class CCompiler;
class CEnvRegistry;
class CLog;
class CLogRecorder;
class CObjectPool;
class CRuntimeRegistry;
class CScriptRegistry;
class CSettingsManager;
class CTimerSystem;
class CUpdateScheduler;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(ILogOutput)

class CCore : public ICrySchematycCore, public ISystemEventListener
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(CCore)
		CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()
		
	CRYGENERATE_SINGLETONCLASS(CCore, "Plugin_SchematycCore", 0x96d98d9835aa4fb6, 0x830b53dbfe71908d)

public:

	// ICryPlugin
	virtual const char* GetName() const override { return "SchematycCore"; }
	virtual const char* GetCategory() const override { return "Plugin"; }
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	// ~ICryPlugin

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	// IPluginUpdateListener
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override;
	// ~IPluginUpdateListener

	// ICore
	virtual void                     SetGUIDGenerator(const GUIDGenerator& guidGenerator) override;
	virtual SGUID                    CreateGUID() const override;

	virtual const char*              GetFileFormat() const override;
	virtual const char*              GetRootFolder() const override;
	virtual const char*              GetScriptsFolder() const override;
	virtual const char*              GetSettingsFolder() const override;
	virtual bool                     IsExperimentalFeatureEnabled(const char* szFeatureName) const override;

	virtual IEnvRegistry&            GetEnvRegistry() override;
	virtual IScriptRegistry&         GetScriptRegistry() override;
	virtual IRuntimeRegistry&        GetRuntimeRegistry() override;
	virtual ICompiler&               GetCompiler() override;
	virtual ILog&                    GetLog() override;
	virtual ILogRecorder&            GetLogRecorder() override;
	virtual ISettingsManager&        GetSettingsManager() override;
	virtual IUpdateScheduler&        GetUpdateScheduler() override;
	virtual ITimerSystem&            GetTimerSystem() override;

	virtual IValidatorArchivePtr     CreateValidatorArchive(const SValidatorArchiveParams& params) const override;
	virtual ISerializationContextPtr CreateSerializationContext(const SSerializationContextParams& params) const override;
	virtual IScriptViewPtr           CreateScriptView(const SGUID& scopeGUID) const override;

	virtual IObject*                 CreateObject(const SObjectParams& params) override;
	virtual IObject*                 GetObject(ObjectId objectId) override;
	virtual void                     DestroyObject(ObjectId objectId) override;
	virtual void                     SendSignal(ObjectId objectId, const SGUID& signalGUID, CRuntimeParams& params) override;
	virtual void                     BroadcastSignal(const SGUID& signalGUID, CRuntimeParams& params) override;

	virtual void                     RefreshLogFileSettings() override;
	virtual void                     RefreshEnv() override;	
	// ~ICore

	void       RefreshLogFileStreams();
	void       RefreshLogFileMessageTypes();

	CRuntimeRegistry& GetRuntimeRegistryImpl();
	CCompiler& GetCompilerImpl();

	static CCore* GetInstance() { return s_pInstance; }

private:
	void                     PrePhysicsUpdate();
	void                     Update();
	
	void                     LoadProjectFiles();

	GUIDGenerator                     m_guidGenerator;
	mutable string                    m_scriptsFolder;    // #SchematycTODO : How can we avoid making this mutable?
	mutable string                    m_settingsFolder;   // #SchematycTODO : How can we avoid making this mutable?
	std::unique_ptr<CEnvRegistry>     m_pEnvRegistry;
	std::unique_ptr<CScriptRegistry>  m_pScriptRegistry;
	std::unique_ptr<CRuntimeRegistry> m_pRuntimeRegistry;
	std::unique_ptr<CCompiler>        m_pCompiler;
	std::unique_ptr<CObjectPool>      m_pObjectPool;
	std::unique_ptr<CTimerSystem>     m_pTimerSystem;
	std::unique_ptr<CLog>             m_pLog;
	ILogOutputPtr                     m_pLogFileOutput;
	std::unique_ptr<CLogRecorder>     m_pLogRecorder;
	std::unique_ptr<CSettingsManager> m_pSettingsManager;
	std::unique_ptr<CUpdateScheduler> m_pUpdateScheduler;

	static CCore* s_pInstance;
};

//////////////////////////////////////////////////////////////////////////
inline CCore* GetSchematycCoreImplPtr()
{
	return CCore::GetInstance();
}
//////////////////////////////////////////////////////////////////////////
inline CCore& GetSchematycCoreImpl()
{
	SCHEMATYC_CORE_ASSERT(CCore::GetInstance());
	return *CCore::GetInstance();
}
} // Schematyc

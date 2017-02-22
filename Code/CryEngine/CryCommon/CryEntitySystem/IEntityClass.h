// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CryEntitySystem/IEntityProxy.h>

struct IEntity;
struct IEntityProxy;
struct SEntitySpawnParams;
struct IEntityScript;
struct IScriptTable;

class ITexture;

struct SEditorClassInfo
{
	SEditorClassInfo() :
		sIcon(""),
		sHelper(""),
		sCategory("")
	{
	}

	const char* sIcon;
	const char* sHelper;
	const char* sCategory;
};

enum EEntityClassFlags
{
	ECLF_INVISIBLE                         = BIT(0),  //!< If set this class will not be visible in editor,and entity of this class cannot be placed manually in editor.
	ECLF_DEFAULT                           = BIT(1),  //!< If this is default entity class.
	ECLF_BBOX_SELECTION                    = BIT(2),  //!< If set entity of this class can be selected by bounding box in the editor 3D view.
	ECLF_DO_NOT_SPAWN_AS_STATIC            = BIT(3),  //!< If set the entity of this class stored as part of the level won't be assigned a static id on creation.
	ECLF_MODIFY_EXISTING                   = BIT(4),  //!< If set modify an existing class with the same name.
	ECLF_SEND_SCRIPT_EVENTS_FROM_FLOWGRAPH = BIT(5),  //!< If set send script events to entity from Flowgraph.
	ECLF_ENTITY_ARCHETYPE                  = BIT(6)   //!< If set this indicate the entity class is actually entity archetype.
};

struct IEntityClassRegistryListener;

//! Custom interface that can be used to enumerate/set an entity class' property.
//! Mostly used for comunication with the editor.
struct IEntityPropertyHandler
{
	virtual ~IEntityPropertyHandler(){}

	virtual void GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/ }

	//! Property info for entity class.
	enum EPropertyType
	{
		Bool,
		Int,
		Float,
		Vector,
		String,
		Entity,
		FolderBegin,
		FolderEnd
	};

	enum EPropertyFlags
	{
		ePropertyFlag_UIEnum   = BIT(0),
		ePropertyFlag_Unsorted = BIT(1),
	};

	struct SPropertyInfo
	{
		const char*   name;        //!< Name of the property.
		const char*   legacyName;  //!< Legacy name of the property, will be used if parsing 'name' failed. Saving will still be done with 'name'.
		EPropertyType type;        //!< Type of the property value.
		const char*   editType;    //!< Type of edit control to use.
		const char*   description; //!< Description of the property.
		uint32        flags;       //!< Property flags.

		const char*   defaultValue;//!< Default value of the property

		//! Limits.
		struct SLimits
		{
			float min;
			float max;
		} limits;
	};

	//! Refresh the class' properties.
	virtual void RefreshProperties() = 0;

	//! Load properties into the entity.
	virtual void LoadEntityXMLProperties(IEntity* entity, const XmlNodeRef& xml) = 0;

	//! Load archetype properties.
	virtual void LoadArchetypeXMLProperties(const char* archetypeName, const XmlNodeRef& xml) = 0;

	//! Init entity with archetype properties.
	virtual void InitArchetypeEntity(IEntity* entity, const char* archetypeName, const SEntitySpawnParams& spawnParams) = 0;

	//! Registers a new property for use in the Editor
	//! \return Index of the new property
	virtual int RegisterProperty(const SPropertyInfo& info) = 0;

	//! \return Number of properties for this entity.
	virtual int GetPropertyCount() const = 0;

	//! Retrieve information about properties of the entity.
	//! \param nIndex Index of the property to retrieve, must be in 0 to GetPropertyCount()-1 range.
	//! \return Specified event description in SPropertyInfo structure.
	virtual bool GetPropertyInfo(int index, SPropertyInfo& info) const = 0;

	//! Retrieve the default value of a property
	const char* GetDefaultProperty(int index) const
	{
		SPropertyInfo info;
		if (GetPropertyInfo(index, info))
		{
			return info.defaultValue;
		}

		return "";
	}

	//! Set a property in a entity of this class.
	//! \param index Index of the property to set, must be in 0 to GetPropertyCount()-1 range.
	virtual void SetProperty(IEntity* entity, int index, const char* value) = 0;

	//! Get a property in a entity of this class.
	//! \param index Index of the property to get, must be in 0 to GetPropertyCount()-1 range.
	virtual const char* GetProperty(IEntity* entity, int index) const = 0;

	//! Get script flags of this class.
	virtual uint32 GetScriptFlags() const = 0;

	//! Inform the implementation that properties have changed. Called at the end of a change block.
	virtual void PropertiesChanged(IEntity* entity) = 0;
};

// Entity registration helpers
template<typename T>
inline void RegisterEntityProperty(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc, float min, float max)
{
	// Cannot use STATIC_ASSERT here, clang catches it despite the template not being used
	CRY_ASSERT_MESSAGE(false, "Tried to register entity property of invalid type!");
}

template<typename T>
inline void RegisterEntityProperty(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	CRY_ASSERT_MESSAGE(false, "Tried to register entity property of invalid type!");
}

template<>
inline void RegisterEntityProperty<Vec3>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::Vector;
	newProperty.description = desc;
	newProperty.editType = "";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

template<>
inline void RegisterEntityProperty<ColorF>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::Vector;
	newProperty.description = desc;
	newProperty.editType = "color";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

template<>
inline void RegisterEntityProperty<float>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc, float min, float max)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::Float;
	newProperty.description = desc;
	newProperty.editType = "";
	newProperty.flags = 0;
	newProperty.limits.min = min;
	newProperty.limits.max = max;

	pHandler->RegisterProperty(newProperty);
}

template<>
inline void RegisterEntityProperty<int>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc, float min, float max)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::Int;
	newProperty.description = desc;
	newProperty.editType = "";
	newProperty.flags = 0;
	newProperty.limits.min = min;
	newProperty.limits.max = max;

	pHandler->RegisterProperty(newProperty);
}

template<>
inline void RegisterEntityProperty<bool>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::Bool;
	newProperty.description = desc;
	newProperty.editType = "b";
	newProperty.flags = 0;
	newProperty.limits.min = 0;
	newProperty.limits.max = 1;

	pHandler->RegisterProperty(newProperty);
}

template<>
inline void RegisterEntityProperty<string>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

template <>
inline void RegisterEntityProperty<ITexture>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "tex";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

template <>
inline void RegisterEntityProperty<IMaterial>(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "material";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyFlare(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "flare_";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyObject(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "obj";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioTrigger(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioTrigger";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioRtpc(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioRTPC";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioSwitch(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioSwitch";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioState(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioSwitchState";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioEnvironment(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioEnvironment";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyAudioPreloadRequest(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "audioPreloadRequest";
	newProperty.flags = 0;

	pHandler->RegisterProperty(newProperty);
}

inline void RegisterEntityPropertyEnum(IEntityPropertyHandler* pHandler, const char* name, const char* legacyName, const char* defaultVal, const char* desc, float min, float max)
{
	IEntityPropertyHandler::SPropertyInfo newProperty;

	newProperty.name = name;
	newProperty.legacyName = legacyName;
	newProperty.defaultValue = defaultVal;
	newProperty.type = IEntityPropertyHandler::String;
	newProperty.description = desc;
	newProperty.editType = "";
	newProperty.flags = IEntityPropertyHandler::ePropertyFlag_UIEnum | IEntityPropertyHandler::ePropertyFlag_Unsorted;
	newProperty.limits.min = min;
	newProperty.limits.max = max;

	pHandler->RegisterProperty(newProperty);
}

// Entity property group helper, any other property registered during the lifetime scope of this group will be registered inside
// Example use case:
// {
//		SEntityPropertyGroupHelper("MyGroup");

//		RegisterProperty(...)
// }
struct SEntityPropertyGroupHelper
{
	SEntityPropertyGroupHelper(IEntityPropertyHandler* pHandler, const char* name)
		: m_name(name)
		, m_pHandler(pHandler)
	{
		IEntityPropertyHandler::SPropertyInfo groupProperty;
		groupProperty.name = name;
		groupProperty.type = IEntityPropertyHandler::FolderBegin;

		m_pHandler->RegisterProperty(groupProperty);
	}

	~SEntityPropertyGroupHelper()
	{
		IEntityPropertyHandler::SPropertyInfo groupProperty;
		groupProperty.name = m_name;
		groupProperty.type = IEntityPropertyHandler::FolderEnd;

		m_pHandler->RegisterProperty(groupProperty);
	}

protected:
	const char*             m_name;

	IEntityPropertyHandler* m_pHandler;
};

//! Custom interface that can be used to reload an entity's script.
//! Only used by the editor, only.
struct IEntityScriptFileHandler
{
	virtual ~IEntityScriptFileHandler(){}

	//! Reloads the specified entity class' script.
	virtual void ReloadScriptFile() = 0;

	//! \return The class' script file name, if any.
	virtual const char* GetScriptFile() const = 0;

	virtual void        GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/ }
};

struct IEntityEventHandler
{
	virtual ~IEntityEventHandler(){}
	virtual void GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/ }

	enum EventValueType
	{
		Bool,
		Int,
		Float,
		Vector,
		Entity,
		String,
	};

	enum EventType
	{
		Input,
		Output,
	};

	//! Events info for this entity class.
	struct SEventInfo
	{
		const char*    name;        //!< Name of event.
		EventType      type;        //!< Input or Output event.
		EventValueType valueType;   //!< Type of event value.
		const char*    description; //!< Description of the event.
	};

	//! Refresh the class' event info.
	virtual void RefreshEvents() = 0;

	//! Load event info into the entity.
	virtual void LoadEntityXMLEvents(IEntity* entity, const XmlNodeRef& xml) = 0;

	//! \return Return Number of events for this entity.
	virtual int GetEventCount() const = 0;

	//! Retrieve information about events of the entity.
	//! \param nIndex Index of the event to retrieve, must be in 0 to GetEventCount()-1 range.
	//! \return Specified event description in SEventInfo structure.
	virtual bool GetEventInfo(int index, SEventInfo& info) const = 0;

	//! Send the specified event to the entity.
	//! \param entity The entity to send the event to.
	//! \param eventName Name of the event to send.
	virtual void SendEvent(IEntity* entity, const char* eventName) = 0;
};

struct IEntityAttribute;

DECLARE_SHARED_POINTERS(IEntityAttribute)

//! Derive from this interface to expose custom entity properties in the editor using the serialization framework.
struct IEntityAttribute
{
	virtual ~IEntityAttribute() {}

	virtual const char*         GetName() const = 0;
	virtual const char*         GetLabel() const = 0;
	virtual void                Serialize(Serialization::IArchive& archive) = 0;
	virtual IEntityAttributePtr Clone() const = 0;
	virtual void                OnAttributeChange(IEntity* pEntity) const {}
};

typedef DynArray<IEntityAttributePtr> TEntityAttributeArray;

//! Entity class defines what is this entity, what script it uses, what user proxy will be spawned with the entity, etc.
//! IEntityClass unique identify type of the entity, Multiple entities share the same entity class.
//! Two entities can be compared if they are of the same type by just comparing their IEntityClass pointers.
struct IEntityClass
{
	//! UserProxyCreateFunc is a function pointer type,.
	//! By calling this function EntitySystem can create user defined UserProxy class for an entity in SpawnEntity.
	//! For example:
	//! IEntityProxy* CreateUserProxy( IEntity *pEntity, SEntitySpawnParams &params )
	//! {
	//!     return new CUserProxy( pEntity,params );.
	//! }
	typedef IEntityProxyPtr (* UserProxyCreateFunc)(IEntity* pEntity, SEntitySpawnParams& params, void* pUserData);

	enum EventValueType
	{
		EVT_INT,
		EVT_FLOAT,
		EVT_BOOL,
		EVT_VECTOR,
		EVT_ENTITY,
		EVT_STRING
	};

	//! Events info for this entity class.
	struct SEventInfo
	{
		const char*    name;    //!< Name of event.
		EventValueType type;    //!< Type of event value.
		bool           bOutput; //!< Input or Output event.
	};

	// <interfuscator:shuffle>
	virtual ~IEntityClass(){}

	//! Destroy IEntityClass object, do not call directly, only EntityRegisty can destroy entity classes.
	virtual void Release() = 0;

	//! If this entity also uses a script, this is the name of the Lua table representing the entity behavior.
	//! \return Name of the entity class. Class name must be unique among all the entity classes.
	virtual const char* GetName() const = 0;

	//! \return Entity class flags.
	virtual uint32 GetFlags() const = 0;

	//! Set entity class flags.
	virtual void SetFlags(uint32 nFlags) = 0;

	//! Returns the Lua script file name.
	//! \return Lua Script filename, return empty string if entity does not use script.
	virtual const char* GetScriptFile() const = 0;

	//! Returns the IEntityScript interface assigned for this entity class.
	//! \return IEntityScript interface if this entity have script, or NULL if no script defined for this entity class.
	virtual IEntityScript* GetIEntityScript() const = 0;

	//! Returns the IScriptTable interface assigned for this entity class.
	//! \return IScriptTable interface if this entity have script, or NULL if no script defined for this entity class.
	virtual IScriptTable*             GetScriptTable() const = 0;

	virtual IEntityPropertyHandler*   GetPropertyHandler() const = 0;
	virtual IEntityEventHandler*      GetEventHandler() const = 0;
	virtual IEntityScriptFileHandler* GetScriptFileHandler() const = 0;

	virtual const SEditorClassInfo&   GetEditorClassInfo() const = 0;
	virtual void                      SetEditorClassInfo(const SEditorClassInfo& editorClassInfo) = 0;

	//! Loads the script.
	//! \note It is safe to call LoadScript multiple times, the script will only be loaded on the first call (unless bForceReload is specified).
	virtual bool LoadScript(bool bForceReload) = 0;

	//! Returns pointer to the user defined function to create UserProxy.
	//! \return Return UserProxyCreateFunc function pointer.
	virtual IEntityClass::UserProxyCreateFunc GetUserProxyCreateFunc() const = 0;

	//! Returns pointer to the user defined data to be passed when creating UserProxy.
	//! \return Return pointer to custom user proxy data.
	virtual void* GetUserProxyData() const = 0;

	//! \return Return Number of input and output events defined in the entity script.
	virtual int GetEventCount() = 0;

	//! Retrieve information about input/output event of the entity.
	//! \param nIndex Index of the event to retrieve, must be in 0 to GetEventCount()-1 range.
	//! \return Specified event description in SEventInfo structure.
	virtual IEntityClass::SEventInfo GetEventInfo(int nIndex) = 0;

	//! Find event by name.
	//! \param sEvent Name of the event.
	//! \param event Output parameter for event.
	//! \return True if event found and event parameter is initialized.
	virtual bool FindEventInfo(const char* sEvent, SEventInfo& event) = 0;

	//! Get attributes associated with this entity class.
	//! \return Array of entity attributes.
	virtual TEntityAttributeArray&       GetClassAttributes() = 0;
	virtual const TEntityAttributeArray& GetClassAttributes() const = 0;

	//! Get attributes associated with entities of this class.
	//! \return Array of entity attributes.
	virtual TEntityAttributeArray&       GetEntityAttributes() = 0;
	virtual const TEntityAttributeArray& GetEntityAttributes() const = 0;

	virtual void                         GetMemoryUsage(ICrySizer* pSizer) const = 0;
	// </interfuscator:shuffle>
};

//! This interface is the repository of the the various entity classes, it allows creation and modification of entities types.
//! There`s only one IEntityClassRegistry interface can exist per EntitySystem.
//! Every entity class that can be spawned must be registered in this interface.
//! \see IEntitySystem::GetClassRegistry
struct IEntityClassRegistry
{
	struct SEntityClassDesc
	{
		SEntityClassDesc()
			: flags(0)
			, sName("")
			, sScriptFile("")
			, pScriptTable(NULL)
			, editorClassInfo()
			, pUserProxyCreateFunc(NULL)
			, pUserProxyData(NULL)
			, pEventHandler(NULL)
			, pScriptFileHandler(NULL)
		{
		};

		int                               flags;
		const char*                       sName;
		const char*                       sScriptFile;
		IScriptTable*                     pScriptTable;

		SEditorClassInfo                  editorClassInfo;

		IEntityClass::UserProxyCreateFunc pUserProxyCreateFunc;
		void*                             pUserProxyData;

		IEntityEventHandler*              pEventHandler;
		IEntityScriptFileHandler*         pScriptFileHandler;

		TEntityAttributeArray             classAttributes;
		TEntityAttributeArray             entityAttributes;
	};

	virtual ~IEntityClassRegistry(){}

	//! Register a new entity class.
	//! \return true if successfully registered.
	virtual bool RegisterEntityClass(IEntityClass* pClass) = 0;

	//! Unregister an entity class.
	//! \return true if successfully unregistered.
	virtual bool UnregisterEntityClass(IEntityClass* pClass) = 0;

	//! Retrieves pointer to the IEntityClass interface by entity class name.
	//! \return Pointer to the IEntityClass interface, or NULL if class not found.
	virtual IEntityClass* FindClass(const char* sClassName) const = 0;

	//! Retrieves pointer to the IEntityClass interface for a default entity class.
	//! \return Pointer to the IEntityClass interface, It can never return NULL.
	virtual IEntityClass* GetDefaultClass() const = 0;

	//! Load all entity class descriptions from the provided xml file.
	//! \param szFilename Path to XML file containing entity class descriptions.
	virtual void LoadClasses(const char* szFilename, bool bOnlyNewClasses = false) = 0;

	//! Register standard entity class, if class id not specified (is zero), generate a new class id.
	//! \return Pointer to the new created and registered IEntityClass interface, or NULL if failed.
	virtual IEntityClass* RegisterStdClass(const SEntityClassDesc& entityClassDesc) = 0;

	//! Register a listener.
	virtual void RegisterListener(IEntityClassRegistryListener* pListener) = 0;

	//! Unregister a listener.
	virtual void UnregisterListener(IEntityClassRegistryListener* pListener) = 0;

	// Registry iterator.

	//! Move the entity class iterator to the begin of the registry.
	//! To iterate over all entity classes, e.g.:
	//! IEntityClass *pClass = NULL;
	//! for (pEntityRegistry->IteratorMoveFirst(); pClass = pEntityRegistry->IteratorNext();;)
	//! {
	//!     pClass
	//!     ...
	//! }
	virtual void IteratorMoveFirst() = 0;

	//! Get the next entity class in the registry.
	//! \return Return a pointer to the next IEntityClass interface, or NULL if is the end
	virtual IEntityClass* IteratorNext() = 0;

	//! Return the number of entity classes in the registry.
	//! \return Return a pointer to the next IEntityClass interface, or NULL if is the end
	virtual int GetClassCount() const = 0;
	// </interfuscator:shuffle>
};

enum EEntityClassRegistryEvent
{
	ECRE_CLASS_REGISTERED = 0,    //!< Sent when new entity class is registered.
	ECRE_CLASS_MODIFIED,          //!< Sent when new entity class is modified (see ECLF_MODIFY_EXISTING).
	ECRE_CLASS_UNREGISTERED       //!< Sent when new entity class is unregistered.
};

//! Use this interface to monitor changes within the entity class registry.
struct IEntityClassRegistryListener
{
	friend class CEntityClassRegistry;

public:

	inline IEntityClassRegistryListener()
		: m_pRegistry(NULL)
	{}

	virtual ~IEntityClassRegistryListener()
	{
		if (m_pRegistry)
		{
			m_pRegistry->UnregisterListener(this);
		}
	}

	virtual void OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass) = 0;

private:

	IEntityClassRegistry* m_pRegistry;
};

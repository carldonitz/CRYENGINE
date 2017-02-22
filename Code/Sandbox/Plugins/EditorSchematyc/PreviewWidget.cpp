// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PreviewWidget.h"

#include <QBoxLayout>
#include <QParentWndWidget.h>
#include <QPushButton>
#include <QSplitter>
#include <QViewport.h>
#include <QViewportConsumer.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySerialization/IArchiveHost.h>
#include <QAdvancedPropertyTree.h>

#include "Objects/DisplayContext.h"
#include "QViewportEvents.h"
#include "IEditor.h"

namespace Schematyc {

CPreviewSettingsWidget::CPreviewSettingsWidget(CPreviewWidget& previewWidget)
{
	m_pPropertyTree = new QAdvancedPropertyTree("Preview Settings");
	m_pPropertyTree->setExpandLevels(4);
	m_pPropertyTree->setValueColumnWidth(0.6f);
	m_pPropertyTree->setAutoRevert(false);
	m_pPropertyTree->setAggregateMouseEvents(false);
	m_pPropertyTree->setFullRowContainers(true);

	m_pPropertyTree->attach(Serialization::SStruct(previewWidget));

	addWidget(m_pPropertyTree);
}

void CPreviewSettingsWidget::showEvent(QShowEvent* pEvent)
{
	QScrollableBox::showEvent(pEvent);

	if (m_pPropertyTree)
		m_pPropertyTree->setSizeToContent(true);
}

CGizmoTranslateOp::CGizmoTranslateOp(ITransformManipulator& gizmo, IScriptComponentInstance& componentInstance)
	: m_gizmo(gizmo)
	, m_componentInstance(componentInstance)
{}

void CGizmoTranslateOp::OnInit()
{
	m_initTransform = m_componentInstance.GetTransform().ToMatrix34();
}

void CGizmoTranslateOp::OnMove(const Vec3& offset)
{
	CTransform transform = m_componentInstance.GetTransform();
	transform.SetTranslation(m_initTransform.GetTranslation() + offset);
	m_componentInstance.SetTransform(transform);

	GetSchematycCore().GetScriptRegistry().ElementModified(m_componentInstance);
}

void CGizmoTranslateOp::OnRelease()
{
	m_gizmo.SetCustomTransform(true, m_componentInstance.GetTransform().ToMatrix34());
}

CPreviewWidget::CPreviewWidget(QWidget* pParent)
	: QWidget(pParent)
{
	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pViewport = new QViewport(gEnv, this);

	m_viewportSettings.rendering.fps = false;
	m_viewportSettings.grid.showGrid = true;
	m_viewportSettings.grid.spacing = 1.0f;
	m_viewportSettings.camera.showViewportOrientation = false;
	m_viewportSettings.camera.moveSpeed = 0.2f;

	connect(m_pViewport, SIGNAL(SignalRender(const SRenderContext &)), SLOT(OnRender(const SRenderContext &)));

	GetSchematycCore().GetScriptRegistry().GetChangeSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CPreviewWidget::OnScriptRegistryChange), m_connectionScope);
}

CPreviewWidget::~CPreviewWidget()
{
	SetClass(nullptr);
}

void CPreviewWidget::InitLayout()
{
	QWidget::setLayout(m_pMainLayout);
	m_pMainLayout->setSpacing(1);
	m_pMainLayout->setMargin(0);
	m_pMainLayout->addWidget(m_pViewport);

	m_pViewport->SetSettings(m_viewportSettings);
	m_pViewport->SetSceneDimensions(Vec3(100.0f, 100.0f, 100.0f));

	ResetCamera(ms_defaultOrbitTarget, ms_defaultOrbitRadius);
	m_pViewport->SetOrbitMode(true);

	Update();
}

void CPreviewWidget::Update()
{
	if (m_pViewport)
	{
		m_pViewport->Update();
	}
}

void CPreviewWidget::SetClass(const IScriptClass* pScriptClass)
{
	auto releaseClass = [this]()
	{
		if (m_pObjectPreviewer)
		{
			m_pObjectPreviewer->DestroyObject(m_objectId);

			m_classGUID = SGUID();
			m_pObjectPreviewer = nullptr;
			m_objectId = ObjectId::Invalid;
		}
	};

	if (pScriptClass)
	{
		if (pScriptClass->GetGUID() != m_classGUID)
		{
			releaseClass();

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptClass->GetGUID());
			const IEnvClass* pEnvClass = pScriptView->GetEnvClass();
			CRY_ASSERT(pEnvClass);
			if (pEnvClass)
			{
				m_pObjectPreviewer = pEnvClass->GetPreviewer();
				if (m_pObjectPreviewer)
				{
					m_classGUID = pScriptClass->GetGUID();

					Reset();

					Vec3 orbitTarget = ms_defaultOrbitTarget;
					float orbitRadius = ms_defaultOrbitRadius;

					const Sphere objetcBounds = m_pObjectPreviewer->GetObjectBounds(m_objectId);
					if (objetcBounds.radius > 0.001f)
					{
						orbitTarget = objetcBounds.center;
						orbitRadius = std::max(objetcBounds.radius * 1.25f, orbitRadius);
					}

					ResetCamera(orbitTarget, orbitRadius);
				}
			}
		}
	}
	else
	{
		releaseClass();
	}
}

void CPreviewWidget::SetComponentInstance(const IScriptComponentInstance* pComponentInstance)
{
	auto releaseComponentInstance = [this]()
	{
		m_componentInstanceGUID = SGUID();

		if (m_pGizmo)
		{
			m_pViewport->GetGizmoManager()->RemoveManipulator(m_pGizmo);
			m_pGizmo = nullptr;
		}
	};

	if (pComponentInstance && pComponentInstance->HasTransform())
	{
		if (pComponentInstance->GetGUID() != m_componentInstanceGUID)
		{
			releaseComponentInstance();

			m_componentInstanceGUID = pComponentInstance->GetGUID();

			m_pGizmo = m_pViewport->GetGizmoManager()->AddManipulator(this);

			auto onBeginDrag = [this](IDisplayViewport*, ITransformManipulator*, Vec2i&, int)
			{
				if (GetIEditor()->GetEditMode() == eEditModeMove)
				{
					IScriptComponentInstance* pComponentInstance = DynamicCast<IScriptComponentInstance>(GetSchematycCore().GetScriptRegistry().GetElement(m_componentInstanceGUID));
					if (pComponentInstance)
					{
						m_pGizmoTransformOp.reset(new CGizmoTranslateOp(*m_pGizmo, *pComponentInstance));
						m_pGizmoTransformOp->OnInit();
					}
				}
				else
				{
					m_pGizmoTransformOp->OnRelease();
					m_pGizmoTransformOp.release();
				}
			};
			m_pGizmo->signalBeginDrag.Connect(onBeginDrag);

			auto onDrag = [this](IDisplayViewport*, ITransformManipulator*, Vec2i&, Vec3 offset, int)
			{
				if (m_pGizmoTransformOp)
				{
					m_pGizmoTransformOp->OnMove(offset);
				}
			};
			m_pGizmo->signalDragging.Connect(onDrag);

			auto onEndDrag = [this](IDisplayViewport*, ITransformManipulator*)
			{
				if (m_pGizmoTransformOp)
				{
					m_pGizmoTransformOp->OnRelease();
					m_pGizmoTransformOp.release();
				}
			};
			m_pGizmo->signalEndDrag.Connect(onEndDrag);
		}
	}
	else
	{
		releaseComponentInstance();
	}
}

void CPreviewWidget::Reset()
{
	if (m_pObjectPreviewer)
	{
		if (m_objectId != ObjectId::Invalid)
		{
			m_objectId = m_pObjectPreviewer->ResetObject(m_objectId);
		}
		else
		{
			m_objectId = m_pObjectPreviewer->CreateObject(m_classGUID);
		}
	}
}

void CPreviewWidget::LoadSettings(const XmlNodeRef& xml)
{
	Serialization::LoadXmlNode(m_viewportSettings, xml);
}

XmlNodeRef CPreviewWidget::SaveSettings(const char* szName)
{
	return Serialization::SaveXmlNode(m_viewportSettings, szName);
}

void CPreviewWidget::Serialize(Serialization::IArchive& archive)
{
	archive(m_viewportSettings, "viewportSettings", "Viewport");
	if (archive.isInput())
	{
		m_pViewport->SetSettings(m_viewportSettings);
	}

	IObject* pObject = GetSchematycCore().GetObject(m_objectId);
	if (pObject)
	{
		m_pObjectPreviewer->SerializeProperties(archive);

		std::set<IComponentPreviewer*> componentPreviewers;

		auto visitComponent = [&componentPreviewers](const CComponent& component) -> EVisitStatus
		{
			IComponentPreviewer* pComponentPreviewer = component.GetPreviewer();
			if (pComponentPreviewer)
			{
				componentPreviewers.insert(pComponentPreviewer);
			}
			return EVisitStatus::Continue;
		};
		pObject->VisitComponents(ObjectComponentConstVisitor::FromLambda(visitComponent));

		for (IComponentPreviewer* pComponentPreviewer : componentPreviewers)
		{
			pComponentPreviewer->SerializeProperties(archive); // #SchematycTODO : How do we avoid name collisions? Do we need to fully qualify component names?
		}
	}
}

bool CPreviewWidget::GetManipulatorMatrix(RefCoordSys coordSys, Matrix34& tm)
{
	return false;
}

void CPreviewWidget::GetManipulatorPosition(Vec3& position)
{
	if (!GUID::IsEmpty(m_componentInstanceGUID))
	{
		const IScriptComponentInstance* pComponentInstance = DynamicCast<const IScriptComponentInstance>(GetSchematycCore().GetScriptRegistry().GetElement(m_componentInstanceGUID));
		if (pComponentInstance)
		{
			position = pComponentInstance->GetTransform().GetTranslation();
		}
	}
	else
	{
		position = Vec3(ZERO);
	}
}

void CPreviewWidget::OnRender(const SRenderContext& context)
{
	IObject* pObject = GetSchematycCore().GetObject(m_objectId);
	if (pObject)
	{
		m_pObjectPreviewer->RenderObject(*pObject, *context.renderParams, *context.passInfo);

		auto visitComponent = [pObject, &context](const CComponent& component) -> EVisitStatus
		{
			const IComponentPreviewer* pComponentPreviewer = component.GetPreviewer();
			if (pComponentPreviewer)
			{
				pComponentPreviewer->Render(*pObject, component, *context.renderParams, *context.passInfo);
			}
			return EVisitStatus::Continue;
		};
		pObject->VisitComponents(ObjectComponentConstVisitor::FromLambda(visitComponent));
	}
}

void CPreviewWidget::ResetCamera(const Vec3& orbitTarget, float orbitRadius) const
{
	SViewportState viewportState = m_pViewport->GetState();
	viewportState.cameraTarget.t = Vec3(0.0f, -orbitRadius, orbitRadius * 0.5f);
	viewportState.orbitTarget = orbitTarget;
	viewportState.orbitRadius = orbitRadius;

	m_pViewport->SetState(viewportState);
	m_pViewport->LookAt(orbitTarget, orbitRadius * 2.0f, true);
}

void CPreviewWidget::OnScriptRegistryChange(const SScriptRegistryChange& change)
{
	Reset();
}

const Vec3 CPreviewWidget::ms_defaultOrbitTarget = Vec3(0.0f, 0.0f, 1.0f);
const float CPreviewWidget::ms_defaultOrbitRadius = 2.0f;
} // Schematyc

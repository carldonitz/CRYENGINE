// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "QScrollableBox.h"

#include <QWidget>
#include <QObject.h>
#include <QViewportSettings.h>
#include <QWidget.h>
#include "Gizmos/ITransformManipulator.h"

// Forward declare interfaces.
struct SRenderContext;
struct SMouseEvent;
// Forward declare classes.
class QBoxLayout;
class QParentWndWidget;
class QAdvancedPropertyTree;
class QPushButton;
class QSplitter;
class QViewport;
enum EMouseEvent;
struct IScriptComponentInstance;

namespace Schematyc {

struct IScriptClass;
class CPreviewWidget;

class CPreviewSettingsWidget : public QScrollableBox
{
	Q_OBJECT

public:
	CPreviewSettingsWidget(CPreviewWidget& previewWidget);

protected:
	void showEvent(QShowEvent* pEvent);

private:
	QAdvancedPropertyTree* m_pPropertyTree;
};

// #SchematycTODO : Use generalized transform system.
struct IGizmoTransformOp
{
public:

	virtual ~IGizmoTransformOp() {};

	virtual void OnInit() = 0;
	virtual void OnMove(const Vec3& offset) = 0;
	virtual void OnRelease() = 0;
};

class CGizmoTranslateOp : public IGizmoTransformOp
{
public:

	CGizmoTranslateOp(ITransformManipulator& gizmo, IScriptComponentInstance& componentInstance);

	virtual void OnInit() override;
	virtual void OnMove(const Vec3& offset) override;
	virtual void OnRelease() override;

private:

	ITransformManipulator&    m_gizmo;
	IScriptComponentInstance& m_componentInstance;
	Matrix34                  m_initTransform;
};

class CPreviewWidget : public QWidget, public ITransformManipulatorOwner
{
	Q_OBJECT

public:

	CPreviewWidget(QWidget* pParent);
	~CPreviewWidget();

	void       InitLayout();
	void       Update();

	void       SetClass(const IScriptClass* pClass);
	void       SetComponentInstance(const IScriptComponentInstance* pComponentInstance);
	void       Reset();

	void       LoadSettings(const XmlNodeRef& xml);
	XmlNodeRef SaveSettings(const char* szName);

	void       Serialize(Serialization::IArchive& archive);

	// ITransformManipulatorOwner
	virtual bool GetManipulatorMatrix(RefCoordSys coordSys, Matrix34& tm) override;
	virtual void GetManipulatorPosition(Vec3& position) override;
	// ~ITransformManipulatorOwner

protected slots:
	void OnRender(const SRenderContext& context);

private:

	void ResetCamera(const Vec3& orbitTarget, float orbitRadius) const;
	void OnScriptRegistryChange(const SScriptRegistryChange& change);

private:

	QBoxLayout*                        m_pMainLayout = nullptr;
	QViewport*                         m_pViewport = nullptr;

	SViewportSettings                  m_viewportSettings;

	SGUID                              m_classGUID;
	SGUID                              m_componentInstanceGUID;

	IObjectPreviewer*                  m_pObjectPreviewer = nullptr;
	ObjectId                           m_objectId;

	ITransformManipulator*             m_pGizmo = nullptr;
	std::unique_ptr<IGizmoTransformOp> m_pGizmoTransformOp;

	CConnectionScope                   m_connectionScope;

	static const Vec3                  ms_defaultOrbitTarget;
	static const float                 ms_defaultOrbitRadius;
};
} // Schematyc

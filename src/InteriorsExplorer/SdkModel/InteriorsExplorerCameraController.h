// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "Types.h"
#include "IAppCamera.h"
#include "GlobeCamera.h"
#include "ICallback.h"
#include "Interiors.h"
#include "ITouchController.h"
#include "VectorMath.h"
#include "GlobeCameraTouchSettings.h"
#include "Rendering.h"

namespace ExampleApp
{
    namespace InteriorsExplorer
    {
        namespace SdkModel
        {
            class InteriorsExplorerCameraController : public AppCamera::SdkModel::IAppCamera, private Eegeo::NonCopyable
            {
            private:
                
                void UpdateCameraView(const Eegeo::Resources::Interiors::InteriorsModel& interiorModel,
                                      const Eegeo::Resources::Interiors::InteriorsFloorModel& currentFloorModel);
                void HandleInteractionModelChanged();
                Eegeo::dv3 CalculateInterestPoint(const Eegeo::Resources::Interiors::InteriorsModel& interiorModel,
                                                  float centerHeightAboveGround,
                                                  float expandedParam,
                                                  bool shouldCenterOnFloor);
                
                bool m_cameraTouchEnabled;
                const Eegeo::Resources::Interiors::IInteriorController& m_interiorController;
                Eegeo::Resources::Interiors::InteriorSelectionModel& m_interiorSelectionModel;
                Eegeo::Resources::Interiors::DefaultInteriorAnimationController& m_interiorAnimationController;
                Eegeo::Resources::Interiors::InteriorInteractionModel& m_interiorInteractionModel;
                Eegeo::Camera::GlobeCamera::GlobeCameraTouchController& m_globeCameraTouchController;
                Eegeo::Camera::GlobeCamera::GlobeCameraController& m_globeCameraController;
                const Eegeo::Rendering::EnvironmentFlatteningService& m_environmentFlatteningService;
                
                Eegeo::Helpers::TCallback0<InteriorsExplorerCameraController> m_interactionModelChangedCallback;
                
                const bool m_interiorsAffectedByFlattening;
                bool m_applyRestrictions;
                float m_cameraInterestAltitude;
                bool m_applyFloorOffset;
                
            public:
                InteriorsExplorerCameraController(const Eegeo::Resources::Interiors::IInteriorController& interiorController,
                                                  Eegeo::Resources::Interiors::InteriorSelectionModel& interiorSelectionModel,
                                                  // todo dependency on concrete DefaultInteriorAnimationController - should be IInteriorViewAnimator or not at all
                                                  Eegeo::Resources::Interiors::DefaultInteriorAnimationController& interiorAnimationController,
                                                  Eegeo::Resources::Interiors::InteriorInteractionModel& interiorInteractionModel,
                                                  const Eegeo::Rendering::EnvironmentFlatteningService& environmentFlatteningService,
                                                  Eegeo::Camera::GlobeCamera::GlobeCameraTouchController& m_globeCameraTouchController,
                                                  Eegeo::Camera::GlobeCamera::GlobeCameraController& globeCameraController,
                                                  const bool interiorsAffectedByFlattening);
                
                ~InteriorsExplorerCameraController();
                
                const Eegeo::Camera::CameraState GetCameraState() const;
                const Eegeo::Camera::RenderCamera GetRenderCamera() const;
                Eegeo::dv3 ComputeNonFlattenedCameraPosition() const;
                Eegeo::ITouchController& GetTouchController() const;
                Eegeo::Camera::GlobeCamera::GlobeCameraController& GetGlobeCameraController();
                
                void Update(float dt);
                
                void SetInterestLocation(const Eegeo::dv3& interestPointEcef);
                const Eegeo::dv3& GetInterestLocation() const;
                
                void SetDistanceToInterest(float distanceMeters);
                const float GetDistanceToInterest() const;
                
                void SetHeading(float headingDegrees);
                float GetHeadingDegrees() const;
                
                void SetTilt(float tiltDegrees);
                void SetApplyRestrictions(bool applyRestrictions);
                void SetApplyFloorOffset(bool applyFloorOffset);
                float GetCameraInterestAltitude() const;
                void SetCameraInterestAltitude(float cameraInterestAltitude);
                float GetFloorOffsetHeight() const;
            };
        }
    }
}
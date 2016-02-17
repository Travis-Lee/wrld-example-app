// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "MyPinCreation.h"
#include "IPoiRingTouchController.h"
#include "AppInterface.h"
#include "IRayPicker.h"
#include "Camera.h"
#include "GpsGlobeCameraController.h"
#include "VectorMathDecl.h"
#include "AppModes.h"
#include "Interiors.h"

namespace ExampleApp
{
    namespace MyPinCreation
    {
        namespace PoiRing
        {
            namespace SdkModel
            {
                class PoiRingTouchController : public IPoiRingTouchController
                {
                public:
                    PoiRingTouchController(MyPinCreation::SdkModel::IMyPinCreationModel& myPinCreationModel,
                                           Eegeo::Collision::IRayPicker& rayPicker,
                                           const IPoiRingController& poiRingController,
                                           ExampleApp::AppModes::SdkModel::IAppModeModel& appModeModel,
                                           const Eegeo::Resources::Interiors::IInteriorController& interiorController,
                                           Eegeo::Resources::Interiors::InteriorInteractionModel& interiorInteractionModel);

                    bool HandleTouchDown(const AppInterface::TouchData& data, const Eegeo::Camera::RenderCamera& renderCamera, const Eegeo::dv3& nonFlattenedCameraPosition);
                    bool HandleTouchUp(const AppInterface::TouchData& data);
                    bool HandleTouchMove(const AppInterface::TouchData& data, const Eegeo::Camera::RenderCamera& renderCamera, const Eegeo::dv3& nonFlattenedCameraPosition);

                    bool IsDragging() const;

                private:
                    MyPinCreation::SdkModel::IMyPinCreationModel& m_myPinCreationModel;
                    Eegeo::Collision::IRayPicker& m_rayPicker;
                    const IPoiRingController& m_poiRingController;
                    
                    ExampleApp::AppModes::SdkModel::IAppModeModel& m_appModeModel;
                    const Eegeo::Resources::Interiors::IInteriorController& m_interiorController;
                    Eegeo::Resources::Interiors::InteriorInteractionModel& m_interiorInteractionModel;

                    bool m_isDragging;
                    Eegeo::dv3 m_dragOffset;
                    
                    float m_initialCameraAltitiude;

                    bool PerformRayPick(const Eegeo::dv3& rayOrigin,
                                        Eegeo::dv3& rayDirection,
                                        Eegeo::dv3& out_rayIntersectionPoint,
                                        double& out_intersectionParam,
                                        float& out_terrainHeight,
                                        float& out_heightAboveTerrain);
                };
            }
        }
    }
}

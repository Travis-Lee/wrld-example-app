// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "IGpsMarkerController.h"
#include "GpsMarker.h"
#include "BidirectionalBus.h"
#include "ICallback.h"
#include "ModalityChangedMessage.h"
#include "Rendering.h"
#include "IVisualMapService.h"

namespace ExampleApp
{
    namespace GpsMarker
    {
        namespace SdkModel
        {
            class GpsMarkerController : public IGpsMarkerController
            {
            public:
                
                GpsMarkerController(GpsMarkerModel& model,
                                    GpsMarkerView& view,
                                    Eegeo::Rendering::EnvironmentFlatteningService& environmentFlatteningService,
                                    VisualMap::SdkModel::IVisualMapService& visualMapService,
                                    ExampleAppMessaging::TMessageBus& messageBus);
                ~GpsMarkerController();
                
                void Update(float dt, const Eegeo::Camera::RenderCamera& renderCamera);
                
            private:
                
                const static float DefaultUpdatePeriod;
                float m_updateTime;
                int m_visibilityCount;
                
                GpsMarkerModel& m_model;
                GpsMarkerView& m_view;
                
                Eegeo::Rendering::EnvironmentFlatteningService& m_environmentFlatteningService;
                VisualMap::SdkModel::IVisualMapService& m_visualMapService;
                
                ExampleAppMessaging::TMessageBus& m_messageBus;
                Eegeo::Helpers::TCallback1<GpsMarkerController, const Modality::ModalityChangedMessage&> m_modalityChangedHandlerBinding;
                Eegeo::Helpers::TCallback1<GpsMarkerController, const GpsMarkerVisibilityMessage&> m_visibilityChangedHandlerBinding;
                Eegeo::Helpers::TCallback1<GpsMarkerController, const InteriorsExplorer::InteriorsExplorerStateChangedMessage&> m_interiorsExplorerStateChangedCallback;
                
                void OnModalityChangedMessage(const Modality::ModalityChangedMessage& message);
                void OnVisbilityChangedMessage(const GpsMarkerVisibilityMessage& message);
                void OnInteriorsExplorerStateChangedMessage(const InteriorsExplorer::InteriorsExplorerStateChangedMessage& message);
                
                void CreateModelViewProjectionMatrix(Eegeo::m44& out_modelViewProjection,
                                                     const Eegeo::dv3& location,
                                                     const float heading,
                                                     const Eegeo::m44& transformMatrix,
                                                     const Eegeo::Camera::RenderCamera& renderCamera);
                const std::string GetCurrentVisualMapTime();
            };
        }
    }
}
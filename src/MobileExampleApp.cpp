// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#include "MobileExampleApp.h"
#include "CameraHelpers.h"
#include "StreamingVolumeController.h"
#include "IWatermarkViewModel.h"
#include "SettingsMenu.h"
#include "IFlattenButtonViewModel.h"
#include "ICompassViewModel.h"
#include "NavigationService.h"
#include "IInitialExperienceController.h"
#include "IMyPinCreationInitiationViewModel.h"
#include "SearchMenuOptions.h"
#include "WorldPinsPlatformServices.h"
#include "IWorldPinsService.h"
#include "IWorldPinsScaleController.h"
#include "IWorldPinsFloorHeightController.h"
#include "PinController.h"
#include "PinsModule.h"
#include "InteriorVisibilityUpdater.h"
#include "InteriorWorldPinController.h"
#include "IInteriorsEntitiesPinsController.h"
#include "IGpsMarkerController.h"
#include "LoadingScreenCompleteMessage.h"
#include "IInteriorsNavigationService.h"
#include "ISearchRefreshService.h"

namespace ExampleApp
{
    namespace
    {
/*        void AddLocalMaterials(
                Eegeo::Helpers::IFileIO& fileIO,
                Eegeo::Resources::Interiors::IInteriorsTextureResourceService& interiorsTextureResourceService,
                Eegeo::Resources::Interiors::Materials::IInteriorsMaterialDtoRepository& interiorsMaterialDtoRepository)
        {
            std::fstream stream;
            size_t size;

            if(fileIO.OpenFile(stream, size, "Interiors/Custom/custom_material_definitions.json"))
            {
                std::string materialsJson((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

                rapidjson::Document document;
                if (document.Parse<0>(materialsJson.c_str()).HasParseError())
                {
                    Eegeo_ASSERT(false, "Error parsing local materials JSON file.\n");
                }

                for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin();
                     iter != document.MemberEnd();
                     ++iter)
                {
                    std::string interiorName(iter->name.GetString());
                    const rapidjson::Value& materials(iter->value);

                    size_t materialsCount(materials.Size());
                    for(size_t i = 0; i < materialsCount; ++ i)
                    {
                        const rapidjson::Value& materialJson(materials[static_cast<int>(i)]);
                        Eegeo::Resources::Interiors::Materials::InteriorsMaterialDto dto(Eegeo::Resources::Interiors::Materials::ParseMaterial(materialJson));
                        Eegeo_ASSERT(!interiorsMaterialDtoRepository.Contains(interiorName, dto.materialName));
                        interiorsMaterialDtoRepository.Add(interiorName, dto);

                        const bool localTexture = true;
                        Eegeo::Resources::Interiors::Materials::CreateAndRegisterTextures(dto, interiorsTextureResourceService, localTexture);
                        Eegeo::Resources::Interiors::Materials::CreateAndRegisterCubeMapTextures(dto, interiorsTextureResourceService, localTexture);
                    }
                }
            }
        }*/
        void SetInitialCameraPosition(ExampleApp::ApplicationConfig::ApplicationConfiguration& appConfig,
                                      Eegeo::Camera::GlobeCamera::GpsGlobeCameraController& gpsCameraController)
        {
            Eegeo::Space::LatLongAltitude location = appConfig.InterestLocation();
            float cameraControllerOrientationDegrees = appConfig.OrientationDegrees();
            float cameraControllerDistanceFromInterestPointMeters = appConfig.DistanceToInterestMetres();
            
            Eegeo::Space::EcefTangentBasis cameraInterestBasis;
            Eegeo::Camera::CameraHelpers::EcefTangentBasisFromPointAndHeading(location.ToECEF(), cameraControllerOrientationDegrees, cameraInterestBasis);
            
            gpsCameraController.SetView(cameraInterestBasis, cameraControllerDistanceFromInterestPointMeters);
        }
        
        void UpdateStreamingVolume(const Eegeo::Camera::CameraState& cameraState,
                                   const Eegeo::Camera::RenderCamera& renderCamera,
                                   Eegeo::Streaming::CameraFrustumStreamingVolume& streamingVolume)
        {
            std::vector<Eegeo::Geometry::Plane> frustumPlanes(Eegeo::Geometry::Frustum::PLANES_COUNT);
            BuildFrustumPlanesFromViewProjection(frustumPlanes, renderCamera.GetViewProjectionMatrix());
            const double d = renderCamera.GetAltitude() * Eegeo::Streaming::StreamingVolumeController::CAMERA_ALTITUDE_TO_FAR_PLANE_DISTANCE_MULTIPLIER;
            const double cameraFarPlaneD = fmin(fmax(d, Eegeo::Streaming::StreamingVolumeController::MIN_STREAMING_FAR_PLANE_DISTANCE),
                                                frustumPlanes[Eegeo::Geometry::Frustum::PLANE_FAR].d);
            frustumPlanes[Eegeo::Geometry::Frustum::PLANE_FAR].d = static_cast<float>(cameraFarPlaneD);
            
            streamingVolume.updateStreamingVolume(renderCamera.GetEcefLocation(), frustumPlanes, renderCamera.GetFOV());
            streamingVolume.ResetVolume(cameraState.InterestPointEcef());
        }
        
        bool UpdateLoadingScreen(float dt, Eegeo::EegeoWorld& eegeoWorld, Eegeo::Rendering::LoadingScreen& loadingScreen)
        {
            if (!eegeoWorld.Initialising() && !loadingScreen.IsDismissed())
            {
                loadingScreen.Dismiss();
            }
             
            loadingScreen.SetProgress(eegeoWorld.GetInitialisationProgress());
            loadingScreen.Update(dt);
             
            if (!loadingScreen.IsVisible())
            {
                 return true;
            }
            return false;
        }
    }
    
    MobileExampleApp::MobileExampleApp(const std::shared_ptr<Eegeo::EegeoWorld>& world,
                                       const std::shared_ptr<AppCamera::SdkModel::IAppCameraController>& appCameraController,
                                       const std::shared_ptr<AppModes::SdkModel::IAppModeStatesFactory>& appStatesFactory,
                                       const std::shared_ptr<AppModes::SdkModel::IAppModeModel>& appModeModel,
                                       const std::shared_ptr<ExampleApp::ApplicationConfig::ApplicationConfiguration>& appConfig,
                                       const std::shared_ptr<Eegeo::Camera::GlobeCamera::GpsGlobeCameraController>& gpsCameraController,
                                       const std::shared_ptr<Eegeo::Streaming::CameraFrustumStreamingVolume>& cameraFrustumStreamingVolume,
                                       const std::shared_ptr<Eegeo::Rendering::ScreenProperties>& screenProperties,
                                       const std::shared_ptr<Eegeo::Rendering::LoadingScreen>& loadingScreen,
                                       const std::shared_ptr<InitialExperience::SdkModel::IInitialExperienceModel>& initialExperienceModel,
                                       const std::shared_ptr<Compass::SdkModel::ICompassUpdateController>& compassUpdateController,
                                       const std::shared_ptr<Eegeo::Location::NavigationService>& navigationService,
                                       const std::shared_ptr<InitialExperience::SdkModel::IInitialExperienceController>& initialExperienceController,
                                       const std::shared_ptr<MyPinCreation::PoiRing::SdkModel::IPoiRingController>& poiRingController,
                                       const std::shared_ptr<CameraTransitions::SdkModel::ICameraTransitionController>& cameraTransitionService,
                                       const std::shared_ptr<WorldPins::SdkModel::WorldPinsPlatformServices>& worldPinsPlatformServices,
                                       const std::shared_ptr<WorldPins::SdkModel::IWorldPinsService>& worldPinsService,
                                       const std::shared_ptr<WorldPins::SdkModel::IWorldPinsScaleController>& worldPinsScaleController,
                                       const std::shared_ptr<WorldPins::SdkModel::IWorldPinsFloorHeightController>& worldPinsFloorHeightController,
                                       const std::shared_ptr<WorldPins::SdkModel::IWorldPinsInFocusController>& worldPinsInFocusController,
                                       const std::shared_ptr<InteriorsExplorer::SdkModel::InteriorVisibilityUpdater>& interiorsVisibilityUpdater,
                                       const std::shared_ptr<InteriorsExplorer::SdkModel::InteriorWorldPinController>& interiorWorldPinController,
                                       const std::shared_ptr<WorldPins::SdkModel::InteriorPinsPlatformServices>& interiorPinsPlatformServices,
                                       const std::shared_ptr<InteriorsEntitiesPins::SdkModel::IInteriorsEntitiesPinsController>& interiorsEntitiesPinsController,
                                       const std::shared_ptr<GpsMarker::SdkModel::IGpsMarkerController>& gpsMarkerController,
                                       const std::shared_ptr<Search::SdkModel::ISearchRefreshService>& searchRefreshService,
                                       const std::shared_ptr<InteriorsNavigation::SdkModel::IInteriorsNavigationService>& interiorsNavigationService,
                                       const std::shared_ptr<ExampleAppMessaging::TMessageBus>& messageBus)
    : m_world(world)
    , m_cameraController(appCameraController)
    , m_gpsCameraController(gpsCameraController)
    , m_appStatesFactory(appStatesFactory)
    , m_appModeModel(appModeModel)
    , m_appConfig(appConfig)
    , m_streamingVolume(cameraFrustumStreamingVolume)
    , m_screenProperties(screenProperties)
    , m_loadingScreen(loadingScreen)
    , m_initialExperienceModel(initialExperienceModel)
    , m_worldPinsInFocusController(worldPinsInFocusController)
    , m_compassUpdateController(compassUpdateController)
    , m_navigationService(navigationService)
    , m_initialExperienceController(initialExperienceController)
    , m_poiRingController(poiRingController)
    , m_cameraTransitionService(cameraTransitionService)
    , m_worldPinsPlatformServices(worldPinsPlatformServices)
    , m_worldPinsService(worldPinsService)
    , m_worldPinsScaleController(worldPinsScaleController)
    , m_worldPinsFloorHeightController(worldPinsFloorHeightController)
    , m_interiorsVisibilityUpdater(interiorsVisibilityUpdater)
    , m_interiorWorldPinController(interiorWorldPinController)
    , m_interiorPinsPlatformServices(interiorPinsPlatformServices)
    , m_interiorsEntitiesPinsController(interiorsEntitiesPinsController)
    , m_gpsMarkerController(gpsMarkerController)
    , m_searchRefreshService(searchRefreshService)
    , m_interiorsNavigationService(interiorsNavigationService)
    , m_messageBus(messageBus)
    {
        Eegeo_ASSERT(m_world != nullptr);
        Eegeo_ASSERT(m_cameraController != nullptr);
        Eegeo_ASSERT(m_gpsCameraController != nullptr);
        Eegeo_ASSERT(m_appStatesFactory != nullptr);
        Eegeo_ASSERT(m_appModeModel != nullptr);
        Eegeo_ASSERT(m_streamingVolume != nullptr);
        Eegeo_ASSERT(m_screenProperties != nullptr);
        Eegeo_ASSERT(m_loadingScreen != nullptr);
        Eegeo_ASSERT(m_initialExperienceModel != nullptr);
        Eegeo_ASSERT(m_worldPinsInFocusController != nullptr);
        Eegeo_ASSERT(m_compassUpdateController != nullptr);
        Eegeo_ASSERT(m_navigationService != nullptr);
        Eegeo_ASSERT(m_initialExperienceModel != nullptr);
        Eegeo_ASSERT(m_initialExperienceController != nullptr);
        Eegeo_ASSERT(m_poiRingController != nullptr);
        Eegeo_ASSERT(m_cameraTransitionService != nullptr);
        Eegeo_ASSERT(m_worldPinsPlatformServices != nullptr);
        Eegeo_ASSERT(m_worldPinsService != nullptr);
        Eegeo_ASSERT(m_worldPinsScaleController != nullptr);
        Eegeo_ASSERT(m_worldPinsFloorHeightController != nullptr);
        Eegeo_ASSERT(m_interiorsVisibilityUpdater != nullptr);
        Eegeo_ASSERT(m_interiorWorldPinController != nullptr);
        Eegeo_ASSERT(m_interiorPinsPlatformServices != nullptr);
        Eegeo_ASSERT(m_interiorsEntitiesPinsController != nullptr);
        Eegeo_ASSERT(m_gpsMarkerController != nullptr);
        Eegeo_ASSERT(m_interiorsNavigationService != nullptr);
        Eegeo_ASSERT(m_messageBus != nullptr);
        Eegeo_ASSERT(m_searchRefreshService != nullptr);
        //AddLocalMaterials(m_platformAbstractions.GetFileIO(),
        //                  m_pWorld->GetMapModule().GetInteriorsMaterialsModule().GetInteriorsTextureResourceService(),
        //                  m_pWorld->GetMapModule().GetInteriorsMaterialsModule().GetInteriorsMaterialDtoRepository());
        SetInitialCameraPosition(*appConfig, *gpsCameraController);
        m_appModeModel->InitialiseStateMachine(m_appStatesFactory->CreateStateMachineStates());
    }
    
    MobileExampleApp::~MobileExampleApp()
    {
        OnPause();
    }

    void MobileExampleApp::OnPause()
    {
        m_world->OnPause();
    }

    void MobileExampleApp::OnResume()
    {
        m_world->OnResume();
    }

    void MobileExampleApp::Update(float dt)
    {
        m_world->EarlyUpdate(dt);
        m_cameraTransitionService->Update(dt);
        m_cameraController->Update(dt);
        m_appModeModel->Update(dt);
        m_interiorsVisibilityUpdater->Update(dt);
        m_interiorWorldPinController->Update(dt);
        
        Eegeo::Camera::RenderCamera renderCamera = m_cameraController->GetRenderCamera();
        Eegeo::Camera::CameraState cameraState = m_cameraController->GetCameraState();

        UpdateStreamingVolume(cameraState, renderCamera, *m_streamingVolume);
        Eegeo::dv3 ecefInterestPoint(cameraState.InterestPointEcef());
        Eegeo::EegeoUpdateParameters updateParameters(dt,
                                                      cameraState.LocationEcef(),
                                                      cameraState.InterestPointEcef(),
                                                      cameraState.ViewMatrix(),
                                                      cameraState.ProjectionMatrix(),
                                                      *m_streamingVolume,
                                                      *m_screenProperties);

        m_poiRingController->Update(dt, renderCamera, ecefInterestPoint);
        
        m_world->Update(updateParameters);
        m_worldPinsPlatformServices->GetPinsModule()->Update(dt, renderCamera);
        m_worldPinsPlatformServices->GetPinController()->Update(dt, renderCamera);
        m_interiorPinsPlatformServices->GetPinsModule()->Update(dt, renderCamera);
        m_interiorPinsPlatformServices->GetPinController()->Update(dt, renderCamera);
        m_searchRefreshService->TryRefreshSearch(dt, ecefInterestPoint, cameraState.LocationEcef());
        
        if (!m_world->Initialising() || (m_loadingScreen == nullptr && m_world->Initialising()))
        {
            m_worldPinsService->Update(dt);
            m_worldPinsScaleController->Update(dt, renderCamera);
            m_worldPinsFloorHeightController->Update(dt);
            m_interiorsEntitiesPinsController->Update(dt);
            m_compassUpdateController->Update(dt);
            m_gpsMarkerController->Update(dt, renderCamera);
            
            if(!m_initialExperienceModel->HasCompletedInitialExperience() && m_loadingScreen == nullptr)
            {
                m_initialExperienceController->Update(dt);
            }
        }
        
        m_navigationService->Update(dt);
        m_interiorsNavigationService->Update(dt);
       
        /*Eegeo::EegeoWorld& eegeoWorld(World());
        
        m_pCurrentTouchController = &m_pAppCameraModule->GetController().GetTouchController();

        eegeoWorld.EarlyUpdate(dt);

        m_pCameraTransitionService->Update(dt);
        m_pAppCameraModule->GetController().Update(dt);
        
        m_pAppModeModel->Update(dt);
        
        m_pInteriorsExplorerModule->Update(dt);
        
        Eegeo::Camera::RenderCamera renderCamera = m_pAppCameraModule->GetController().GetRenderCamera();
        Eegeo::Camera::CameraState cameraState = m_pAppCameraModule->GetController().GetCameraState();
        
        Eegeo::dv3 ecefInterestPoint(cameraState.InterestPointEcef());

        Eegeo::EegeoUpdateParameters updateParameters(dt,
                cameraState.LocationEcef(),
                cameraState.InterestPointEcef(),
                cameraState.ViewMatrix(),
                cameraState.ProjectionMatrix(),
                GetUpdatedStreamingVolume(cameraState, renderCamera),
                m_screenProperties);

        eegeoWorld.Update(updateParameters);

        m_pSearchModule->GetSearchRefreshService().TryRefreshSearch(dt, ecefInterestPoint, cameraState.LocationEcef());

        m_pPinsModule->GetController().Update(dt, renderCamera);
        
        if(!eegeoWorld.Initialising() || (m_pLoadingScreen == NULL && eegeoWorld.Initialising()))
        {
            WorldPinsModule().GetWorldPinsService().Update(dt);
            WorldPinsModule().GetWorldPinsScaleController().Update(dt, renderCamera);
            WorldPinsModule().GetWorldPinsFloorHeightController().Update(dt);
            
            CompassModule().GetCompassUpdateController().Update(dt);
            CompassModule().GetCompassUpdateController().Update(dt);
            m_pGpsMarkerModule->GetGpsMarkerController().Update(dt, renderCamera);
            
            if (m_interiorsEnabled)
            {
                Eegeo_ASSERT(m_pInteriorsEntitiesPinsModule != NULL);
                
                m_pInteriorsEntitiesPinsModule->GetPinsModule().Update(dt, renderCamera);
                m_pInteriorsEntitiesPinsModule->GetInteriorsEntitiesPinsController().Update(dt);
            }
            
            InitialExperience::SdkModel::IInitialExperienceModel& initialExperienceModel = m_initialExperienceModule.GetInitialExperienceModel();
            if(!initialExperienceModel.HasCompletedInitialExperience() && IsLoadingScreenComplete())
            {
                InitialExperience::SdkModel::IInitialExperienceController& initialExperienceController = m_initialExperienceModule.GetInitialExperienceController();
                initialExperienceController.Update(dt);
            }
  
            if (!m_setMetricsLocation)
            {
                Eegeo::dv3 gpsLocation;
                if(m_pNavigationService->TryGetGpsLocationOnTerrain(gpsLocation))
                {
                    Eegeo::Space::LatLong ll = Eegeo::Space::LatLong::FromECEF(gpsLocation);
                    m_metricsService.SetPosition(ll.GetLatitudeInDegrees(), ll.GetLongitudeInDegrees(), 0.f, 0.f);
                    m_setMetricsLocation = true;
                }
            }
        }

        m_pNavigationService->Update(dt);
        m_pInteriorsNavigationService->Update(dt);
        
        if(ToursEnabled())
        {
            ToursModule().GetTourService().UpdateCurrentTour(dt);
        }*/
        
        if (m_loadingScreen != nullptr && (UpdateLoadingScreen(dt, *m_world, *m_loadingScreen)))
        {
            m_loadingScreenCallbacks.ExecuteCallbacks();
            m_messageBus->Publish(LoadingScreenCompleteMessage());
            m_loadingScreen = nullptr;
        }
    }
    
    void MobileExampleApp::InitialiseApplicationViewState(const TContainer& container)
    {
        container->resolve<Watermark::View::IWatermarkViewModel>()->AddToScreen();
        container->resolve<SettingsMenu::View::SettingsMenuViewModel>()->AddToScreen();
        container->resolve<FlattenButton::View::IFlattenButtonViewModel>()->AddToScreen();
        container->resolve<Compass::View::ICompassViewModel>()->AddToScreen();
        container->resolve<MyPinCreation::View::IMyPinCreationInitiationViewModel>()->AddToScreen();
        container->resolve<SearchMenu::View::SearchMenuViewModel>()->AddToScreen();
    }

    void MobileExampleApp::RegisterLoadingScreenComplete(Eegeo::Helpers::ICallback0& callback)
    {
        m_loadingScreenCallbacks.AddCallback(callback);
    }
    
    void MobileExampleApp::UnregisterLoadingScreenComplete(Eegeo::Helpers::ICallback0& callback)
    {
        m_loadingScreenCallbacks.RemoveCallback(callback);
    }
    
    void MobileExampleApp::Draw (float dt)
    {
        Eegeo::Camera::RenderCamera renderCamera = m_cameraController->GetRenderCamera();
        Eegeo::Camera::CameraState cameraState = m_cameraController->GetCameraState();
        
        Eegeo::dv3 ecefInterestPoint(cameraState.InterestPointEcef());

        if (!m_world->Initialising())
        {
            m_worldPinsInFocusController->Update(dt, ecefInterestPoint, renderCamera);
        }

        Eegeo::EegeoDrawParameters drawParameters(cameraState.LocationEcef(),
                                                  cameraState.InterestPointEcef(),
                                                  cameraState.ViewMatrix(),
                                                  cameraState.ProjectionMatrix(),
                                                  *m_screenProperties);
        m_world->Draw(drawParameters);
        
        if (m_loadingScreen != nullptr)
        {
            m_loadingScreen->Draw();
        }
    }

    void MobileExampleApp::NotifyScreenPropertiesChanged(const std::shared_ptr<Eegeo::Rendering::ScreenProperties>& screenProperties)
    {
        m_screenProperties = screenProperties;

        if (m_loadingScreen != nullptr)
        {
            m_loadingScreen->NotifyScreenDimensionsChanged(screenProperties->GetScreenWidth(), screenProperties->GetScreenHeight());
        }
        
        m_worldPinsPlatformServices->GetPinsModule()->UpdateScreenProperties(*m_screenProperties);
        m_interiorPinsPlatformServices->GetPinsModule()->UpdateScreenProperties(*m_screenProperties);

		/*m_pPinsModule->UpdateScreenProperties(m_screenProperties);

        m_pGlobeCameraController->UpdateScreenProperties(m_screenProperties);

		m_pInteriorsExplorerModule->GetInteriorsCameraController().UpdateScreenProperties(m_screenProperties);*/
    }
    
    bool MobileExampleApp::IsRunning() const
    {
        return !m_world->Initialising();
    }

}

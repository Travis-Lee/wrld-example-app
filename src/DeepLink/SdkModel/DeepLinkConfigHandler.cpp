// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#include "DeepLinkConfigHandler.h"
#include "IWebLoadRequest.h"
#include "IWebLoadRequestFactory.h"
#include "ICameraTransitionController.h"
#include "ApiKey.h"
#include "ApplicationConfigurationBuilder.h"
#include "ApplicationConfigurationJsonParser.h"
#include "ApplicationConfigurationXorCipher.h"
#include "base64.h"
#include "ICoverageTreeManifestLoader.h"
#include "CityThemeLoader.h"
#include "CoverageTreeManifestNotifier.h"
#include "ApiTokenService.h"
#include "ICityThemesService.h"
#include "NavigationService.h"
#include "CityThemeData.h"
#include "IAppModeModel.h"
#include "InteriorSelectionModel.h"

namespace ExampleApp
{
    namespace DeepLink
    {
        namespace SdkModel
        {            
            DeepLinkConfigHandler::DeepLinkConfigHandler(CameraTransitions::SdkModel::ICameraTransitionController& cameraTransitionController,
                                                         Eegeo::Web::IWebLoadRequestFactory& webRequestFactory,
                                                         Eegeo::UI::NativeAlerts::IAlertBoxFactory& alertBoxFactory,
                                                         ApplicationConfig::ApplicationConfiguration& defaultConfig,
                                                         Eegeo::Streaming::CoverageTrees::ICoverageTreeManifestLoader& manifestLoader,
                                                         Eegeo::Streaming::CoverageTrees::CoverageTreeManifestNotifier& manifestNotifier,
                                                         Eegeo::Resources::CityThemes::CityThemeLoader& cityThemeLoader,
                                                         Eegeo::Resources::CityThemes::ICityThemesService& cityThemeService,
                                                         Search::SdkModel::ISearchQueryPerformer& searchQueryPerformer,
                                                         Eegeo::Location::NavigationService& navigationService,
                                                         Eegeo::Web::ApiTokenService& apiTokenService,
                                                         Eegeo::Resources::Interiors::InteriorSelectionModel& interiorSelectionModel,
                                                         const ExampleApp::AppModes::SdkModel::IAppModeModel& appModeModel)
            :m_webRequestFactory(webRequestFactory)
            ,m_configRequestCompleteCallback(this, &DeepLinkConfigHandler::HandleConfigResponse)
            ,m_failAlertHandler(this, &DeepLinkConfigHandler::OnFailAlertBoxDismissed)
            ,m_cameraTransitionController(cameraTransitionController)
            ,m_alertBoxFactory(alertBoxFactory)
            ,m_defaultConfig(defaultConfig)
            ,m_manifestLoader(manifestLoader)
            ,m_manifestNotifier(manifestNotifier)
            ,m_cityThemeLoader(cityThemeLoader)
            ,m_cityThemeService(cityThemeService)
            ,m_searchQueryPerformer(searchQueryPerformer)
            ,m_apiTokenService(apiTokenService)
            ,m_navigationService(navigationService)
            ,m_previouslyLoadedCoverageTreeUrl("")
            ,m_previouslyLoadedThemeManifestUrl("")
            ,m_newManifestCallback(this, &DeepLinkConfigHandler::HandleNewCoverageTreeManifestLoaded)
            ,m_newThemeDataCallback(this, &DeepLinkConfigHandler::HandleNewThemeManifestLoaded)
            ,m_interiorSelectionModel(interiorSelectionModel)
            ,m_appModeModel(appModeModel)

            {
                m_manifestNotifier.AddManifestLoadedObserver(m_newManifestCallback);
                m_cityThemeService.SubscribeSharedThemeDataChanged(m_newThemeDataCallback);
            }

            DeepLinkConfigHandler::~DeepLinkConfigHandler()
            {
                m_cityThemeService.UnsubscribeSharedThemeDataChanged(m_newThemeDataCallback);
                m_manifestNotifier.RemoveManifestLoadedObserver(m_newManifestCallback);
            }
            
            void DeepLinkConfigHandler::HandleDeepLink(const AppInterface::UrlData& data)
            {
                const std::string url = GenerateConfigUrl(data);
                Eegeo::Web::IWebLoadRequest* webRequest = m_webRequestFactory.Begin(Eegeo::Web::HttpVerbs::GET, url, m_configRequestCompleteCallback).Build();
                webRequest->Load();
            }

            std::string DeepLinkConfigHandler::GenerateConfigUrl(const AppInterface::UrlData& data) const
            {
                return CONFIG_FILES_HOME + data.path + "/manifest";
            }
            
            void DeepLinkConfigHandler::HandleNewCoverageTreeManifestLoaded(const Eegeo::Streaming::CoverageTrees::CoverageTreeManifest& manifest)
            {
                m_previouslyLoadedCoverageTreeUrl = manifest.ManifestUrl;
            }
            
            void DeepLinkConfigHandler::HandleNewThemeManifestLoaded()
            {
                m_previouslyLoadedThemeManifestUrl = m_cityThemeService.GetSharedThemeData().ManifestUrl;
            }

            void DeepLinkConfigHandler::HandleConfigResponse(Eegeo::Web::IWebResponse& webResponse)
            {
                if(webResponse.IsSucceeded())
                {
                    const std::string& configKey = base64_decode(ExampleApp::ApplicationConfigurationSecret);
                    ApplicationConfig::SdkModel::ApplicationConfigurationXorCipher applicationConfigurationEncryption(configKey);
                    ApplicationConfig::SdkModel::ApplicationConfigurationBuilder configBuilder(applicationConfigurationEncryption, configKey);
                    
                    ApplicationConfig::SdkModel::ApplicationConfigurationJsonParser parser(m_defaultConfig, configBuilder);
                    size_t resultSize = webResponse.GetBodyData().size();
                    std::string resultString = std::string(reinterpret_cast<char const*>(&(webResponse.GetBodyData().front())), resultSize);
                    
                    if(parser.IsValidConfig(resultString))
                    {
                        ApplicationConfig::ApplicationConfiguration applicationConfig = parser.ParseConfiguration(resultString);
                        m_apiTokenService.ApiKeyChanged(applicationConfig.EegeoApiKey());
                        if(applicationConfig.CoverageTreeManifestURL() != m_previouslyLoadedCoverageTreeUrl)
                        {
                            m_manifestLoader.LoadCoverageTreeManifest(applicationConfig.CoverageTreeManifestURL());
                        }
                        
                        const std::string themeNameContains = "Summer";
                        const std::string themeStateName = "DayDefault";
                        if(applicationConfig.ThemeManifestURL() != m_previouslyLoadedThemeManifestUrl)
                        {
                            m_cityThemeLoader.LoadThemes(applicationConfig.ThemeManifestURL(), themeNameContains, themeStateName);
                        }
                        
                        const float newHeading = Eegeo::Math::Deg2Rad(applicationConfig.OrientationDegrees());
                        if(m_appModeModel.GetAppMode() != ExampleApp::AppModes::SdkModel::InteriorMode)
                        {
                            m_interiorSelectionModel.ClearSelection();
                        }
                        m_cameraTransitionController.StartTransitionTo(applicationConfig.InterestLocation().ToECEF(), applicationConfig.DistanceToInterestMetres(), newHeading, applicationConfig.IndoorId(), applicationConfig.FloorIndex());
                        
                        const bool useGps = applicationConfig.TryStartAtGpsLocation();
                        if (useGps)
                        {
                            m_navigationService.SetGpsMode(Eegeo::Location::NavigationService::GpsModeFollow);
                        }
                        else
                        {
                            m_navigationService.SetGpsMode(Eegeo::Location::NavigationService::GpsModeOff);
                        }
                        
                        const std::string PerformStartUpSearch = "perform_start_up_search";
                        const bool mapsceneSpecifiesStartUpSearch = parser.HasKey(resultString, PerformStartUpSearch);
                        const bool shouldPerformStartUpSearch = mapsceneSpecifiesStartUpSearch && applicationConfig.ShouldPerformStartUpSearch();
                        if (shouldPerformStartUpSearch)
                        {
                            m_searchQueryPerformer.PerformSearchQuery(applicationConfig.StartUpSearchTag(), true, false, false, applicationConfig.InterestLocation());
                        }
                        else
                        {
                            m_searchQueryPerformer.RemoveSearchQueryResults();
                        }
                    }
                    else
                    {
                        m_alertBoxFactory.CreateSingleOptionAlertBox("User config file invalid", "Reverting to default config",m_failAlertHandler);
                    }
                }
                else
                {
                    m_alertBoxFactory.CreateSingleOptionAlertBox("User config load failed", "Reverting to default config",m_failAlertHandler);
                }
            }

            void DeepLinkConfigHandler::OnFailAlertBoxDismissed()
            { //Do nothing
            }
        }
    }
}

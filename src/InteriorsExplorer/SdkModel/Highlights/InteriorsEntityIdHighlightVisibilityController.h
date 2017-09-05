// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "Types.h"
#include "InteriorsExplorer.h"
#include "Search.h"
#include "Interiors.h"
#include "Rendering.h"
#include "SearchResultSection.h"
#include "BidirectionalBus.h"
#include "HighlightColorMapper.h"


namespace ExampleApp
{
    namespace InteriorsExplorer
    {
        namespace SdkModel
        {
            namespace Highlights
            {
                
                class InteriorsEntityIdHighlightVisibilityController : private Eegeo::NonCopyable
                {
                public:
                    InteriorsEntityIdHighlightVisibilityController(
                        Eegeo::Resources::Interiors::Highlights::IInteriorsHighlightService& highlightsService,
                        Search::SdkModel::ISearchQueryPerformer& searchQueryPerformer,
                        Search::SdkModel::ISearchResultRepository& searchResultRepository,
                        ExampleAppMessaging::TMessageBus& messageBus,
                        IHighlightColorMapper& highlightColorMapper);
                    
                    ~InteriorsEntityIdHighlightVisibilityController();
                    
                private:
                    void OnSearchResultsLoaded(const Search::SearchQueryResponseReceivedMessage& message);
                    void OnSearchResultsCleared();
                    void OnSearchItemSelected(const SearchResultSection::SearchResultSectionItemSelectedMessage& message);
                    
                    void ClearHighlights();
                    void RefreshHighlights();

                    std::vector<Search::SdkModel::SearchResultModel> m_searchResults;
                    int m_searchResultsIndex;

                    typedef std::vector<Eegeo::Rendering::Renderables::InstancedInteriorFloorRenderable*> InstancedRenderableVector;
                    InstancedRenderableVector m_lastHighlightedRenderables;

                    Eegeo::Resources::Interiors::Highlights::IInteriorsHighlightService& m_interiorsHighlightService;
                    
                    Search::SdkModel::ISearchQueryPerformer& m_searchQueryPerformer;
                    ExampleAppMessaging::TMessageBus& m_messageBus;
                    
                    Eegeo::Helpers::TCallback1<InteriorsEntityIdHighlightVisibilityController, const SearchResultSection::SearchResultSectionItemSelectedMessage&> m_handleSearchResultSectionItemSelectedMessageBinding;
                    Eegeo::Helpers::TCallback1<InteriorsEntityIdHighlightVisibilityController, const Search::SearchQueryResponseReceivedMessage&> m_searchResultsHandler;
                    Eegeo::Helpers::TCallback0<InteriorsEntityIdHighlightVisibilityController> m_searchResultsClearedHandler;
                    
                    IHighlightColorMapper& m_highlightColorMapper;
                };
            }
        }
    }
}


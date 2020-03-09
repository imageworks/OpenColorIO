// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "Display.h"
#include "ParseUtils.h"

namespace OCIO_NAMESPACE
{

DisplayMap::iterator find_display(DisplayMap & displays, const std::string & display)
{
    for(DisplayMap::iterator iter = displays.begin();
        iter != displays.end();
        ++iter)
    {
        if(StrEqualsCaseIgnore(display, iter->first)) return iter;
    }
    return displays.end();
}

DisplayMap::const_iterator find_display_const(const DisplayMap & displays, const std::string & display)
{
    for(DisplayMap::const_iterator iter = displays.begin();
        iter != displays.end();
        ++iter)
    {
        if(StrEqualsCaseIgnore(display, iter->first)) return iter;
    }
    return displays.end();
}

int find_view(const ViewVec & vec, const std::string & name)
{
    for(unsigned int i=0; i<vec.size(); ++i)
    {
        if(StrEqualsCaseIgnore(name, vec[i].name)) return i;
    }
    return -1;
}

void AddDisplay(DisplayMap & displays,
                const std::string & display,
                const std::string & view,
                const std::string & colorspace,
                const std::string & looks)
{
    DisplayMap::iterator iter = find_display(displays, display);
    if(iter == displays.end())
    {
        ViewVec views;
        views.push_back( View(view, colorspace, looks) );
        displays.push_back(std::make_pair(display, views));
    }
    else
    {
        ViewVec & views = iter->second;
        int index = find_view(views, view);
        if (index < 0)
        {
            views.push_back( View(view, colorspace, looks) );
        }
        else
        {
            views[index].colorspace = colorspace;
            views[index].looks = looks;
        }
    }
}

void ComputeDisplays(StringUtils::StringVec & displayCache,
                     const DisplayMap & displays,
                     const StringUtils::StringVec & activeDisplays,
                     const StringUtils::StringVec & activeDisplaysEnvOverride)
{
    displayCache.clear();

    StringUtils::StringVec displayMasterList;
    for(DisplayMap::const_iterator iter = displays.begin();
        iter != displays.end();
        ++iter)
    {
        displayMasterList.push_back(iter->first);
    }

    // Apply the env override if it's not empty.
    if(!activeDisplaysEnvOverride.empty())
    {
        displayCache = IntersectStringVecsCaseIgnore(activeDisplaysEnvOverride, displayMasterList);
        if(!displayCache.empty()) return;
    }
    // Otherwise, apply the active displays if it's not empty.
    else if(!activeDisplays.empty())
    {
        displayCache = IntersectStringVecsCaseIgnore(activeDisplays, displayMasterList);
        if(!displayCache.empty()) return;
    }

    displayCache = displayMasterList;
}

} // namespace OCIO_NAMESPACE

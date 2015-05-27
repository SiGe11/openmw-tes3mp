#include "pathgridcheck.hpp"

#include <sstream>
#include <algorithm>

#include "../world/universalid.hpp"
#include "../world/idcollection.hpp"
#include "../world/subcellcollection.hpp"
#include "../world/pathgrid.hpp"

#include "../settings/usersettings.hpp"

CSMTools::PathgridCheckStage::PathgridCheckStage (const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids)
: mPathgrids (pathgrids)
{}

int CSMTools::PathgridCheckStage::setup()
{
    return mPathgrids.getSize();
}

void CSMTools::PathgridCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    // NOTE: This is horribly inefficient but in order to use signals the entire Stage class
    // hierarchy needs to be braught under Qt which seems like an overkill for a small
    // performance gain during verify operations
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    bool extraCheck = userSettings.setting ("verifier/pathgrid-extra-check", QString ("false"))=="true";

    const CSMWorld::Record<CSMWorld::Pathgrid>& record = mPathgrids.getRecord (stage);

    if (record.isDeleted())
        return;

    const CSMWorld::Pathgrid& pathgrid = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Pathgrid, pathgrid.mId);

    // check the number of pathgrid points
    if (pathgrid.mData.mS2 > static_cast<int>(pathgrid.mPoints.size()))
        messages.push_back (std::make_pair (id, pathgrid.mId + " has less points than expected"));
    else if (pathgrid.mData.mS2 > static_cast<int>(pathgrid.mPoints.size()))
        messages.push_back (std::make_pair (id, pathgrid.mId + " has more points than expected"));

    std::vector<CSMTools::Point> pointList(pathgrid.mPoints.size());
    std::vector<int> duplList;

    for (unsigned int i = 0; i < pathgrid.mEdges.size(); ++i)
    {
        if (pathgrid.mEdges[i].mV0 < static_cast<int>(pathgrid.mPoints.size()) && pathgrid.mEdges[i].mV0 >= 0)
        {
            pointList[pathgrid.mEdges[i].mV0].mConnectionNum++;
            // first check for duplicate edges
            unsigned int j = 0;
            for (; j < pointList[pathgrid.mEdges[i].mV0].mOtherIndex.size(); ++j)
            {
                if (pointList[pathgrid.mEdges[i].mV0].mOtherIndex[j] == pathgrid.mEdges[i].mV1)
                {
                    std::ostringstream ss;
                    ss << "has a duplicate edge between points" << pathgrid.mEdges[i].mV0
                        << " and " << pathgrid.mEdges[i].mV1;
                    messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
                    break;
                }
            }

            // only add if not a duplicate
            if (j == pointList[pathgrid.mEdges[i].mV0].mOtherIndex.size())
                pointList[pathgrid.mEdges[i].mV0].mOtherIndex.push_back(pathgrid.mEdges[i].mV1);
        }
        else
        {
            std::ostringstream ss;
            ss << " has an edge connecting a non-existent point " << pathgrid.mEdges[i].mV0;
            messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
        }
    }

    for (unsigned int i = 0; i < pathgrid.mPoints.size(); ++i)
    {
        // check the connection number for each point matches the edge connections
        if (pathgrid.mPoints[i].mConnectionNum > pointList[i].mConnectionNum)
        {
            std::ostringstream ss;
            ss << " has has less edges than expected for point " << i;
            messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
        }
        else if (pathgrid.mPoints[i].mConnectionNum < pointList[i].mConnectionNum)
        {
            std::ostringstream ss;
            ss << " has has more edges than expected for point " << i;
            messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
        }

        // check that edges are bidirectional
        bool foundReverse = false;
        for (unsigned int j = 0; j < pointList[i].mOtherIndex.size(); ++j)
        {
            for (unsigned int k = 0; k < pointList[pointList[i].mOtherIndex[j]].mOtherIndex.size(); ++k)
            {
                if (pointList[pointList[i].mOtherIndex[j]].mOtherIndex[k] == static_cast<int>(i))
                {
                    foundReverse = true;
                    break;
                }
            }

            if (!foundReverse)
            {
                std::ostringstream ss;
                ss << " has a missing edge between points " << i << " and " << pointList[i].mOtherIndex[j];
                messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
            }
        }

        if (!extraCheck)
            continue;

        // check duplicate points
        // FIXME: how to do this efficiently?
        for (unsigned int j = 0; j < pathgrid.mPoints.size(); ++j)
        {
            if (j == i)
                continue;

            if (pathgrid.mPoints[i].mX == pathgrid.mPoints[j].mX &&
                pathgrid.mPoints[i].mY == pathgrid.mPoints[j].mY &&
                pathgrid.mPoints[i].mZ == pathgrid.mPoints[j].mZ)
            {
                std::vector<int>::const_iterator it = find(duplList.begin(), duplList.end(), i);
                if (it == duplList.end())
                {
                    std::ostringstream ss;
                    ss << " has a duplicated point (" << i
                        << ") x=" << pathgrid.mPoints[i].mX
                        << ", y=" << pathgrid.mPoints[i].mY
                        << ", z=" << pathgrid.mPoints[i].mZ;
                    messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));

                    duplList.push_back(i);
                    break;
                }
            }
        }
    }

    if (!extraCheck)
        return;

    // check pathgrid points that are not connected to anything
    for (unsigned int i = 0; i < pointList.size(); ++i)
    {
        if (pointList[i].mConnectionNum == 0)
        {
            std::ostringstream ss;
            ss << " has an orphaned point (" << i
                << ") x=" << pathgrid.mPoints[i].mX
                << ", y=" << pathgrid.mPoints[i].mY
                << ", z=" << pathgrid.mPoints[i].mZ;
            messages.push_back (std::make_pair (id, pathgrid.mId + ss.str()));
        }
    }

    // TODO: check whether there are disconnected graphs
}

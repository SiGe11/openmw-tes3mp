#include "terraingrid.hpp"

#include <memory>

#include <osg/Group>

#include "chunkmanager.hpp"

namespace Terrain
{

TerrainGrid::TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask)
    : Terrain::World(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask)
    , mNumSplits(4)
{
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }
}

osg::ref_ptr<osg::Node> TerrainGrid::cacheCell(int x, int y)
{
    osg::Vec2f center(x+0.5f, y+0.5f);
    return buildTerrain(NULL, 1.f, center);
}

osg::ref_ptr<osg::Node> TerrainGrid::buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter)
{
    if (chunkSize * mNumSplits > 1.f)
    {
        // keep splitting
        osg::ref_ptr<osg::Group> group (new osg::Group);
        if (parent)
            parent->addChild(group);

        float newChunkSize = chunkSize/2.f;
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, -newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, -newChunkSize/2.f));
        return group;
    }
    else
    {
        osg::ref_ptr<osg::Node> node = mChunkManager->getChunk(chunkSize, chunkCenter, 0, 0);
        if (!node)
            return NULL;
        if (parent)
            parent->addChild(node);

        return node;
    }
}

void TerrainGrid::loadCell(int x, int y)
{
    if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
        return; // already loaded

    osg::Vec2f center(x+0.5f, y+0.5f);
    osg::ref_ptr<osg::Node> terrainNode = buildTerrain(NULL, 1.f, center);
    if (!terrainNode)
        return; // no terrain defined

    mTerrainRoot->addChild(terrainNode);

    mGrid[std::make_pair(x,y)] = terrainNode;
}

void TerrainGrid::unloadCell(int x, int y)
{
    Grid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    osg::ref_ptr<osg::Node> terrainNode = it->second;
    mTerrainRoot->removeChild(terrainNode);

    mGrid.erase(it);
}

}

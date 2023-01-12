/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Common.h>
#include <rocky/Config.h>
#include <rocky/Color.h>
#include <string>

namespace ROCKY_NAMESPACE
{
    // Options structure for a terrain engine (internal)
    class ROCKY_EXPORT TerrainOptions : public DriverConfigOptions
    {
    public:
        META_ConfigOptions(rocky, TerrainOptions, DriverConfigOptions);
        ROCKY_OPTION(int, tileSize);
        ROCKY_OPTION(float, minTileRangeFactor);
        ROCKY_OPTION(bool, clusterCulling);
        ROCKY_OPTION(unsigned, maxLOD);
        ROCKY_OPTION(unsigned, minLOD);
        ROCKY_OPTION(unsigned, firstLOD);
        ROCKY_OPTION(bool, enableLighting);
        ROCKY_OPTION(bool, enableBlending);
        ROCKY_OPTION(bool, compressNormalMaps);
        ROCKY_OPTION(unsigned, minNormalMapLOD);
        ROCKY_OPTION(bool, gpuTessellation);
        ROCKY_OPTION(float, tessellationLevel);
        ROCKY_OPTION(float, tessellationRange);
        ROCKY_OPTION(bool, debug);
        ROCKY_OPTION(int, renderBinNumber);
        ROCKY_OPTION(unsigned, minExpiryFrames);
        ROCKY_OPTION(double, minExpiryTime);
        ROCKY_OPTION(float, minExpiryRange);
        ROCKY_OPTION(unsigned, maxTilesToUnloadPerFrame);
        ROCKY_OPTION(unsigned, minResidentTiles);
        ROCKY_OPTION(bool, castShadows);
        //ROCKY_OPTION(osg::LOD::RangeMode, rangeMode);
        ROCKY_OPTION(float, tilePixelSize);
        ROCKY_OPTION(float, heightFieldSkirtRatio);
        ROCKY_OPTION(Color, color);
        ROCKY_OPTION(bool, progressive);
        ROCKY_OPTION(bool, useNormalMaps);
        ROCKY_OPTION(bool, normalizeEdges);
        ROCKY_OPTION(bool, morphTerrain);
        ROCKY_OPTION(bool, morphImagery);
        ROCKY_OPTION(unsigned, mergesPerFrame);
        ROCKY_OPTION(float, priorityScale);
        ROCKY_OPTION(std::string, textureCompression);
        ROCKY_OPTION(unsigned, concurrency);
        ROCKY_OPTION(bool, useLandCover);

        ROCKY_OPTION(float, screenSpaceError);

        virtual Config getConfig() const;
    private:
        void fromConfig(const Config&);
    };

    class ROCKY_EXPORT TerrainOptionsAPI
    {
    public:
        //! Size of each dimension of each terrain tile, in verts.
        //! Ideally this will be a power of 2 plus 1, i.e.: a number X
        //! such that X = (2^Y)+1 where Y is an integer >= 1. Default=17.
        void setTileSize(const int& value);
        const int& getTileSize() const;

        //! The minimum tile LOD range as a factor of a tile's radius. Default = 7.0
        void setMinTileRangeFactor(const float& value);
        const float& getMinTileRangeFactor() const;

        //! Whether cluster culling is enabled on terrain tiles. Deafult=true
        void setClusterCulling(const bool& value);
        const bool& getClusterCulling() const;

        //! (Legacy property)
        //! The maximum level of detail to which the terrain should subdivide.
        //! If you leave this unset the terrain will subdivide until the map layers
        //! stop providing data (default behavior). If you set a value, the terrain
        //! will stop subdividing at the specified LOD even if higher-resolution
        //! data is available. (It still might stop subdividing before it reaches
        //! this level if data runs out
        void setMaxLOD(const unsigned& value);
        const unsigned& getMaxLOD() const;

        //! (Legacy property)
        //! The minimum level of detail to which the terrain should subdivide (no matter what).
        //! If you leave this unset, the terrain will subdivide until the map layers
        //! stop providing data (default behavior). If you set a value, the terrain will subdivide
        //! to the specified LOD no matter what (and may continue farther if higher-resolution
        //! data is available).
        void setMinLOD(const unsigned& value);
        const unsigned& getMinLOD() const;

        //! (Legacy property)
        //! The lowest LOD to display. By default, the terrain begins at LOD 0.
        //! Set this to start the terrain tile mesh at a higher LOD.
        //! Don't set this TOO high though or you will run into memory problems.
        void setFirstLOD(const unsigned& value);
        const unsigned& getFirstLOD() const;

        //! Whether to explicity enable or disable GL lighting on the map node.
        //! Default = true
        void setEnableLighting(const bool& value);
        const bool& getEnableLighting() const;
            
        //! (Legacy property)
        //! Whether to enable blending on the terrain. Default is true.
        void setEnableBlending(const bool& value);
        const bool& getEnableBlending() const;

        //! @deprecated
        //! Whether to compress the normal maps before sending to the GPU
        void setCompressNormalMaps(const bool& value);
        const bool& getCompressNormalMaps() const;

        //! Minimum level of detail at which to generate elevation-based normal maps,
        //! assuming normal maps have been activated. This mitigates the overhead of 
        //! calculating normal maps for very high altitude scenes where they are no
        //! of much use. Default is 0 (zero).
        void setMinNormalMapLOD(const unsigned& value);
        const unsigned& getMinNormalMapLOD() const;

        //! Whether the terrain engine will be using GPU tessellation shaders.
        void setGPUTessellation(const bool& value);
        const bool& getGPUTessellation() const;

        //! GPU tessellation level
        void setTessellationLevel(const float& value);
        const float& getTessellationLevel() const;

        //! Maximum range in meters to apply GPU tessellation
        void setTessellationRange(const float& value);
        const float& getTessellationRange() const;

        //! Whether to activate debugging mode
        void setDebug(const bool& value);
        const bool& getDebug() const;

        //! Render bin number for the terrain
        void setRenderBinNumber(const int& value);
        const int& getRenderBinNumber() const;

        //! Minimum number of frames before unused terrain data is eligible to expire
        void setMinExpiryFrames(const unsigned& value);
        const unsigned& getMinExpiryFrames() const;

        //! Minimum time (seconds) before unused terrain data is eligible to expire
        void setMinExpiryTime(const double& value);
        const double& getMinExpiryTime() const;

        //! Minimun range (distance from camera) beyond which unused terrain data 
        //! is eligible to expire
        void setMinExpiryRange(const float& value);
        const float& getMinExpiryRange() const;

        //! Maximum number of terrain tiles to unload/expire each frame.
        void setMaxTilesToUnloadPerFrame(const unsigned& value);
        const unsigned& getMaxTilesToUnloadPerFrame() const;

        //! Minimum number of terrain tiles to keep in memory before expiring usused data
        void setMinResidentTiles(const unsigned& value);
        const unsigned& getMinResidentTiles() const;

        //! Whether the terrain should cast shadows - default is false
        void setCastShadows(const bool& value);
        const bool& getCastShadows() const;

        //! Size of the tile, in pixels, when using rangeMode = PIXEL_SIZE_ON_SCREEN
        void setTilePixelSize(const float& value);
        const float& getTilePixelSize() const;

        //! Ratio of skirt height to tile width. The "skirt" is geometry extending
        //! down from the edge of terrain tiles meant to hide cracks between adjacent
        //! levels of detail. Default is 0 (no skirt).
        void setHeightFieldSkirtRatio(const float& value);
        const float& getHeightFieldSkirtRatio() const;

        //! Color of the untextured globe (where no imagery is displayed) (default is white)
        void setColor(const Color& value);
        const Color& getColor() const;

        //! Whether to load levels of details one after the other, instead of 
        //! prioritizing them based on the camera position. (default = false)
        void setProgressive(const bool& value);
        const bool& getProgressive() const;

        //! Whether to generate normal map textures. Default is true
        void setUseNormalMaps(const bool& value);
        const bool& getUseNormalMaps() const;

        //! Whether to include landcover textures. Default is true.
        void setUseLandCover(const bool& value);
        const bool& getUseLandCover() const;

        //! Whether to average normal vectors on tile boundaries. Doing so reduces the
        //! the appearance of seams when using lighting, but requires extra CPU work.
        void setNormalizeEdges(const bool& value);
        const bool& getNormalizeEdges() const;

        //! Whether to morph terrain data between terrain tile LODs.
        //! This feature is not available when rangeMode is PIXEL_SIZE_ON_SCREEN
        void setMorphTerrain(const bool& value);
        const bool& getMorphTerrain() const;

        //! Whether to morph imagery between terrain tile LODs.
        //! This feature is not available when rangeMode is PIXEL_SIZE_ON_SCREEN
        void setMorphImagery(const bool& value);
        const bool& getMorphImagery() const;

        //! Maximum number of tile data merges permitted per frame. 0 = infinity.
        void setMergesPerFrame(const unsigned& value);
        const unsigned& getMergesPerFrame() const;

        //! Scale factor for background loading priority of terrain tiles.
        //! Default = 1.0. Make it higher to prioritize terrain loading over
        //! other modules.
        void setPriorityScale(const float& value);
        const float& getPriorityScale() const;

        //! Texture compression to use by default on terrain image textures
        void setTextureCompressionMethod(const std::string& method);
        const std::string& getTextureCompressionMethod() const;

        //! Target concurrency of terrain data loading operations. Default = 4.
        void setConcurrency(const unsigned& value);
        const unsigned& getConcurrency() const;

        //! Screen space error for PIXEL SIZE ON SCREEN LOD mode
        void setScreenSpaceError(const float& value);
        const float& getScreenSpaceError() const;

    public: // Legacy support

        //! Sets the name of the terrain engine driver to use
        void setDriver(const std::string& name);

    private:
        friend class MapNode;
        friend class TerrainEngineNode;
        TerrainOptionsAPI(TerrainOptions*);
        TerrainOptions* _ptr;
        TerrainOptions& options() { return *_ptr; }
        const TerrainOptions& options() const { return *_ptr; }
    };

} // namespace ROCKY_NAMESPACE

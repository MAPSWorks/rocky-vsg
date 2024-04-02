/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/TMS.h>
#ifdef ROCKY_HAS_TMS

#include <rocky/ImageLayer.h>
#include <rocky/URI.h>

namespace ROCKY_NAMESPACE
{
    /**
     * Image layer reading from a TMS (Tile Map Service) endpoint
     */
    class ROCKY_EXPORT TMSImageLayer : public Inherit<ImageLayer, TMSImageLayer>, public TMS::Options
    {
    public:
        //! Construct an empty TMS layer
        TMSImageLayer();
        TMSImageLayer(const JSON&);

        //! Destructor
        virtual ~TMSImageLayer() { }

        //! serialize
        JSON to_json() const override;

    protected: // Layer

        Status openImplementation(const IOOptions& io) override;

        void closeImplementation() override;

        //! Creates a raster image for the given tile key
        Result<GeoImage> createImageImplementation(const TileKey& key, const IOOptions& io) const override;

        //! Writes a raster image for he given tile key (if open for writing)
        //virtual Status writeImageImplementation(const TileKey& key, const osg::Image* image, ProgressCallback* progress) const override;

    private:
        TMS::Driver _driver;

        void construct(const JSON&);
    };
}


#else // if !ROCKY_HAS_TMS
#ifndef ROCKY_BUILDING_SDK
#error TMS support is not enabled in Rocky.
#endif
#endif // ROCKY_HAS_TMS

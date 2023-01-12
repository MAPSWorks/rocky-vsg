/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#if 0
#include "Cube.h"
#include "Notify.h"

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::contrib;

#define LC "[Cube] "

// --------------------------------------------------------------------------

bool
CubeUtils::latLonToFaceCoords(double lat_deg, double lon_deg,
                              double& out_x, double& out_y, int& out_face,
                              int faceHint )
{
    // normalized latitude and longitude
    double nlat = (lat_deg+90.0)/180.0;
    double nlon = (lon_deg+180.0)/360.0;

    // check for out-of-range:
    if ( nlat < 0 || nlat > 1 || nlon < 0 || nlon > 1 )
        return false;

    int face_x;

    if ( faceHint >= 0 )
    {
        out_face = faceHint;
        if ( faceHint < 4 )
        {
            face_x = faceHint;
        }
        else
        {
            face_x = (int)(4 * nlon);
            if ( face_x == 4 ) 
                face_x = 3;
        }        
    }
    else
    {
        face_x = (int)(4 * nlon);
        if ( face_x == 4 )
            face_x = 3;

        int face_y = (int)(2 * nlat + 0.5);
        if ( face_y == 1 )
            out_face = face_x;
        else
            out_face = face_y < 1 ? 5 : 4;

        //GW: not sure why this was here; but I think this issue is the cause of cracks when
        //    projecting CUBE source imagery onto a WGS84 globe.
        //
        //if ( equiv( lat_deg, -45 ) )
        //    out_face = 5;
    }

    out_x = 4 * nlon - face_x;
    out_y = 2 * nlat - 0.5;

    if(out_face < 4) // equatorial calculations done
        return true;

    double tmp;
    if(out_face == 4) // north polar face
    {
        out_y = 1.5 - out_y;
        out_x = 2 * (out_x - 0.5) * out_y + 0.5;
        switch(face_x)
        {
        case 0: // bottom
            out_y = 0.5 - out_y;
            break;
        case 1: // right side, swap and reverse lat
            tmp = out_x;
            out_x = 0.5 + out_y;
            out_y = tmp;
            break;
        case 2: // top; reverse lat and lon
            out_x = 1 - out_x;
            out_y = 0.5 + out_y;
            break;
        case 3: // left side; swap and reverse lon
            tmp = out_x;
            out_x = 0.5 - out_y;
            out_y = 1 - tmp;
            break;
        }
    }
    else // south polar face
    {
        out_y += 0.5;
        out_x = 2 * (out_x - 0.5) * out_y + 0.5;
        switch(face_x)
        {
        case 0: // left
            tmp = out_x;
            out_x = 0.5 - out_y;
            out_y = tmp;
            break;
        case 1: // top
            out_y = 0.5 + out_y;
            break;
        case 2: // right
            tmp = out_x;
            out_x = 0.5 + out_y;
            out_y = 1 - tmp;
            break;
        case 3: // bottom
            out_x = 1 - out_x;
            out_y = 0.5 - out_y;
            break;
        }
    }
    return true;
}

bool
CubeUtils::faceCoordsToLatLon(double x, double y, int face, double& out_lat_deg, double& out_lon_deg)
{
    double offset = 0.0;
    dvec2 s( x, y );

    // validate coordinate range:
    ROCKY_SOFT_ASSERT_AND_RETURN((x >= 0 && x <= 1 && y >= 0 && y <= 1), false);

    if ( face < 4 ) // equatorial faces
    {
        s.x = (x + face) * 0.25;
        s.y = (y + 0.5) * 0.5;
    }
    else if( face == 4 ) // north polar face
    {
        if ( x < y ) // left or top quadrant
        {
            if(x + y < 1.0) // left quadrant
            {
                s.x = 1.0 - y;
                s.y = x;
                offset += 3;
            }
            else // top quadrant
            {
                s.y = 1.0 - y;
                s.x = 1.0 - x;
                offset += 2;
            }
        }
        else if( x + y >= 1.0 ) // right quadrant
        {
            s.x = y;
            s.y = 1.0 - x;
            offset += 1.0;
        }
        s.x -= s.y;
        if(s.y != 0.5)
            s.x *= 0.5 / (0.5 - s.y);

        s.x = (s.x + offset) * 0.25;
        s.y = (s.y + 1.5) * 0.5;
    }
    else if ( face == 5 ) // south polar face
    {
        offset = 1.0;
        if ( x > y ) // right or bottom quadrant
        {
            if( x + y >= 1.0) // right quadrant
            {
                s.x = 1.0 - y;
                s.y = x - 0.5;
                offset += 1.0;
            }
            else // bottom quadrant
            {
                s.x = 1.0 - x;
                s.y = 0.5 - y;
                offset += 2;
            }
        }
        else // left or top quadrant
        {
            if(x + y < 1.0) // left quadrant
            {
                s.x = y;
                s.y = 0.5 - x;
                offset -= 1.0;
            }
            else // top quadrant
                s.y = y - 0.5;
        }
        if(s.y != 0)
            s.x = (s.x - 0.5) * 0.5 / s.y + 0.5;
        s.x = (s.x + offset) * 0.25;
        s.y *= 0.5;
    }
    else 
    {
        return false; // invalid face specification
    }

    // convert to degrees
    out_lon_deg = s.x * 360 - 180;
    out_lat_deg = s.y * 180 - 90;

    return true;
}

bool
CubeUtils::cubeToFace( double& in_out_x, double& in_out_y, int& out_face )
{
    // convert from unicube space (0,0=>6,1) to face space (0,0=>1,1 + face#)
    // too tired to compute a formula right now
    out_face = 
        in_out_x <= 1.0 ? 0 :
        in_out_x <= 2.0 ? 1 :
        in_out_x <= 3.0 ? 2 :
        in_out_x <= 4.0 ? 3 :
        in_out_x <= 5.0 ? 4 : 5;

    in_out_x = in_out_x - (double)out_face;
    // y unchanged
    return true;
}

bool
CubeUtils::cubeToFace(double& in_out_xmin, double& in_out_ymin,
                      double& in_out_xmax, double& in_out_ymax,
                      int& out_face)
{
    int min_face = 
        in_out_xmin < 1.0 ? 0 :
        in_out_xmin < 2.0 ? 1 :
        in_out_xmin < 3.0 ? 2 :
        in_out_xmin < 4.0 ? 3 :
        in_out_xmin < 5.0 ? 4 : 5;

    int max_face =
        in_out_xmax <= 1.0 ? 0 :
        in_out_xmax <= 2.0 ? 1 :
        in_out_xmax <= 3.0 ? 2 :
        in_out_xmax <= 4.0 ? 3 :
        in_out_xmax <= 5.0 ? 4 : 5;

    if ( min_face != max_face )
    {
        ROCKY_WARN << LC << "Min face <> Max face!" << std::endl;
        return false;
    }

    out_face = min_face;

    in_out_xmin -= (double)out_face;
    in_out_xmax -= (double)out_face;

    // y values are unchanged
    return true;
}

bool
CubeUtils::faceToCube( double& in_out_x, double& in_out_y, int face )
{
    // convert from face space (0,0=>1,1 + face#) to unicube space (0,0=>6,1)
    in_out_x = (double)face + in_out_x;
    // y unchanged
    return true;
}

// --------------------------------------------------------------------------

CubeSRS::CubeSRS(const SRS::Key& key) :
    SRS(key)
{
    _is_user_defined = true;
    _is_cube = true;
    _domain = PROJECTED;
    _name = "Unified Cube";

    // Custom units. The big number there roughly converts [0..1] to meters
    // on a spheroid with WGS84-ish radius. Not perfect but close enough for
    // the purposes of this class
    _units = Units("Cube face", "cube", Units::TYPE_LINEAR, 42949672.96 / 4.0);
}

bool
CubeSRS::preTransform(
    std::vector<dvec3>& points,
    const SRS** in_out_srs) const
{
    for( unsigned i=0; i<points.size(); ++i )
    {
        dvec3& p = points[i];

        // Convert the incoming points from cube => face => lat/long.
        int face;
        if (!CubeUtils::cubeToFace(p.x, p.y, face))
        {
            ROCKY_WARN << LC << "Failed to convert (" << p.x << "," << p.y << ") into face coordinates." << std::endl;
            return false;
        }

        double lat_deg, lon_deg;
        bool success = CubeUtils::faceCoordsToLatLon( p.x, p.y, face, lat_deg, lon_deg );
        if (!success)
        {
            ROCKY_WARN << LC << 
                std::fixed << std::setprecision(2)
                << "Could not transform face coordinates ["
                << p.x << ", " << p.y << ", " << face << "] to lat lon"
                << std::endl;
            return false;
        }
        p.x = lon_deg;
        p.y = lat_deg;
    }

    if (in_out_srs)
        *in_out_srs = getGeodeticSRS().get();

    return true;
}

bool
CubeSRS::postTransform(
    std::vector<dvec3>& points,
    const SRS** in_out_srs) const
{
    for( unsigned i=0; i<points.size(); ++i )
    {
        dvec3& p = points[i];

        //Convert the incoming points from lat/lon back to face coordinates
        int face;
        double out_x, out_y;

        // convert from lat/long to x/y/face
        bool success = CubeUtils::latLonToFaceCoords( p.y, p.x, out_x, out_y, face );
        if (!success)
        {
            ROCKY_WARN << LC
                << std::fixed << std::setprecision(2)
                << "Could not transform lat long ["
                << p.y << ", " << p.x << "] coordinates to face" 
                << std::endl;
            return false;
        }

        //TODO: what to do about boundary points?

        if ( !CubeUtils::faceToCube( out_x, out_y, face ) )
        {
            ROCKY_WARN << LC << "fromFace(" << out_x << "," << out_y << "," << face << ") failed" << std::endl;
            return false;
        }
        
        p.x = out_x;
        p.y = out_y;
    }

    if (in_out_srs)
        *in_out_srs = getGeodeticSRS().get();

    return true;
}

#define LL 0
#define LR 1
#define UR 2
#define UL 3

bool
CubeSRS::transformExtentToMBR(
    const SRS& to_srs,
    double&                 in_out_xmin,
    double&                 in_out_ymin,
    double&                 in_out_xmax,
    double&                 in_out_ymax) const
{
    // input bounds:
    Box inBounds(in_out_xmin, in_out_ymin, in_out_xmax, in_out_ymax);

    Box outBounds;

    // for each CUBE face, find the intersection of the input bounds and that face.
    for (int face = 0; face < 6; ++face)
    {
        Box faceBounds((double)(face), 0.0, 0.0, (double)(face + 1), 1.0, 0.0);

        Box isect = faceBounds.intersection_with(inBounds);

        // if they intersect (with a non-zero area; abutting doesn't count in this case)
        // transform the intersection and include in the result.
        if (isect.valid() && isect.area2d() > 0.0)
        {
            double
                xmin = isect.xmin, ymin = isect.ymin,
                xmax = isect.xmax, ymax = isect.ymax;

            if (transformInFaceExtentToMBR(to_srs, face, xmin, ymin, xmax, ymax))
            {
                outBounds.expandBy(Box(xmin, ymin, xmax, ymax));
            }
        }
    }

    if (outBounds.valid())
    {
        in_out_xmin = outBounds.xmin;
        in_out_ymin = outBounds.ymin;
        in_out_xmax = outBounds.xmax;
        in_out_ymax = outBounds.ymax;
        return true;
    }
    else
    {
        return false;
    }
}

bool
CubeSRS::transformInFaceExtentToMBR(
    const SRS& to_srs,
    int face,
    double& in_out_xmin,
    double& in_out_ymin,
    double& in_out_xmax,
    double& in_out_ymax) const
{
    // note: this method only works when the extent is isolated to one face of the cube. If you
    // want to transform an artibrary extent, you need to break it up into separate extents for
    // each cube face.
    bool ok = true;

    double face_xmin = in_out_xmin, face_ymin = in_out_ymin;
    double face_xmax = in_out_xmax, face_ymax = in_out_ymax;

    //int face;
    CubeUtils::cubeToFace( face_xmin, face_ymin, face_xmax, face_ymax, face );

    // for equatorial faces, the normal transformation process will suffice (since it will call into
    // pre/postTransform).
    if ( face < 4 )
    {
        ok = SRS::transformExtentToMBR( to_srs, in_out_xmin, in_out_ymin, in_out_xmax, in_out_ymax );
    }
    else
    {
        // otherwise we are on one of the polar faces (4 or 5):    

        // four corners in face space:
        double fx[4] = { face_xmin, face_xmax, face_xmax, face_xmin };
        double fy[4] = { face_ymin, face_ymin, face_ymax, face_ymax };

        bool crosses_pole = fx[LL] < 0.5 && fx[UR] > 0.5 && fy[LL] < 0.5 && fy[UR] > 0.5;

        if ( crosses_pole ) // full x extent.
        {
            bool north = face == 4; // else south
            dvec3 output;
            
            to_srs.getGeographicSRS()->transform( dvec3(-180.0, north? 45.0 : -90.0, 0), to_srs.get(), output );
            in_out_xmin = output.x;
            in_out_ymin = output.y;

            to_srs.getGeographicSRS()->transform( dvec3(180.0, north? 90.0 : -45.0, 0), to_srs.get(), output );
            in_out_xmax = output.x;
            in_out_ymax = output.y;
            
            //to_srs.getGeographicSRS()->transform2D( -180.0, north? 45.0 : -90.0, to_srs, in_out_xmin, in_out_ymin );
            //to_srs.getGeographicSRS()->transform2D( 180.0, north? 90.0 : -45.0, to_srs, in_out_xmax, in_out_ymax );
        }

        else
        {
            double lat_deg[4];
            double lon_deg[4];
            double latmin, latmax, lonmin, lonmax;

            for( int i=0; i<4; ++i )
            {
                CubeUtils::faceCoordsToLatLon( fx[i], fy[i], face, lat_deg[i], lon_deg[i] );
            }

            latmin = smallest(lat_deg[0], lat_deg[1], lat_deg[2], lat_deg[3]);
            latmax = largest(lat_deg[0], lat_deg[1], lat_deg[2], lat_deg[3]);

            // check to see whether the extent crosses the date line boundary. If so,
            // make the UL corner the southwest and the LR corner the east.
            bool crosses_date_line = fx[UL]+(1-fy[UL]) < 1.0 && (1-fx[LR])+fy[LR] < 1.0 && fx[LL]+fy[LL] < 1.0;
            if ( crosses_date_line )
            {
                lonmin = lon_deg[UL];
                lonmax = lon_deg[LR];
            }
            else
            {
                lonmin = smallest(lon_deg[0], lon_deg[1], lon_deg[2], lon_deg[3]);
                lonmax = largest(lon_deg[0], lon_deg[1], lon_deg[2], lon_deg[3]);
            }

            if ( to_srs.isGeographic() )
            {
                in_out_xmin = lonmin;
                in_out_xmax = lonmax;
                in_out_ymin = latmin;
                in_out_ymax = latmax;
            }
            else
            {
                dvec3 output;

                bool ok1 = transform(dvec3(lonmin, latmin, 0), to_srs.get(), output);
                if (ok1) {
                    in_out_xmin = output.x;
                    in_out_ymin = output.y;
                }
                bool ok2 = transform(dvec3(lonmax, latmax, 0), to_srs.get(), output);
                if (ok2) {
                    in_out_xmax = output.x;
                    in_out_ymax = output.y;
                }

                //bool ok1 = transform2D( lonmin, latmin, to_srs, in_out_xmin, in_out_ymin, context );
                //bool ok2 = transform2D( lonmax, latmax, to_srs, in_out_xmax, in_out_ymax, context );
                ok = ok1 && ok2;
            }
        }
    }

    return ok;
}

// --------------------------------------------------------------------------

UnifiedCubeProfile::UnifiedCubeProfile() :
    super(
        SRS::get("unified-cube"),
        Box(0.0, 0.0, 6.0, 1.0),
        6, 1)

{
    auto srs = srs().getGeographicSRS();

    _shared->_latlong_extent = GeoExtent(
        srs, -180.0, -90.0, 180.0, 90.0);

    // set up some constant extents
    _faceExtent_gcs[0] = GeoExtent(srs, -180, -45, -90, 45);
    _faceExtent_gcs[1] = GeoExtent(srs, -90, -45, 0, 45);
    _faceExtent_gcs[2] = GeoExtent(srs, 0, -45, 90, 45);
    _faceExtent_gcs[3] = GeoExtent(srs, 90, -45, 180, 45);
    _faceExtent_gcs[4] = GeoExtent(srs, -180, 45, 180, 90); // north polar
    _faceExtent_gcs[5] = GeoExtent(srs, -180, -90, 180, -45); // south polar
}

int
UnifiedCubeProfile::getFace( const TileKey& key )
{
    return key.getTileX() >> key.getLevelOfDetail();
}

GeoExtent
UnifiedCubeProfile::transformGcsExtentOnFace( const GeoExtent& gcsExtent, int face ) const
{
    if ( face < 4 )
    {
        const GeoExtent& fex = _faceExtent_gcs[face];

        return GeoExtent(
            srs(),
            (double)face + (gcsExtent.xMin()-fex.xMin()) / fex.width(),
            (gcsExtent.yMin()-fex.yMin()) / fex.height(),
            (double)face + (gcsExtent.xMax()-fex.xMin()) / fex.width(),
            (gcsExtent.yMax()-fex.yMin()) / fex.height() );
    }
    else
    {
        // transform all 4 corners; then do the min/max for x/y.
        double lon[4] = { gcsExtent.xMin(), gcsExtent.xMax(), gcsExtent.xMax(), gcsExtent.xMin() };
        double lat[4] = { gcsExtent.yMin(), gcsExtent.yMin(), gcsExtent.yMax(), gcsExtent.yMax() };
        double x[4], y[4];

        for( int i=0; i<4; ++i )
        {
            int dummy;
            if ( ! CubeUtils::latLonToFaceCoords( lat[i], lon[i], x[i], y[i], dummy, face ) )
            {
                ROCKY_WARN << LC << "transformGcsExtentOnFace, ll2fc failed" << std::endl;
            }
        }

        double xmin = smallest(x[0], x[1], x[2], x[3]);
        double xmax = largest(x[0], x[1], x[2], x[3]);
        double ymin = smallest(y[0], y[1], y[2], y[3]);
        double ymax = largest(y[0], y[1], y[2], y[3]);

        CubeUtils::faceToCube( xmin, ymin, face );
        CubeUtils::faceToCube( xmax, ymax, face );

        return GeoExtent( srs(), xmin, ymin, xmax, ymax );
    }
}

bool
UnifiedCubeProfile::transformAndExtractContiguousExtents(
    const GeoExtent& input,
    std::vector<GeoExtent>& output) const
{
    ROCKY_SOFT_ASSERT_AND_RETURN(valid() && input.valid(), false);

    if (srs().isHorizEquivalentTo(input.srs()))
    {
        output.push_back(input);
    }
    else
    {
        // the cube profile is non-contiguous. so there may be multiple local extents required
        // to fully intersect the remote extent.

        // first transform the remote extent to lat/long.
        GeoExtent input_gcs = input.srs().isGeographic()
            ? input
            : input.transform(input.srs().getGeographicSRS());

        // Chop the input extent into three separate extents: for the equatorial, north polar,
        // and south polar tile regions.
        for (int face = 0; face < 6; ++face)
        {
            GeoExtent partExtent_gcs = _faceExtent_gcs[face].intersectionSameSRS(input_gcs);
            if (partExtent_gcs.valid())
            {
                GeoExtent partExtent = transformGcsExtentOnFace(partExtent_gcs, face);
                if (partExtent.valid())
                {
                    output.push_back(partExtent);
                }
            }
        }
    }

    return true;
}

#if 0
void
UnifiedCubeProfile::getIntersectingTiles(const GeoExtent&      remoteExtent,
                                         unsigned              localLOD,
                                         std::vector<TileKey>& out_intersectingKeys ) const
{
    if (srs().isHorizEquivalentTo(remoteExtent.srs().get()))
    {
        addIntersectingKeys(remoteExtent, localLOD, out_intersectingKeys);
    }
    else
    {
        // the cube profile is non-contiguous. so there may be multiple local extents required
        // to fully intersect the remote extent.

        // first transform the remote extent to lat/long.
        GeoExtent remoteExtent_gcs = remoteExtent.srs().isGeographic()
            ? remoteExtent
            : remoteExtent.transform( remoteExtent.srs().getGeographicSRS() );

        // Chop the input extent into three separate extents: for the equatorial, north polar,
        // and south polar tile regions.
        for( int face=0; face<6; ++face )
        {
            GeoExtent partExtent_gcs = _faceExtent_gcs[face].intersectionSameSRS( remoteExtent_gcs );
            if ( partExtent_gcs.valid() )
            {
                GeoExtent partExtent = transformGcsExtentOnFace( partExtent_gcs, face );
                addIntersectingTiles( partExtent, localLOD, out_intersectingKeys );
            }
        }        
    }
}
#endif

unsigned
UnifiedCubeProfile::getEquivalentLOD(const Profile* rhsProfile, unsigned rhsLOD) const
{    
    return rhsLOD;
}


UnifiedCubeProfile::~UnifiedCubeProfile()
{
}
#endif
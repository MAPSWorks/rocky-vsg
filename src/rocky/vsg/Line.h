/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/vsg/ECS.h>
#include <rocky/SRS.h>
#include <vsg/nodes/Geometry.h>
#include <vsg/commands/DrawIndexed.h>
#include <vsg/state/BindDescriptorSet.h>
#include <optional>

namespace ROCKY_NAMESPACE
{
    //! Settings when constructing a similar set of line drawables
    //! Note, this structure is mirrored on the GPU so alignment rules apply!
    struct LineStyle
    {
        // if alpha is zero, use the line's per-vertex color instead
        vsg::vec4 color = { 1, 1, 1, 0 };
        float width = 2.0f; // pixels
        int stipple_pattern = 0xffff;
        int stipple_factor = 1;
        float resolution = 100000.0f; // meters
        float depth_offset = 0.0f; // meters
    };

    namespace detail
    {
        /**
        * Renders a line or linestring geometry.
        */
        class ROCKY_EXPORT LineGeometry : public vsg::Inherit<vsg::Geometry, LineGeometry>
        {
        public:
            //! Construct a new line string geometry node
            LineGeometry();

            //! Adds a vertex to the end of the line string
            void push_back(const vsg::vec3& vert);

            //! Number of verts comprising this line string
            unsigned numVerts() const;

            //! The first vertex in the line string to render
            void setFirst(unsigned value);

            //! Number of vertices in the line string to render
            void setCount(unsigned value);

            //! Recompile the geometry after making changes.
            //! TODO: just make it dynamic instead
            void compile(vsg::Context&) override;

        protected:
            vsg::vec4 _defaultColor = { 1,1,1,1 };
            std::vector<vsg::vec3> _current;
            std::vector<vsg::vec3> _previous;
            std::vector<vsg::vec3> _next;
            std::vector<vsg::vec4> _colors;
            vsg::ref_ptr<vsg::DrawIndexed> _drawCommand;
        };

        /**
        * Applies a line style.
        */
        class ROCKY_EXPORT BindLineDescriptors : public vsg::Inherit<vsg::BindDescriptorSet, BindLineDescriptors>
        {
        public:
            //! Construct a line style node
            BindLineDescriptors();

            //! Initialize this command with the associated layout
            void init(vsg::ref_ptr<vsg::PipelineLayout> layout);

            //! Refresh the data buffer contents on the GPU
            void updateStyle(const LineStyle&);

            vsg::ref_ptr<vsg::ubyteArray> _styleData;
        };
    }

    /**
    * LineString component - holds one or more separate line string geometries
    * sharing the same style.
    */
    class ROCKY_EXPORT Line : public ECS::NodeComponent
    {
    public:
        //! Construct a new component
        Line();

        //! Dynamic line styling. This is optional.
        std::optional<LineStyle> style;

        //! Whether lines should write to the depth buffer
        bool write_depth = false;

        //! Pushes a new sub-geometry along with its range of points.
        //! @param begin Iterator of first point to add to the new sub-geometry
        //! @param end Iterator past the final point to add to the new sub-geometry
        template<class DVEC3_ITER>
        inline void push(DVEC3_ITER begin, DVEC3_ITER end);

        //! Applies changes to the dynanmic "style"
        void dirty();

        //! serialize as JSON string
        std::string to_json() const override;

    public: // NodeComponent

        int featureMask() const override;

    private:
        vsg::ref_ptr<detail::BindLineDescriptors> bindCommand;
        std::vector<vsg::ref_ptr<detail::LineGeometry>> geometries;
        friend class LineSystemNode;
    };

    // inline implementations
    template<class DVEC3_ITER> void Line::push(DVEC3_ITER begin, DVEC3_ITER end)
    {
        auto geom = detail::LineGeometry::create();
        for (DVEC3_ITER i = begin; i != end; ++i)
        {
            auto local = vsg::dvec3(i->x, i->y, i->z) - refPoint;
            geom->push_back(vsg::vec3(local));
        }
        geometries.push_back(geom);
    }
}

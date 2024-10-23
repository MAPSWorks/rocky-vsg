/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Mesh.h"
#include "json.h"
#include "engine/MeshSystem.h"
#include "engine/Runtime.h"
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/utils/ComputeBounds.h>

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::detail;

Mesh::Mesh()
{
    geometry = MeshGeometry::create();
}

void
Mesh::dirty()
{
    if (bindCommand)
    {
        // update the UBO with the new style data.
        if (style.has_value())
        {
            bindCommand->updateStyle(style.value());
        }
    }
}

int
Mesh::featureMask() const
{
    return MeshSystemNode::featureMask(*this);
}

void
Mesh::initializeNode(const ECS::NodeComponent::Params& params)
{
    auto cull = vsg::CullNode::create();

    vsg::ref_ptr<vsg::Group> parent;

    if (style.has_value() || texture)
    {
        bindCommand = BindMeshDescriptors::create();
        if (texture)
            bindCommand->_imageInfo = texture;
        dirty();
        bindCommand->init(params.layout);

        auto sg = vsg::StateGroup::create();
        sg->stateCommands.push_back(bindCommand);

        if (refPoint != vsg::dvec3())
        {
            auto mt = vsg::MatrixTransform::create(vsg::translate(refPoint));
            mt->addChild(geometry);
            sg->addChild(mt);
        }
        else
        {
            sg->addChild(geometry);
        }

        cull->child = sg;
    }
    else
    {
        if (refPoint != vsg::dvec3())
        {
            auto mt = vsg::MatrixTransform::create(vsg::translate(refPoint));
            mt->addChild(geometry);
            cull->child = mt;
        }
        else
        {
            cull->child = geometry;
        }
    }

    vsg::ComputeBounds cb;
    cull->child->accept(cb);
    cull->bound.set((cb.bounds.min + cb.bounds.max) * 0.5, vsg::length(cb.bounds.min - cb.bounds.max) * 0.5);

    node = cull;
}

JSON
Mesh::to_json() const
{
    ROCKY_SOFT_ASSERT(false, "Not yet implemented");

    auto j = json::parse(ECS::Component::to_json());
    // todo
    return j.dump();
}

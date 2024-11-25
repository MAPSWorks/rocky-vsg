/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <vsg/io/read.h>
#include <vsg/nodes/MatrixTransform.h>
#include <rocky/URI.h>
#include <rocky/vsg/Transform.h>

#include "helpers.h"
using namespace ROCKY_NAMESPACE;

auto Demo_Model = [](Application& app)
{
    static entt::entity entity = entt::null;
    static Status status;
    const double scale = 500000.0;

    if (status.failed())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Model load failed!");
        return;
    }

    if (entity == entt::null)
    {
        // Load model data from a URI
        URI uri("https://raw.githubusercontent.com/vsg-dev/vsgExamples/master/data/models/teapot.vsgt");
        auto result = uri.read(app.instance.io());
        status = result.status;
        if (status.failed())
            return;

        // Parse the model
        // this is a bit awkward but it works when the URI has an extension
        auto options = vsg::Options::create(*app.runtime().readerWriterOptions);
        auto extension = std::filesystem::path(uri.full()).extension();
        options->extensionHint = extension.empty() ? std::filesystem::path(result.value.contentType) : extension;
        std::stringstream in(result.value.data);
        auto node = vsg::read_cast<vsg::Node>(in, options);
        if (!node)
        {
            status = Status(Status::ResourceUnavailable, "Failed to parse model");
            return;
        }

        // New entity to host our model
        entity = app.registry.create();

        // The model component; we just set the node directly.
        auto& model = app.registry.emplace<NodeGraph>(entity);
        model.node = node;

        // A transform component to place and move it on the map
        auto& transform = app.registry.emplace<Transform>(entity);
        transform.setPosition(GeoPoint(SRS::WGS84, 50, 0, 250000));
        transform.localMatrix = vsg::scale(scale);
    }

    if (ImGuiLTable::Begin("model"))
    {
        bool visible = ecs::visible(app.registry, entity);
        if (ImGuiLTable::Checkbox("Show", &visible))
            ecs::setVisible(app.registry, entity, visible);

        auto& transform = app.registry.get<Transform>(entity);
        if (ImGuiLTable::SliderDouble("Latitude", &transform.position.y, -85.0, 85.0, "%.1lf"))
            transform.dirty();
        if (ImGuiLTable::SliderDouble("Longitude", &transform.position.x, -180.0, 180.0, "%.1lf"))
            transform.dirty();
        if (ImGuiLTable::SliderDouble("Altitude", &transform.position.z, 0.0, 2500000.0, "%.1lf"))
            transform.dirty();

        auto rot = util::quaternion_from_matrix<vsg::dquat>(transform.localMatrix);
        auto [pitch, roll, heading] = util::euler_degrees_from_quaternion(rot);

        if (ImGuiLTable::SliderDouble("Heading", &heading, -180.0, 180.0, "%.1lf"))
        {
            auto rot = util::quaternion_from_euler_degrees<vsg::dquat>(pitch, roll, heading);
            transform.localMatrix = vsg::scale(scale) * vsg::rotate(rot);
            transform.dirty();
        }

        if (ImGuiLTable::SliderDouble("Pitch", &pitch, -90.0, 90.0, "%.1lf"))
        {
            auto rot = util::quaternion_from_euler_degrees<vsg::dquat>(pitch, roll, heading);
            transform.localMatrix = vsg::scale(scale) * vsg::rotate(rot);
            transform.dirty();
        }

        ImGuiLTable::End();
    }
};

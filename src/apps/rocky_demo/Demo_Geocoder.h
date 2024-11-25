/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/vsg/Icon.h>
#include <rocky/vsg/Label.h>
#include <rocky/vsg/Mesh.h>
#include <rocky/Geocoder.h>

#include "helpers.h"
using namespace ROCKY_NAMESPACE;

auto Demo_Geocoder = [](Application& app)
{
    static entt::entity entity = entt::null;
    static Status status;
    static jobs::future<Result<std::vector<Feature>>> geocoding_task;
    static char input_buf[256];
    static LabelStyle label_style_point;
    static LabelStyle label_style_area;
    static FeatureView feature_view;

    if (status.failed())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Icon load failed");
        ImGui::TextColored(ImVec4(1, 0, 0, 1), status.message.c_str());
        return;
    }

    if (entity == entt::null)
    {
        // Load an icon image
        auto& io = app.io();
        auto image = io.services.readImageFromURI("https://readymap.org/readymap/filemanager/download/public/icons/placemark32.png", io);
        if (image.status.failed())
        {
            status = image.status;
            return;
        }

        // Make an entity to host our icon:
        entity = app.registry.create();

        // Attach the new Icon and set up its properties:
        auto& icon = app.registry.emplace<Icon>(entity);
        icon.image = image.value;
        icon.style = IconStyle{ 32, 0.0f }; // pixel size, rotation(radians)

        // Attach a label:
        label_style_point.font = app.runtime().defaultFont;
        label_style_point.horizontalAlignment = vsg::StandardLayout::LEFT_ALIGNMENT;
        label_style_point.pointSize = 26.0f;
        label_style_point.outlineSize = 0.5f;
        label_style_point.pixelOffset = { icon.style.size_pixels, 0.0f, 0.0f };

        label_style_area.font = app.runtime().defaultFont;
        label_style_area.horizontalAlignment = vsg::StandardLayout::CENTER_ALIGNMENT;
        label_style_area.pointSize = 26.0f;
        label_style_area.outlineSize = 0.5f;

        auto& label = app.registry.emplace<Label>(entity);

        // Outline for location boundary:
        feature_view.styles.line = LineStyle();
        feature_view.styles.line->color = vsg::vec4{ 1, 1, 0, 1 };
        feature_view.styles.line->depth_offset = 9000.0f; //meters

        app.registry.get<Visibility>(entity).active = false;
        //ecs::setVisible(app.registry, feature_view.entity, false);

        // Transform to place the entity:
        auto& xform = app.registry.emplace<Transform>(entity);
    }

    if (ImGuiLTable::Begin("geocoding"))
    {
        if (ImGuiLTable::InputText("Location:", input_buf, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            // hide the placemark:
            auto& icon = app.registry.get<Icon>(entity);
            app.registry.get<Visibility>(entity).active = false;

            std::string input(input_buf);
            geocoding_task = jobs::dispatch([&app, input](jobs::cancelable& c)
                {
                    Result<std::vector<Feature>> result;
                    if (!c.canceled())
                    {
                        Geocoder geocoder;
                        result = geocoder.geocode(input, app.io());
                    }
                    return result;
                });
        }
        ImGuiLTable::End();
    }

    if (geocoding_task.working())
    {
        ImGui::Text("Searching...");
    }

    else if (geocoding_task.available())
    {
        auto& result = geocoding_task.value();
        if (result.status.ok())
        {
            ImGui::Text("Click on a result to center:");
            for (auto& feature : result.value)
            {
                bool selected = false;
                ImGui::Separator();
                std::string display_name = feature.field("display_name").stringValue;
                ImGui::Selectable(display_name.c_str(), &selected);
                if (selected)
                {
                    app.onNextUpdate([&, display_name]()
                        {
                            auto extent = feature.extent;
                            if (extent.area() == 0.0)
                                extent.expand(Distance(10, Units::KILOMETERS), Distance(10, Units::KILOMETERS));

                            auto view = app.displayManager->windowsAndViews.begin()->second.front();
                            auto manip = MapManipulator::get(view);
                            if (manip)
                            {
                                Viewpoint vp = manip->getViewpoint();
                                vp.point = extent.centroid();
                                manip->setViewpoint(vp, std::chrono::seconds(2));
                            }

                            auto&& [icon, label] = app.registry.get<Icon, Label>(entity);

                            // show the placemark:
                            if (feature.geometry.type == Geometry::Type::Points)
                            {
                                app.registry.get<Visibility>(entity).active = true;
                                if (feature_view.entity != entt::null)
                                    app.registry.get<Visibility>(feature_view.entity).active = false;
                                label.style = label_style_point;
                            }
                            else
                            {
                                // update the mesh:
                                auto copy_of_feature = feature;
                                copy_of_feature.geometry.convertToType(Geometry::Type::LineString);
                                Geometry::iterator i(copy_of_feature.geometry);
                                while(i.hasMore()) for (auto& point : i.next().points) point.z = 500.0;
                                feature_view.clear(app.registry);
                                feature_view.features = { copy_of_feature };
                                feature_view.generate(app.registry, app.mapNode->worldSRS(), app.runtime());
                                app.registry.get<Visibility>(entity).active = true;
                                if (feature_view.entity != entt::null)
                                    app.registry.get<Visibility>(feature_view.entity).active = true;
                                label.style = label_style_area;
                            }

                            // update the label:
                            auto text = display_name;
                            replace_in_place(text, ", ", "\n");
                            label.text = text;                            
                            label.revision++;

                            // position it:
                            auto& xform = app.registry.get<Transform>(entity);
                            xform.setPosition(extent.centroid());
                        });
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Clear"))
            {
                geocoding_task.reset();
                input_buf[0] = (char)0;

                app.onNextUpdate([&]()
                    {
                        app.registry.get<Visibility>(entity).active = false;
                        if (feature_view.entity != entt::null)
                            app.registry.get<Visibility>(feature_view.entity).active = false;
                    });
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0.5, 0.5, 1), std::string("Geocoding failed! " + result.status.message).c_str());
        }
    }
};

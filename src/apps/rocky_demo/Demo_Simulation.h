/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/vsg/Icon.h>
#include <rocky/vsg/Transform.h>
#include <rocky/vsg/Motion.h>
#include <set>
#include <random>

#include "helpers.h"
using namespace ROCKY_NAMESPACE;

using namespace std::chrono_literals;

namespace
{
    // Simple simulation system runinng in its own thread.
    // It uses a MotionSystem to process Motion components and update
    // their corresponding Transform components.
    class Simulator
    {
    public:
        Application& app;
        MotionSystem motion;
        float hertz = 30.0f; // updates per second

        Simulator(Application& in_app) : app(in_app), motion(in_app.entities) { }

        void run()
        {
            jobs::context context;
            context.pool = jobs::get_pool("rocky.simulation", 1);
            jobs::dispatch([this]()
                {
                    while (app.active())
                    {
                        auto t0 = std::chrono::steady_clock::now();
                        motion.update(app.runtime());
                        app.runtime().requestFrame();
                        auto t1 = std::chrono::steady_clock::now();
                        auto sleep_time = std::chrono::duration<float>(1.0f / hertz);
                        std::this_thread::sleep_for(sleep_time - (t1 - t0));
                    }
                }, context);
        }
    };


    struct DeclutterData
    {
        vsg::dmat4 mvm;
        vsg::box box;
    };

    class DeclutterSystem : public ECS::System
    {
        DeclutterSystem(entt::registry& registry) : ECS::System(registry) { }

        void initializeSystem(Runtime& runtime)
        {
            //nop
        }

        void updateComponents(Runtime& runtime)
        {
            auto view = registry.view<Transform, DeclutterData>();

            view.each([this](const auto entity, auto& xform, auto& declutter)
                {
                    //nop
                });
        }
    };
}

auto Demo_Simulation = [](Application& app)
{
    // Make an entity for us to tether to and set it in motion
    static std::set<entt::entity> platforms;
    static Status status;
    static Simulator sim(app);
    const unsigned num_platforms = 10000;

    if (status.failed())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Image load failed!");
        ImGui::TextColored(ImVec4(1, 0, 0, 1), status.message.c_str());
        return;
    }

    const float s = 20.0;

    if (platforms.empty())
    {
        // add an icon:
        auto io = app.instance.io();
        auto image = io.services.readImageFromURI("https://github.com/gwaldron/osgearth/blob/master/data/airport.png?raw=true", io);
        status = image.status;
        if (image.status.ok())
        {
            std::mt19937 mt;
            std::uniform_real_distribution<float> rand_unit(0.0, 1.0);

            auto ll_to_ecef = SRS::WGS84.to(SRS::ECEF);

            for (unsigned i = 0; i < num_platforms; ++i)
            {
                // Create a host entity:
                auto entity = app.entities.create();

                auto& icon = app.entities.emplace<Icon>(entity);
                icon.style = IconStyle{ 16.0f + rand_unit(mt) * 16.0f, 0.0f }; // pixels, rotation(rad)

                if (image.status.ok())
                {
                    // attach an icon to the host:
                    icon.image = image.value;
                }

                double lat = -80.0 + rand_unit(mt) * 160.0;
                double lon = -180 + rand_unit(mt) * 360.0;
                double alt = 1000.0 + rand_unit(mt) * 150000.0;
                GeoPoint pos(SRS::WGS84, lon, lat, alt);

                // This is optional, since a Transform can take a point expresssed in any SRS.
                // But since we KNOW the map is geocentric, this will give us better performance
                // by avoiding a conversion every frame.
                pos.transformInPlace(SRS::ECEF);

                // Add a transform component:
                auto& transform = app.entities.emplace<Transform>(entity);
                transform.localTangentPlane = false;
                transform.setPosition(pos);

                // Add a motion component to represent movement.
                auto& motion = app.entities.emplace<Motion>(entity);
                motion.velocity = { -75000 + rand_unit(mt) * 150000, 0.0, 0.0 };

                // Label the platform.
                auto& label = app.entities.emplace<Label>(entity);
                label.text = std::to_string(i);
                label.style.font = app.runtime().defaultFont;
                label.style.pointSize = 16.0f;
                label.style.outlineSize = 0.2f;

                platforms.emplace(entity);
            }

            sim.run();
        }
    }

    ImGui::Text("Simulating %ld platforms", platforms.size());
    ImGuiLTable::Begin("sim");
    ImGuiLTable::SliderFloat("Update rate (hertz)", &sim.hertz, 1.0f, 120.0f, "%.0f");
    ImGuiLTable::End();
};

/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/vsg/Line.h>
#include <rocky/vsg/ECS.h>
#include "helpers.h"
#include <chrono>
#include <limits>
#include <list>

using namespace ROCKY_NAMESPACE;
using namespace std::chrono_literals;

namespace
{
    const int track_chunk_size = 16;

    // ECS Component
    struct TrackHistory
    {
        struct Chunk
        {
            GeoPoint referencePoint;
            entt::entity attach_point = entt::null; // attachment point for the line
            std::size_t numPoints = 0;
        };

        LineStyle style;
        unsigned maxPoints = 48;

    private:
        std::list<Chunk> chunks;
        std::size_t numChunks = 0;
        friend class TrackHistorySystem;
    };

    class TrackHistorySystem : public ecs::System
    {
    public:
        //! Construct a new system for managing TrackHistory components.
        static auto create(ecs::Registry& r) { 
            return std::make_shared<TrackHistorySystem>(r);
        }

        //! Construct a new system for managing TrackHistory components.
        //! Please call create(registry)
        TrackHistorySystem(ecs::Registry& registry) :
            System(registry)
        {
            // destruction of a TrackHistory requries some extra work:
            registry.on_destroy<TrackHistory>().connect<&TrackHistorySystem::on_destroy>(this);
        }

        float update_hertz = 1.0f; // updates per second
        bool tracks_visible = true;

        void update(Runtime& runtime) override
        {
            //std::scoped_lock(std::shared_lock(registry.mutex));

            auto now = std::chrono::steady_clock::now();
            auto freq = 1s / update_hertz;
            auto elapsed = (now - last_update);

            if (elapsed >= freq)
            {
                auto view = registry.view<TrackHistory, Transform>();
                for (auto&& [entity, track, transform] : view.each())
                {
                    if (transform.position.valid())
                    {
                        if (track.numChunks == 0 || track.chunks.back().numPoints >= track_chunk_size)
                        {
                            createNewChunk(entity, track, transform);
                        }

                        updateChunk(entity, track, track.chunks.back(), transform);

                        // approximation for now.
                        auto maxChunks = track.maxPoints / track_chunk_size;
                        if (track.numChunks > 1u && track.numChunks > maxChunks)
                        {
                            registry.erase<Line>(track.chunks.front().attach_point);
                            track.chunks.erase(track.chunks.begin());
                            --track.numChunks;
                        }
                    }
                }

                last_update = now;
            }
        };

        void createNewChunk(entt::entity host_entity, TrackHistory& track, Transform& transform)
        {
            // Make a new chunk and set its reference point
            auto& new_chunk = track.chunks.emplace_back();
            ++track.numChunks;

            new_chunk.attach_point = registry.create();
            new_chunk.referencePoint = transform.position;

            // Each chunk get a line primitive.
            auto& line = registry.emplace<Line>(new_chunk.attach_point);
            line.style = track.style;
            line.referencePoint = new_chunk.referencePoint;
            line.points.reserve(track_chunk_size);

            // Tie track visibility to host visibility:
            auto& track_visibility = registry.emplace<Visibility>(new_chunk.attach_point);
            updateVisibility(host_entity, new_chunk);

            // If this is not the first chunk, connect it to the previous one
            if (track.numChunks > 1)
            {
                auto prev_chunk = std::prev(std::prev(track.chunks.end()));
                auto& prev_line = registry.get<Line>(prev_chunk->attach_point);
                line.points.emplace_back(prev_line.points.back());
                ++new_chunk.numPoints;
            }

            // pre-allocates space for all future points
            line.staticSize = track_chunk_size;
        }

        void updateChunk(entt::entity host_entity, TrackHistory& track, TrackHistory::Chunk& chunk, Transform& transform)
        {
            auto& line = registry.get<Line>(chunk.attach_point);

            if (chunk.numPoints > 0 && line.points.back() == to_vsg((glm::dvec3)(transform.position)))
                return;

            // append the new position:
            line.points.emplace_back(to_vsg((glm::dvec3)transform.position));
            ++chunk.numPoints;
            ++line.revision;
        }

        void updateVisibility(entt::entity host_entity, TrackHistory::Chunk& chunk)
        {
            auto& track_visibility = registry.get<Visibility>(chunk.attach_point);
            if (tracks_visible)
                track_visibility.parent = &registry.get<Visibility>(host_entity);
            else
                track_visibility.parent = nullptr, track_visibility.setAll(false);
        }

        void updateVisibility()
        {
            // ensure that the visibility of a track matches the visibility of its host.
            auto view = registry.view<TrackHistory>();
            for (auto&& [host_entity, track] : view.each())
            {
                for (auto& chunk : track.chunks)
                {
                    updateVisibility(host_entity, chunk);
                }
            }
        }

        void reset()
        {
            //std::scoped_lock(std::unique_lock(app.registry_mutex));

            // first delete any existing track histories
            registry.clear<TrackHistory>();

            // style our tracks.
            LineStyle style;
            style.color = vsg::vec4{ 0.0f, 1.0f, 0.0f, 1.0f };
            style.width = 2.0f;

            // then re-scan and add new ones.
            auto view = registry.view<Transform>();
            for (auto&& [entity, transform] : view.each())
            {
                if (transform.parent == nullptr)
                {
                    auto& track = registry.emplace<TrackHistory>(entity);
                    track.style = style;
                }
            }
        }

    protected:

        std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();

        // called by EnTT when a TrackHistory component is destroyed.
        void on_destroy(entt::registry& registry, entt::entity entity)
        {
            auto& track = registry.get<TrackHistory>(entity);
            for (auto& chunk : track.chunks)
            {
                registry.destroy(chunk.attach_point);
            }
        }
    };
}


auto Demo_TrackHistory = [](Application& app)
{
    static bool init = false;
    static std::shared_ptr<TrackHistorySystem> system;

    if (!system)
    {
        // make a System to handle track histories, and add it to the app.
        system = TrackHistorySystem::create(app.registry);
        app.ecsManager->add(system);
        system->reset();
    }

    if (ImGuiLTable::Begin("track history"))
    {
        if (ImGuiLTable::Checkbox("Show", &system->tracks_visible))
        {
            system->updateVisibility();
        }
        ImGuiLTable::SliderFloat("Update frequency", &system->update_hertz, 1.0f, 15.0f);
        ImGuiLTable::End();
    }

    ImGui::Separator();
    if (ImGui::Button("Reset"))
    {
        system->reset();
    }
};

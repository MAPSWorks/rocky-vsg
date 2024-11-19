/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/vsg/InstanceVSG.h>
#include <rocky/vsg/MapNode.h>
#include <rocky/vsg/SkyNode.h>
#include <rocky/vsg/ECSNodes.h>
#include <rocky/vsg/DisplayManager.h>

#include <vsg/app/Viewer.h>
#include <vsg/app/Window.h>
#include <vsg/app/RenderGraph.h>
#include <vsg/app/CommandGraph.h>
#include <vsg/app/View.h>
#include <vsg/nodes/Group.h>

#include <chrono>
#include <list>
#include <mutex>

namespace ROCKY_NAMESPACE
{
    class ROCKY_EXPORT Application
    {
    public:
        //! Construct a new application object
        Application();

        //! Construct a new application object
        //! @param argc Number of command line arguments, including the exe name
        //! @param argv Command line arguments
        Application(int& argc, char** argv);

        //! Run until exit.
        //! @return exit code, 0 = no error
        int run();

        //! Process and render one frame. If you call run(), this will
        //! happen automatically in a continuous loop.
        //! @return True upon success; false to quit the application.
        bool frame();

        //! Queues a function to run during the next update cycle,
        //! during which it's safe to modify the scene qgraph and the
        //! display manager.
        void onNextUpdate(std::function<void()> func);

        //! Sets a custom view to use for this application.
        void setViewer(vsg::ref_ptr<vsg::Viewer> viewer);

        //! About the application. Lists all the dependencies and their versions.
        std::string about() const;

        //! True if the debug validation layer is active, which will affect performance
        bool debugLayerOn() const {
            return _debuglayer;
        }

        //! Creates the default window. This is called automatically by run() if you
        //! don't call it yourself. You may need to call this yourself if you plan to
        //! access windows, views, or manipulators before starting the frame loop.
        void realize();

        //! Shortcut to the instance's IOOptions
        IOOptions& io() {
            return instance.io();
        }
        
        //! Shortcut to the Runtime interface
        Runtime& runtime() {
            return instance.runtime();
        }

        bool active() const {
            return _lastFrameOK;
        }

    public: // public properties

        jobs::detail::semaphore handle;

        ecs::Registry registry;

        rocky::InstanceVSG instance;
        vsg::ref_ptr<rocky::MapNode> mapNode;
        vsg::ref_ptr<rocky::SkyNode> skyNode;
        vsg::ref_ptr<vsg::Viewer> viewer;
        vsg::ref_ptr<vsg::Group> root;
        vsg::ref_ptr<vsg::Group> mainScene;
        vsg::ref_ptr<ecs::ECSNode> ecsManager;
        std::shared_ptr<DisplayManager> displayManager;
        BackgroundServices backgroundServices;
        bool autoCreateWindow = true;
        Status commandLineStatus;

        // user function to call during update and before event handling
        std::function<void()> updateFunction;

        // in renderOnDemand mode, user function to call when not rendering a frame
        std::function<void()> noRenderFunction;

        //! Runtime timing statistics
        struct Stats
        {
            std::chrono::microseconds frame;
            std::chrono::microseconds events;
            std::chrono::microseconds update;
            std::chrono::microseconds record;
            std::chrono::microseconds present;
            double memory;

        };
        Stats stats;

    public:
        //! Copy construction is disabled.
        Application(const Application&) = delete;

        //! Move construction is disabled.
        Application(Application&&) = delete;

        //! Destructor
        ~Application();

    private:
        bool _apilayer = false;
        bool _debuglayer = false;
        bool _vsync = true;
        bool _multithreaded = true;
        bool _viewerRealized = false;
        int _framesSinceLastRender = 0; // for renderOnDemand
        bool _lastFrameOK = true;

        void ctor(int& argc, char** argv);

        void setupViewer(vsg::ref_ptr<vsg::Viewer> viewer);

        friend class DisplayManager;
    };
}


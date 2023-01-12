/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky_vsg/Common.h>
#include <rocky/Instance.h>
#include <rocky/IOTypes.h>
#include <rocky/Threading.h>

#include <vsg/core/observer_ptr.h>
#include <vsg/app/CompileManager.h>
#include <vsg/app/UpdateOperations.h>
#include <vsg/threading/OperationThreads.h>
#include <vsg/utils/SharedObjects.h>

namespace vsg
{
    class Viewer;
    class Group;
    class Node;
}

namespace ROCKY_NAMESPACE
{
    /**
     * Access layer to viewer, rendering, scene graph, and 
     * other runtime operations.
     */
    class ROCKY_VSG_EXPORT RuntimeContext
    {
    public:
        //! Compiler for new vsg objects
        std::function<vsg::CompileManager*()> compiler;

        //! Update operations queue
        std::function<vsg::UpdateOperations*()> updates;

        //! Pool of threads used to load terrain data
        vsg::ref_ptr<vsg::OperationThreads> loaders;

        //! VSG state sharing
        vsg::ref_ptr<vsg::SharedObjects> sharedObjects;

        //! Function that creates a node
        using NodeFactory = std::function<vsg::ref_ptr<vsg::Node>(Cancelable&)>;

        //! Schedules data creation; the resulting node or nodes 
        //! get added to "parent" if the operation suceeds.
        //! Returns a future you can check for completion.
        util::Future<bool> compileAndAddNode(
            vsg::Group* parent,
            NodeFactory factory);

        //! Safely removes a node from the scene graph (async)
        void removeNode(
            vsg::Group* parent,
            unsigned index);

        //! Runs two function, first an asychronous function that returns
        //! a piece of data; then a synchronous function (during the
        //! viewer's 'update' phase). For example, you can load data
        //! asynchronously and then merge it into the scene graph
        //! synchronously.
        template<class T>
        util::Future<bool> runAsyncUpdateSync(
            std::function<Result<T>(Cancelable&)> async,
            std::function<void(const T&, Cancelable&)> sync);
    };


    // inlines

    template<class T>
    util::Future<bool> RuntimeContext::runAsyncUpdateSync(
        std::function<Result<T>(Cancelable&)> async,
        std::function<void(const T&, Cancelable&)> sync)
    {
        struct TwoStepOp : public vsg::Inherit<vsg::Operation, TwoStepOp> {
            RuntimeContext* _runtime;
            std::function<Result<T>(Cancelable&)> _async;
            std::function<void(const T&, Cancelable&)> _sync;
            Result<T> _result;
            util::Promise<bool> _promise;
            void run() override {
                if (!_promise.abandoned()) {
                    if (!_result.status.ok()) {
                        // step 1: run the async function and set the status
                        _result = _async(_promise);
                        if (_result.status.ok()) {
                            _runtime->updates()->add(vsg::ref_ptr<vsg::Operation>(this));
                        } 
                        else {
                            _promise.resolve(false);
                        }
                    }
                    else {
                        // step 2: run the sync function and resolve the promise
                        _sync(_result.value, _promise);
                        _promise.resolve(true);
                    }
                }
            }
        };

        auto op = TwoStepOp::create();
        op->_runtime = this;
        op->_async = async;
        op->_sync = sync;
        auto result = op->_promise.getFuture();
        this->loaders->add(op);
        return result;
    }

}


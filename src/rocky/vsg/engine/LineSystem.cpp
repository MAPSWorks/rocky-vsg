/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "LineSystem.h"
#include "Runtime.h"
#include "PipelineState.h"

#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/ViewDependentState.h>
#include <vsg/commands/DrawIndexed.h>

#include <vsg/nodes/CullNode.h>
#include <vsg/utils/ComputeBounds.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/StateGroup.h>

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::detail;

#define LINE_VERT_SHADER "shaders/rocky.line.vert"
#define LINE_FRAG_SHADER "shaders/rocky.line.frag"

#define LINE_BUFFER_SET 0 // must match layout(set=X) in the shader UBO
#define LINE_BUFFER_BINDING 1 // must match the layout(binding=X) in the shader UBO (set=0)

namespace
{
    vsg::ref_ptr<vsg::ShaderSet> createLineShaderSet(Runtime& runtime)
    {
        vsg::ref_ptr<vsg::ShaderSet> shaderSet;

        // load shaders
        auto vertexShader = vsg::ShaderStage::read(
            VK_SHADER_STAGE_VERTEX_BIT,
            "main",
            vsg::findFile(LINE_VERT_SHADER, runtime.searchPaths),
            runtime.readerWriterOptions);

        auto fragmentShader = vsg::ShaderStage::read(
            VK_SHADER_STAGE_FRAGMENT_BIT,
            "main",
            vsg::findFile(LINE_FRAG_SHADER, runtime.searchPaths),
            runtime.readerWriterOptions);

        if (!vertexShader || !fragmentShader)
        {
            return { };
        }

        vsg::ShaderStages shaderStages{ vertexShader, fragmentShader };

        shaderSet = vsg::ShaderSet::create(shaderStages);

        // "binding" (3rd param) must match "layout(location=X) in" in the vertex shader
        shaderSet->addAttributeBinding("in_vertex", "", 0, VK_FORMAT_R32G32B32_SFLOAT, { });
        shaderSet->addAttributeBinding("in_vertex_prev", "", 1, VK_FORMAT_R32G32B32_SFLOAT, {});
        shaderSet->addAttributeBinding("in_vertex_next", "", 2, VK_FORMAT_R32G32B32_SFLOAT, {});
        shaderSet->addAttributeBinding("in_color", "", 3, VK_FORMAT_R32G32B32A32_SFLOAT, {});

        // line data uniform buffer (width, stipple, etc.)
        shaderSet->addDescriptorBinding("line", "", LINE_BUFFER_SET, LINE_BUFFER_BINDING,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, {});

        // We need VSG's view-dependent data:
        PipelineUtils::addViewDependentData(shaderSet, VK_SHADER_STAGE_VERTEX_BIT);

        // Note: 128 is the maximum size required by the Vulkan spec so don't increase it
        shaderSet->addPushConstantRange("pc", "", VK_SHADER_STAGE_VERTEX_BIT, 0, 128);

        return shaderSet;
    }
}

LineSystemNode::LineSystemNode(entt::registry& registry) :
    vsg::Inherit<ECS::SystemNode, LineSystemNode>(registry),
    helper(registry)
{
    helper.initializeComponent = [this](Line& line, InitContext& context) {
        this->initializeComponent(line, context);
    };
}

void
LineSystemNode::initializeSystem(Runtime& runtime)
{
    // Now create the pipeline and stategroup to bind it
    auto shaderSet = createLineShaderSet(runtime);

    if (!shaderSet)
    {
        status = Status(Status::ResourceUnavailable,
            "Line shaders are missing or corrupt. "
            "Did you set ROCKY_FILE_PATH to point at the rocky share folder?");
        return;
    }

    helper.pipelines.resize(NUM_PIPELINES);

    for (int feature_mask = 0; feature_mask < NUM_PIPELINES; ++feature_mask)
    {
        auto& c = helper.pipelines[feature_mask];

        // Create the pipeline configurator for terrain; this is a helper object
        // that acts as a "template" for terrain tile rendering state.
        c.config = vsg::GraphicsPipelineConfig::create(shaderSet);

        // Apply any custom compile settings / defines:
        c.config->shaderHints = runtime.shaderCompileSettings;

        // activate the arrays we intend to use
        c.config->enableArray("in_vertex", VK_VERTEX_INPUT_RATE_VERTEX, 12);
        c.config->enableArray("in_vertex_prev", VK_VERTEX_INPUT_RATE_VERTEX, 12);
        c.config->enableArray("in_vertex_next", VK_VERTEX_INPUT_RATE_VERTEX, 12);
        c.config->enableArray("in_color", VK_VERTEX_INPUT_RATE_VERTEX, 16);

        // Uniforms we will need:
        c.config->enableDescriptor("line");

        // always both
        PipelineUtils::enableViewDependentData(c.config);

        struct SetPipelineStates : public vsg::Visitor
        {
            int feature_mask;
            SetPipelineStates(int feature_mask_) : feature_mask(feature_mask_) { }
            void apply(vsg::Object& object) override {
                object.traverse(*this);
            }
            void apply(vsg::RasterizationState& state) override {
                state.cullMode = VK_CULL_MODE_NONE;
            }
            void apply(vsg::DepthStencilState& state) override {
                if ((feature_mask & WRITE_DEPTH) == 0) {
                    state.depthWriteEnable = (feature_mask & WRITE_DEPTH) ? VK_TRUE : VK_FALSE;
                }
            }
            void apply(vsg::ColorBlendState& state) override {
                state.attachments = vsg::ColorBlendState::ColorBlendAttachments
                {
                    { true,
                      VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                      VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT }
                };
            }
        };
        SetPipelineStates visitor(feature_mask);
        c.config->accept(visitor);

        c.config->init();

        // Assemble the commands required to activate this pipeline:
        c.commands = vsg::Commands::create();
        c.commands->children.push_back(c.config->bindGraphicsPipeline);
        c.commands->children.push_back(PipelineUtils::createViewDependentBindCommand(c.config));
    }
}

void
LineSystemNode::initializeComponent(Line& line, InitContext& context)
{
    auto cull = vsg::CullNode::create();

    vsg::ref_ptr<vsg::Group> geom_parent;

    if (line.style.has_value())
    {
        line.bindCommand = BindLineDescriptors::create();
        line.dirty();
        line.bindCommand->init(context.layout);

        auto sg = vsg::StateGroup::create();
        sg->stateCommands.push_back(line.bindCommand);
        geom_parent = sg;

        if (line.refPoint != vsg::dvec3())
        {
            auto localizer = vsg::MatrixTransform::create(vsg::translate(line.refPoint));
            localizer->addChild(sg);
            cull->child = localizer;
        }
        else
        {
            cull->child = sg;
        }
    }
    else
    {
        if (line.refPoint != vsg::dvec3())
        {
            auto localizer = vsg::MatrixTransform::create(vsg::translate(line.refPoint));
            cull->child = localizer;
            geom_parent = localizer;
        }
        else
        {
            auto group = vsg::Group::create();
            cull->child = group;
            geom_parent = group;
        }
    }

    for (auto& geom : line.geometries)
    {
        geom_parent->addChild(geom);
    }

    vsg::ComputeBounds cb;
    cull->child->accept(cb);
    cull->bound.set((cb.bounds.min + cb.bounds.max) * 0.5, vsg::length(cb.bounds.min - cb.bounds.max) * 0.5);

    line.node = cull;
}

int LineSystemNode::featureMask(const Line& c)
{
    int mask = 0;
    if (c.write_depth) mask |= WRITE_DEPTH;
    return mask;
}


BindLineDescriptors::BindLineDescriptors()
{
    //nop
}

void
BindLineDescriptors::updateStyle(const LineStyle& value)
{
    if (!_styleData)
    {
        _styleData = vsg::ubyteArray::create(sizeof(LineStyle));

        // tells VSG that the contents can change, and if they do, the data should be
        // transfered to the GPU before or during recording.
        _styleData->properties.dataVariance = vsg::DYNAMIC_DATA;
    }

    LineStyle& my_style = *static_cast<LineStyle*>(_styleData->dataPointer());
    my_style = value;
    _styleData->dirty();
}

void
BindLineDescriptors::init(vsg::ref_ptr<vsg::PipelineLayout> layout)
{
    vsg::Descriptors descriptors;

    // the style buffer:
    auto ubo = vsg::DescriptorBuffer::create(_styleData, LINE_BUFFER_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    descriptors.push_back(ubo);

    if (!descriptors.empty())
    {
        this->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        this->firstSet = 0;
        this->layout = layout;
        this->descriptorSet = vsg::DescriptorSet::create(
            layout->setLayouts.front(),
            descriptors);
    }
}


LineGeometry::LineGeometry()
{
    _drawCommand = vsg::DrawIndexed::create(
        0, // index count
        1, // instance count
        0, // first index
        0, // vertex offset
        0  // first instance
    );
}

void
LineGeometry::setFirst(unsigned value)
{
    _drawCommand->firstIndex = value * 4;
}

void
LineGeometry::setCount(unsigned value)
{
    _drawCommand->indexCount = value;
}

unsigned
LineGeometry::numVerts() const
{
    return (unsigned)_current.size() / 4;
}

void
LineGeometry::push_back(const vsg::vec3& value)
{
    bool first = _current.empty();

    _previous.push_back(first ? value : _current.back());
    _previous.push_back(first ? value : _current.back());
    _previous.push_back(first ? value : _current.back());
    _previous.push_back(first ? value : _current.back());

    if (!first)
    {
        *(_next.end() - 4) = value;
        *(_next.end() - 3) = value;
        *(_next.end() - 2) = value;
        *(_next.end() - 1) = value;
    }

    _current.push_back(value);
    _current.push_back(value);
    _current.push_back(value);
    _current.push_back(value);

    _next.push_back(value);
    _next.push_back(value);
    _next.push_back(value);
    _next.push_back(value);

    _colors.push_back(_defaultColor);
    _colors.push_back(_defaultColor);
    _colors.push_back(_defaultColor);
    _colors.push_back(_defaultColor);
}

void
LineGeometry::compile(vsg::Context& context)
{
    if (commands.empty())
    {
        if (_current.size() == 0)
            return;

        auto vert_array = vsg::vec3Array::create(_current.size(), _current.data());
        auto prev_array = vsg::vec3Array::create(_previous.size(), _previous.data());
        auto next_array = vsg::vec3Array::create(_next.size(), _next.data());
        auto colors_array = vsg::vec4Array::create(_colors.size(), _colors.data());

        unsigned numIndices = (numVerts() - 1) * 6;
        auto indices = vsg::uintArray::create(numIndices);
        for (int e = 2, i = 0; e < _current.size() - 2; e += 4)
        {
            (*indices)[i++] = e + 3;
            (*indices)[i++] = e + 1;
            (*indices)[i++] = e + 0; // provoking vertex
            (*indices)[i++] = e + 2;
            (*indices)[i++] = e + 3;
            (*indices)[i++] = e + 0; // provoking vertex
        }

        assignArrays({ vert_array, prev_array, next_array, colors_array });
        assignIndices(indices);

        _drawCommand->indexCount = (uint32_t)indices->size();

        commands.clear();
        commands.push_back(_drawCommand);
    }

    vsg::Geometry::compile(context);
}
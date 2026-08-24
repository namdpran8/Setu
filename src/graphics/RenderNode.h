#pragma once
#include "Canvas.h"
#include <vector>
#include <memory>
#include <string>

namespace setu {
namespace graphics {

// Base class for all recorded drawing operations
class DrawCommand {
public:
    virtual ~DrawCommand() = default;
    virtual void execute(Canvas& canvas) = 0;
};

// Represents a display list of drawing commands for a single View
class RenderNode {
public:
    RenderNode() = default;
    ~RenderNode() = default;

    void addCommand(std::unique_ptr<DrawCommand> command) {
        mCommands.push_back(std::move(command));
    }

    void clear() {
        mCommands.clear();
    }

    void draw(Canvas& canvas) {
        for (const auto& cmd : mCommands) {
            cmd->execute(canvas);
        }
    }

private:
    std::vector<std::unique_ptr<DrawCommand>> mCommands;
};

// Specific draw commands
class SaveCommand : public DrawCommand {
public:
    void execute(Canvas& canvas) override { canvas.save(); }
};

class RestoreCommand : public DrawCommand {
public:
    void execute(Canvas& canvas) override { canvas.restore(); }
};

class TranslateCommand : public DrawCommand {
public:
    TranslateCommand(float dx, float dy) : dx(dx), dy(dy) {}
    void execute(Canvas& canvas) override { canvas.translate(dx, dy); }
private:
    float dx, dy;
};

class ScaleCommand : public DrawCommand {
public:
    ScaleCommand(float sx, float sy) : sx(sx), sy(sy) {}
    void execute(Canvas& canvas) override { canvas.scale(sx, sy); }
private:
    float sx, sy;
};

class ClipRectCommand : public DrawCommand {
public:
    ClipRectCommand(float l, float t, float r, float b) : l(l), t(t), r(r), b(b) {}
    void execute(Canvas& canvas) override { canvas.clipRect(l, t, r, b); }
private:
    float l, t, r, b;
};

class DrawColorCommand : public DrawCommand {
public:
    DrawColorCommand(uint32_t c) : color(c) {}
    void execute(Canvas& canvas) override { canvas.drawColor(color); }
private:
    uint32_t color;
};

class DrawRectCommand : public DrawCommand {
public:
    DrawRectCommand(float l, float t, float r, float b, const Paint& p) : l(l), t(t), r(r), b(b), paint(p) {}
    void execute(Canvas& canvas) override { canvas.drawRect(l, t, r, b, paint); }
private:
    float l, t, r, b;
    Paint paint;
};

class DrawRoundRectCommand : public DrawCommand {
public:
    DrawRoundRectCommand(float l, float t, float r, float b, float rx, float ry, const Paint& p) 
        : l(l), t(t), r(r), b(b), rx(rx), ry(ry), paint(p) {}
    void execute(Canvas& canvas) override { canvas.drawRoundRect(l, t, r, b, rx, ry, paint); }
private:
    float l, t, r, b, rx, ry;
    Paint paint;
};

class DrawLineCommand : public DrawCommand {
public:
    DrawLineCommand(float sx, float sy, float ex, float ey, const Paint& p) 
        : sx(sx), sy(sy), ex(ex), ey(ey), paint(p) {}
    void execute(Canvas& canvas) override { canvas.drawLine(sx, sy, ex, ey, paint); }
private:
    float sx, sy, ex, ey;
    Paint paint;
};

class DrawTextCommand : public DrawCommand {
public:
    DrawTextCommand(const std::wstring& txt, float x, float y, const Paint& p)
        : text(txt), x(x), y(y), paint(p) {}
    void execute(Canvas& canvas) override { canvas.drawText(text, x, y, paint); }
private:
    std::wstring text;
    float x, y;
    Paint paint;
};

class DrawPathCommand : public DrawCommand {
public:
    DrawPathCommand(const Path& p, const Paint& pt) : path(p), paint(pt) {}
    void execute(Canvas& canvas) override { canvas.drawPath(path, paint); }
private:
    // Held by value like Paint: a Path is just two vectors of floats, and the
    // drawable that produced it may be gone by the time the list is replayed.
    Path path;
    Paint paint;
};

class DrawRenderNodeCommand : public DrawCommand {
public:
    DrawRenderNodeCommand(RenderNode* node) : renderNode(node) {}
    void execute(Canvas& canvas) override { 
        if (renderNode) {
            renderNode->draw(canvas);
        }
    }
private:
    RenderNode* renderNode;
};

} // namespace graphics
} // namespace setu

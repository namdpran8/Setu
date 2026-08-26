#include "RecordingCanvas.h"

namespace setu {
namespace graphics {

RecordingCanvas::RecordingCanvas(RenderNode* node) : mNode(node) {
}

void RecordingCanvas::save() {
    mNode->addCommand(std::make_unique<SaveCommand>());
}

void RecordingCanvas::restore() {
    mNode->addCommand(std::make_unique<RestoreCommand>());
}

void RecordingCanvas::translate(float dx, float dy) {
    mNode->addCommand(std::make_unique<TranslateCommand>(dx, dy));
}

void RecordingCanvas::scale(float sx, float sy) {
    mNode->addCommand(std::make_unique<ScaleCommand>(sx, sy));
}

void RecordingCanvas::clipRect(float left, float top, float right, float bottom) {
    mNode->addCommand(std::make_unique<ClipRectCommand>(left, top, right, bottom));
}

void RecordingCanvas::drawColor(uint32_t color) {
    mNode->addCommand(std::make_unique<DrawColorCommand>(color));
}

void RecordingCanvas::drawRect(float left, float top, float right, float bottom, const Paint& paint) {
    mNode->addCommand(std::make_unique<DrawRectCommand>(left, top, right, bottom, paint));
}

void RecordingCanvas::drawRoundRect(float left, float top, float right, float bottom, float rx, float ry, const Paint& paint) {
    mNode->addCommand(std::make_unique<DrawRoundRectCommand>(left, top, right, bottom, rx, ry, paint));
}

void RecordingCanvas::drawLine(float startX, float startY, float stopX, float stopY, const Paint& paint) {
    mNode->addCommand(std::make_unique<DrawLineCommand>(startX, startY, stopX, stopY, paint));
}

void RecordingCanvas::drawText(const std::wstring& text, float x, float y, const Paint& paint) {
    mNode->addCommand(std::make_unique<DrawTextCommand>(text, x, y, paint));
}

void RecordingCanvas::drawPath(const Path& path, const Paint& paint) {
    mNode->addCommand(std::make_unique<DrawPathCommand>(path, paint));
}

void RecordingCanvas::drawBitmap(const Bitmap& bitmap, const RectF& src, const RectF& dst,
                                 const Paint& paint) {
    // Take a share of the bitmap rather than a copy of it, so the display list can
    // outlive the Drawable that recorded this and still have pixels to replay.
    //
    // shared_from_this on a const Bitmap yields shared_ptr<const Bitmap>, which is
    // exactly what the command wants: replaying a recorded draw only ever reads the
    // image. It cannot throw here either - Bitmap's constructor is private and every
    // factory hands back a shared_ptr, so a Bitmap that no shared_ptr already owns
    // cannot be constructed.
    mNode->addCommand(
        std::make_unique<DrawBitmapCommand>(bitmap.shared_from_this(), src, dst, paint));
}

void RecordingCanvas::drawRenderNode(RenderNode* node) {
    mNode->addCommand(std::make_unique<DrawRenderNodeCommand>(node));
}

} // namespace graphics
} // namespace setu

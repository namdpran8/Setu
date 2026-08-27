/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "../graphics/drawable/Drawable.h"
#include "../view/View.h"

namespace setu {
namespace graphics {
class Bitmap;
}
namespace widget {

// android.widget.ImageView: a View whose content is a Drawable rather than text.
//
// Now that DrawableInflater decodes rasters, android:src on an ImageView is the
// most direct route from an APK's res/drawable-*/ to the screen, and the two jobs
// this class does are the ones that decide whether the result looks right:
//
//   * Measurement comes from the drawable's intrinsic size. A wrap_content
//     ImageView is as big as its image (plus padding), which the old stub's flat
//     100x100 could only ever be by accident.
//
//   * android:scaleType decides how a mismatch between that intrinsic size and the
//     final content box is resolved. The drawable is drawn at its own size with the
//     canvas transformed around it, rather than by handing the drawable a stretched
//     bounds rect - which is what keeps a nine-patch's corners intact and a
//     centerCrop image un-squashed.
//
// The one exception is FIT_XY, where the drawable *is* handed the whole content box
// and told to fill it, exactly as AOSP does: for a NinePatchDrawable that is the
// difference between stretching only the stretchable regions and stretching all of
// it, and FIT_XY is the scale type that asks for the latter.
//
// Deliberately absent: android:tint and android:tintMode (no ColorFilter in this
// runtime), android:baseline and android:baselineAlignBottom (nothing does baseline
// alignment yet), and android:src as a Uri. ScaleType::MATRIX is accepted but
// behaves as the identity, which is all it can ever be here - see setScaleType.
class ImageView : public view::View {
public:
    // android:scaleType. The values are AOSP's enum ordinals from attrs.xml, which
    // is what AAPT compiles the attribute down to, so the int off the wire is used
    // as this enum directly. Do not renumber: fitCenter and centerCrop would swap
    // places silently, and nothing would log.
    enum class ScaleType : int {
        MATRIX = 0,
        FIT_XY = 1,
        FIT_START = 2,
        FIT_CENTER = 3,
        FIT_END = 4,
        CENTER = 5,
        CENTER_CROP = 6,
        CENTER_INSIDE = 7,
    };

    ImageView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser,
              uint32_t defStyleAttr, uint32_t defStyleRes);
    ImageView();
    ~ImageView() override;

    // The image. Takes ownership of the drawable's callback the way setBackground
    // does, so a stateful or animating one reports back to this view.
    void setImageDrawable(graphics::DrawablePtr drawable);
    graphics::Drawable* getDrawable() const { return mDrawable.get(); }
    const graphics::DrawablePtr& getImageDrawable() const { return mDrawable; }

    // @drawable/foo by ID, inflated through DrawableInflater. Needs the
    // ResourceManager this view was constructed with; a no-op with a log line on a
    // view built by the default constructor, which has none.
    void setImageResource(uint32_t resId);

    // Wraps the bitmap in a BitmapDrawable, as AOSP does. The target density is
    // left at the drawable's default, so a bitmap decoded outside the resource
    // pipeline is treated as already correct for this screen.
    void setImageBitmap(std::shared_ptr<const graphics::Bitmap> bitmap);

    void setScaleType(ScaleType scaleType);
    ScaleType getScaleType() const { return mScaleType; }

    // android:adjustViewBounds. Lets the view's own measured size change to
    // preserve the image's aspect ratio, but only along an axis the parent left
    // free - an EXACTLY spec still wins. Turning it on also forces FIT_CENTER,
    // which is AOSP's behaviour and not just a default: the aspect-matched box is
    // computed from the intrinsic ratio, so any other scale type would fight it.
    void setAdjustViewBounds(bool adjustViewBounds);
    bool getAdjustViewBounds() const { return mAdjustViewBounds; }

    // android:maxWidth/maxHeight. Only consulted while adjustViewBounds is on,
    // matching AOSP - which is why the pair is the standard idiom for "scale this
    // photo down to fit, but no taller than 200dp".
    void setMaxWidth(int maxWidth);
    void setMaxHeight(int maxHeight);
    int getMaxWidth() const { return mMaxWidth; }
    int getMaxHeight() const { return mMaxHeight; }

    // android:cropToPadding. Off by default, as in AOSP, which means a centerCrop
    // image legitimately paints over this view's own padding. On, the padding box
    // clips it.
    void setCropToPadding(bool cropToPadding);
    bool getCropToPadding() const { return mCropToPadding; }

    // 0..255, multiplied into the image only - not into the background, and not
    // into this view's children. AOSP's setImageAlpha.
    void setImageAlpha(int alpha);
    int getImageAlpha() const { return mImageAlpha; }

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;
    void onDraw(graphics::Canvas& canvas) override;

    // The image drawable's intrinsic size can change under us - a BitmapDrawable's
    // does when its target density is set - and the transform is computed from it,
    // so an invalidation is also the cue to recheck it.
    void invalidateDrawable(graphics::Drawable* who) override;

    std::string getClassName() const override { return "ImageView"; }

protected:
    // Pushes the view's state into the image, so an android:src naming a
    // <selector> tracks pressed/disabled the way a background does.
    void drawableStateChanged() override;

    // True when android:scaleType was present in the layout or the style.
    // ImageButton defaults to CENTER and must not clobber an authored value, and
    // this constructor has already run by the time it gets a chance to set it -
    // the same problem, and the same fix, as TextView's mGravitySetFromXml.
    bool mScaleTypeSetFromXml = false;

    // Kept so setImageResource() can inflate after construction. View retains
    // neither, and no other widget here needs to: they read what they want in
    // their constructors and never look at a resource again.
    ResourceManager* mResManager = nullptr;
    Theme* mTheme = nullptr;

private:
    // The transform from the drawable's own bounds into the content box: scale
    // about the origin, then translate. That is the whole of AOSP's mDrawMatrix for
    // every scale type except MATRIX, because none of the others rotates or skews.
    struct DrawTransform {
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float transX = 0.0f;
        float transY = 0.0f;
    };

    // The callback/visibility/state handover shared by every setImage* entry point.
    void updateDrawable(graphics::DrawablePtr drawable);

    // AOSP's configureBounds: sizes the drawable and derives mDrawTransform from
    // the scale type. Driven by layout rather than by draw, so the transform is
    // computed once per size change instead of once per frame.
    void configureBounds();

    // AOSP's resolveAdjustedSize. Distinct from View::resolveSize in that an
    // UNSPECIFIED spec is still capped by mMaxWidth/mMaxHeight.
    int resolveAdjustedSize(int desiredSize, int maxSize, int measureSpec) const;

    // AOSP's getSuggestedMinimumWidth/Height, which View does not expose here. The
    // background contributes its own minimum, so an ImageView with no image is
    // still as big as the chrome behind it - which is what stops a styled
    // ImageButton with no src from measuring to zero.
    int suggestedMinimumWidth() const;
    int suggestedMinimumHeight() const;

    graphics::DrawablePtr mDrawable;

    ScaleType mScaleType = ScaleType::FIT_CENTER;

    // getIntrinsicWidth/Height as of the last time the drawable changed. Cached
    // because onMeasure and configureBounds both want them, and because comparing
    // against them is how invalidateDrawable notices a resize.
    int mDrawableWidth = -1;
    int mDrawableHeight = -1;

    int mMaxWidth = (std::numeric_limits<int>::max)();
    int mMaxHeight = (std::numeric_limits<int>::max)();

    bool mAdjustViewBounds = false;
    bool mCropToPadding = false;

    int mImageAlpha = 255;

    DrawTransform mDrawTransform;
    // False means "draw the drawable where it stands". Kept separate from a
    // conveniently-identity DrawTransform so the fast path in onDraw can skip the
    // canvas save entirely, as AOSP's null mDrawMatrix check does.
    bool mHasDrawTransform = false;

    // AOSP's mHaveFrame. configureBounds divides by the content box, so it cannot
    // run before there is one; without this a pre-layout call would compute a
    // transform against a zero-sized view and cache it.
    bool mHaveFrame = false;
    bool mBoundsDirty = true;
};

} // namespace widget
} // namespace setu

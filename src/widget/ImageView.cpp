#include "ImageView.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "../graphics/Bitmap.h"
#include "../graphics/Canvas.h"
#include "../graphics/drawable/BitmapDrawable.h"
#include "../ui/AndroidAttrs.h"
#include "../ui/DrawableInflater.h"
#include "../ui/TypedArray.h"
#include "../utils/Logger.h"

namespace setu {
namespace widget {

namespace {

constexpr const char* kTag = "ImageView";

// mMaxWidth/mMaxHeight when android:maxWidth/maxHeight are absent. AOSP uses
// Integer.MAX_VALUE, and the value has to be a real int rather than a sentinel
// because resolveAdjustedSize min()s against it unconditionally.
constexpr int kUnbounded = (std::numeric_limits<int>::max)();

// Indices into the styleable array below. Named because obtainStyledAttributes
// answers positionally, and a silent off-by-one here reads android:maxWidth as
// android:adjustViewBounds.
enum {
    kAttrSrc = 0,
    kAttrScaleType,
    kAttrAdjustViewBounds,
    kAttrMaxWidth,
    kAttrMaxHeight,
    kAttrCropToPadding,
    kAttrCount,
};

std::string hex32(uint32_t value) {
    static const char* kDigits = "0123456789abcdef";
    std::string out = "0x";
    for (int shift = 28; shift >= 0; shift -= 4) {
        out += kDigits[(value >> shift) & 0xF];
    }
    return out;
}

} // namespace

ImageView::ImageView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser,
                     uint32_t defStyleAttr, uint32_t defStyleRes)
    : View(resManager, theme, parser, defStyleAttr, defStyleRes),
      mResManager(resManager),
      mTheme(theme) {
    if (!resManager) return;

    // Looked up by name rather than written as hex literals, per the note on
    // androidAttr(). An unresolvable name comes back as 0, which never matches a
    // real attribute, so a missing framework degrades to "no attributes present"
    // instead of reading whatever attribute 0 happens to collide with.
    std::vector<uint32_t> styleables((size_t)kAttrCount, 0);
    styleables[kAttrSrc] = androidAttr(resManager, "src");
    styleables[kAttrScaleType] = androidAttr(resManager, "scaleType");
    styleables[kAttrAdjustViewBounds] = androidAttr(resManager, "adjustViewBounds");
    styleables[kAttrMaxWidth] = androidAttr(resManager, "maxWidth");
    styleables[kAttrMaxHeight] = androidAttr(resManager, "maxHeight");
    styleables[kAttrCropToPadding] = androidAttr(resManager, "cropToPadding");

    TypedArray a(resManager, styleables);
    a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);

    if (a.hasValue(kAttrSrc)) {
        // getDrawable, so android:src accepts everything a real one does: a raster
        // via DrawableInflater's image path, a <selector>, a <shape>, or a bare
        // colour. The bitmap cases are the new ones, and the reason this widget is
        // worth having now.
        graphics::DrawablePtr src = a.getDrawable(kAttrSrc);
        if (src) {
            setImageDrawable(std::move(src));
        } else {
            Logger::w(kTag, "android:src could not be inflated");
        }
    }

    // AOSP's order, and it is load-bearing: setAdjustViewBounds(true) forces
    // FIT_CENTER, so reading scaleType afterwards is what lets an explicit
    // android:scaleType win over that. Swap the two and adjustViewBounds silently
    // overrides every authored scale type.
    setAdjustViewBounds(a.getBoolean(kAttrAdjustViewBounds, false));
    setMaxWidth(a.getDimensionPixelSize(kAttrMaxWidth, kUnbounded));
    setMaxHeight(a.getDimensionPixelSize(kAttrMaxHeight, kUnbounded));

    if (a.hasValue(kAttrScaleType)) {
        // The compiled ordinal is used as the enum directly - see the note on
        // ScaleType. Range-checked rather than trusted, because the value arrives
        // from a file this process did not produce.
        const int index = a.getInt(kAttrScaleType, -1);
        if (index >= (int)ScaleType::MATRIX && index <= (int)ScaleType::CENTER_INSIDE) {
            setScaleType((ScaleType)index);
            mScaleTypeSetFromXml = true;
        } else {
            Logger::w(kTag, "android:scaleType is " + std::to_string(index) +
                                ", not one of the eight known values; keeping the default");
        }
    }

    setCropToPadding(a.getBoolean(kAttrCropToPadding, false));
}

ImageView::ImageView() : view::View(nullptr, nullptr, nullptr, 0, 0) {}

ImageView::~ImageView() {
    // View's destructor only knows about the background, so the image's callback
    // has to be dropped here. It points at this object, and a drawable that
    // outlives the view - a shared <selector> from a theme, say - would otherwise
    // report an invalidation into freed memory.
    if (mDrawable) {
        unscheduleDrawable(mDrawable.get());
        mDrawable->setCallback(nullptr);
    }
}

void ImageView::setImageDrawable(graphics::DrawablePtr drawable) {
    if (mDrawable == drawable) return;

    const int oldWidth = mDrawableWidth;
    const int oldHeight = mDrawableHeight;

    updateDrawable(std::move(drawable));

    if (oldWidth != mDrawableWidth || oldHeight != mDrawableHeight) {
        requestLayout();
    }
    // AOSP gets away with relying on requestLayout, because there a layout pass is
    // guaranteed to follow and setFrame calls configureBounds. Nothing here
    // guarantees that, so the transform is marked stale and onDraw rebuilds it if
    // no layout arrives - the same lazy-recompute TextView::onDraw does with its
    // layout.
    mBoundsDirty = true;
    invalidate();
}

void ImageView::setImageResource(uint32_t resId) {
    if (!mResManager) {
        Logger::w(kTag, "setImageResource(" + hex32(resId) +
                            ") on an ImageView with no ResourceManager");
        return;
    }

    graphics::DrawablePtr drawable = DrawableInflater::inflate(mResManager, mTheme, resId);
    if (!drawable) {
        Logger::w(kTag, "could not inflate drawable " + hex32(resId));
    }
    // Installed either way, so a failed resolve clears the old image rather than
    // leaving a stale one behind. AOSP does the same.
    setImageDrawable(std::move(drawable));
}

void ImageView::setImageBitmap(std::shared_ptr<const graphics::Bitmap> bitmap) {
    setImageDrawable(std::make_shared<graphics::BitmapDrawable>(std::move(bitmap)));
}

void ImageView::updateDrawable(graphics::DrawablePtr drawable) {
    if (mDrawable) {
        unscheduleDrawable(mDrawable.get());
        mDrawable->setCallback(nullptr);
        mDrawable->setVisible(false, false);
    }

    mDrawable = std::move(drawable);

    if (!mDrawable) {
        mDrawableWidth = -1;
        mDrawableHeight = -1;
        mHasDrawTransform = false;
        return;
    }

    mDrawable->setCallback(this);
    mDrawable->setVisible(getVisibility() == VISIBLE, true);

    // Immediately, not on the first touch: an empty state set reads as "none of
    // these are set" to the matcher, so a <selector> whose first item is
    // state_enabled="false" would render its disabled image until something
    // happened to this view. Same reasoning as View::setBackground.
    if (mDrawable->isStateful()) {
        mDrawable->setState(getDrawableState());
    }

    // Only when it has actually been asked for. AOSP gates the equivalent push on
    // mColorMod, which is set by setImageAlpha and nothing else; pushing 255
    // unconditionally would silently overwrite an alpha the drawable was inflated
    // with - <bitmap android:alpha="0.5"> being the obvious casualty.
    if (mImageAlpha != 255) {
        mDrawable->setAlpha(mImageAlpha);
    }

    mDrawableWidth = mDrawable->getIntrinsicWidth();
    mDrawableHeight = mDrawable->getIntrinsicHeight();
}

void ImageView::setScaleType(ScaleType scaleType) {
    if (mScaleType == scaleType) return;
    mScaleType = scaleType;

    if (scaleType == ScaleType::MATRIX) {
        // Reported rather than silently ignored, but it is not a defect worth
        // fixing here: MATRIX draws through the matrix set by setImageMatrix, which
        // does not exist in this runtime because there is no Matrix class and
        // Canvas has no concat(). AOSP's own mMatrix starts out identity, so the
        // only MATRIX state actually reachable from XML is the identity - which is
        // exactly what an absent transform produces.
        Logger::d(kTag, "scaleType=matrix draws untransformed; there is no image "
                        "matrix to apply in this runtime");
    }

    requestLayout();
    mBoundsDirty = true;
    invalidate();
}

void ImageView::setAdjustViewBounds(bool adjustViewBounds) {
    mAdjustViewBounds = adjustViewBounds;
    // Not a default but a coupling, and AOSP's own: onMeasure derives the adjusted
    // box straight from the intrinsic aspect ratio, so any scale type that scales
    // the axes independently would then re-fit the image inside a box already
    // shaped for it.
    if (adjustViewBounds) {
        setScaleType(ScaleType::FIT_CENTER);
    }
}

void ImageView::setMaxWidth(int maxWidth) {
    // No requestLayout, matching AOSP. The maxima only take effect through
    // onMeasure, and the two are set back-to-back during inflation before any
    // measure pass has run.
    mMaxWidth = maxWidth;
}

void ImageView::setMaxHeight(int maxHeight) {
    mMaxHeight = maxHeight;
}

void ImageView::setCropToPadding(bool cropToPadding) {
    if (mCropToPadding == cropToPadding) return;
    mCropToPadding = cropToPadding;
    requestLayout();
    invalidate();
}

void ImageView::setImageAlpha(int alpha) {
    // Clamped, where AOSP's deprecated setAlpha(int) masks with 0xFF. Masking is an
    // artifact of that overload's signature, and it turns 256 into fully
    // transparent - which is not what any caller means.
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mImageAlpha == alpha) return;
    mImageAlpha = alpha;
    if (mDrawable) {
        mDrawable->setAlpha(alpha);
        invalidate();
    }
}

int ImageView::suggestedMinimumWidth() const {
    // AOSP's View.getSuggestedMinimumWidth, which this View does not expose. The
    // background's own minimum counts, which is what keeps a styled ImageButton
    // with no android:src from measuring to nothing.
    if (!mBackground) return mMinWidth;
    return (std::max)(mMinWidth, mBackground->getMinimumWidth());
}

int ImageView::suggestedMinimumHeight() const {
    if (!mBackground) return mMinHeight;
    return (std::max)(mMinHeight, mBackground->getMinimumHeight());
}

int ImageView::resolveAdjustedSize(int desiredSize, int maxSize, int measureSpec) const {
    switch (getMode(measureSpec)) {
        case MEASURE_SPEC_UNSPECIFIED:
            // The parent will take anything, so the only ceiling is the one this
            // view imposed on itself. This is the case View::resolveSize gets
            // wrong for an ImageView: it would return the full desired size and
            // ignore android:maxWidth entirely.
            return (std::min)(desiredSize, maxSize);
        case MEASURE_SPEC_AT_MOST:
            return (std::min)((std::min)(desiredSize, getSize(measureSpec)), maxSize);
        case MEASURE_SPEC_EXACTLY:
            return getSize(measureSpec);
        default:
            return desiredSize;
    }
}

void ImageView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int w;
    int h;

    // Aspect ratio of the content box this view would like, or 0 for "no
    // preference". Only ever non-zero while adjustViewBounds is on.
    float desiredAspect = 0.0f;

    bool resizeWidth = false;
    bool resizeHeight = false;

    const int widthSpecMode = getMode(widthMeasureSpec);
    const int heightSpecMode = getMode(heightMeasureSpec);

    if (!mDrawable) {
        mDrawableWidth = -1;
        mDrawableHeight = -1;
        w = h = 0;
    } else {
        w = mDrawableWidth;
        h = mDrawableHeight;
        // A drawable with no intrinsic size counts as 1x1 rather than 0x0, so a
        // ColorDrawable src still leaves an aspect ratio to divide by below.
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;

        if (mAdjustViewBounds) {
            // An EXACTLY spec is the parent's decision and is not negotiable; the
            // aspect ratio can only be honoured along an axis left free.
            resizeWidth = widthSpecMode != MEASURE_SPEC_EXACTLY;
            resizeHeight = heightSpecMode != MEASURE_SPEC_EXACTLY;
            desiredAspect = (float)w / (float)h;
        }
    }

    const int pleft = mPaddingLeft;
    const int pright = mPaddingRight;
    const int ptop = mPaddingTop;
    const int pbottom = mPaddingBottom;

    int widthSize;
    int heightSize;

    if (resizeWidth || resizeHeight) {
        // The largest this view may be on each axis, before the aspect ratio gets a
        // say.
        widthSize = resolveAdjustedSize(w + pleft + pright, mMaxWidth, widthMeasureSpec);
        heightSize = resolveAdjustedSize(h + ptop + pbottom, mMaxHeight, heightMeasureSpec);

        if (desiredAspect != 0.0f) {
            const float actualAspect = (float)(widthSize - pleft - pright) /
                                       (float)(heightSize - ptop - pbottom);

            if (std::fabs(actualAspect - desiredAspect) > 0.0000001f) {
                bool done = false;

                // Narrow the view to match the height it has been given.
                if (resizeWidth) {
                    int newWidth =
                        (int)(desiredAspect * (float)(heightSize - ptop - pbottom)) + pleft + pright;

                    // When the height is fixed, the width is allowed to exceed the
                    // estimate above: that estimate was the image's own width, and
                    // a taller-than-natural box legitimately needs a wider one.
                    // AOSP gates this on sCompatAdjustViewBounds, which is only set
                    // for targetSdk < 18 - so this is the modern branch, taken
                    // unconditionally here.
                    if (!resizeHeight) {
                        widthSize = resolveAdjustedSize(newWidth, mMaxWidth, widthMeasureSpec);
                    }

                    if (newWidth <= widthSize) {
                        widthSize = newWidth;
                        done = true;
                    }
                }

                // Otherwise shorten the view to match the width it has been given.
                if (!done && resizeHeight) {
                    int newHeight =
                        (int)((float)(widthSize - pleft - pright) / desiredAspect) + ptop + pbottom;

                    if (!resizeWidth) {
                        heightSize = resolveAdjustedSize(newHeight, mMaxHeight, heightMeasureSpec);
                    }

                    if (newHeight <= heightSize) {
                        heightSize = newHeight;
                    }
                }
            }
        }
    } else {
        // Either the aspect ratio does not matter or both axes are already fixed.
        // Measure to the image's own size, which is the whole point of this class:
        // a wrap_content ImageView is as big as its drawable.
        w += pleft + pright;
        h += ptop + pbottom;

        w = (std::max)(w, suggestedMinimumWidth());
        h = (std::max)(h, suggestedMinimumHeight());

        widthSize = resolveSize(w, widthMeasureSpec);
        heightSize = resolveSize(h, heightMeasureSpec);
    }

    setMeasuredDimension(widthSize, heightSize);
}

void ImageView::onLayout(bool changed, int l, int t, int r, int b) {
    view::View::onLayout(changed, l, t, r, b);

    // AOSP does this from setFrame, which there is the one hook guaranteed to run
    // on every layout pass whether the geometry moved or not. View::layout calls
    // onLayout unconditionally, so this is the equivalent place.
    mHaveFrame = true;
    mBoundsDirty = true;
    configureBounds();
}

void ImageView::configureBounds() {
    if (!mDrawable || !mHaveFrame) return;
    mBoundsDirty = false;

    const int dwidth = mDrawableWidth;
    const int dheight = mDrawableHeight;

    const int vwidth = getWidth() - mPaddingLeft - mPaddingRight;
    const int vheight = getHeight() - mPaddingTop - mPaddingBottom;

    if (dwidth <= 0 || dheight <= 0 || mScaleType == ScaleType::FIT_XY) {
        // No intrinsic size to preserve, or an explicit instruction to fill. Either
        // way the drawable is handed the whole content box and asked to cover it,
        // and there is nothing left for a transform to do.
        //
        // This is also the one path where the *drawable* does the stretching rather
        // than the canvas, and for a nine-patch that distinction is the whole
        // point: given real bounds it stretches only its stretchable regions, where
        // a canvas scale would smear its corners too.
        mDrawable->setBounds(0, 0, vwidth, vheight);
        mHasDrawTransform = false;
        return;
    }

    // Every remaining scale type draws the image at its native size and moves the
    // canvas instead.
    mDrawable->setBounds(0, 0, dwidth, dheight);

    if (mScaleType == ScaleType::MATRIX) {
        // The identity, for the reason given in setScaleType.
        mHasDrawTransform = false;
        return;
    }

    // AOSP writes this as (dwidth < 0 || vwidth == dwidth) && ...; the negative
    // halves are unreachable past the early return above.
    if (vwidth == dwidth && vheight == dheight) {
        mHasDrawTransform = false;
        return;
    }

    const float dw = (float)dwidth;
    const float dh = (float)dheight;
    const float vw = (float)vwidth;
    const float vh = (float)vheight;

    DrawTransform t;

    switch (mScaleType) {
        case ScaleType::CENTER:
            // Native size, centred. Rounded to whole pixels, as AOSP does, so an
            // odd leftover does not land the image on a half-pixel and resample it.
            t.transX = (float)std::lround((vw - dw) * 0.5f);
            t.transY = (float)std::lround((vh - dh) * 0.5f);
            break;

        case ScaleType::CENTER_CROP: {
            // Uniform scale by the *larger* ratio, so the content box is fully
            // covered and the excess falls outside it - the standard treatment for
            // a photo in a fixed frame.
            float scale;
            float dx = 0.0f;
            float dy = 0.0f;

            // Cross-multiplied to compare dw/dh against vw/vh without dividing.
            // Widened to 64-bit, where AOSP multiplies ints: a camera-resolution
            // image against a 4K view is within a factor of a few hundred of
            // overflowing, and a wrapped product would pick the wrong axis.
            if ((long long)dwidth * vheight > (long long)vwidth * dheight) {
                scale = vh / dh;
                dx = (vw - dw * scale) * 0.5f;
            } else {
                scale = vw / dw;
                dy = (vh - dh * scale) * 0.5f;
            }

            t.scaleX = t.scaleY = scale;
            t.transX = (float)std::lround(dx);
            t.transY = (float)std::lround(dy);
            break;
        }

        case ScaleType::CENTER_INSIDE: {
            // Like FIT_CENTER, except it never scales *up*: an icon smaller than
            // the view stays at its native size instead of being enlarged.
            float scale;
            if (dwidth <= vwidth && dheight <= vheight) {
                scale = 1.0f;
            } else {
                scale = (std::min)(vw / dw, vh / dh);
            }

            t.scaleX = t.scaleY = scale;
            t.transX = (float)std::lround((vw - dw * scale) * 0.5f);
            t.transY = (float)std::lround((vh - dh * scale) * 0.5f);
            break;
        }

        case ScaleType::FIT_START:
        case ScaleType::FIT_CENTER:
        case ScaleType::FIT_END:
        default: {
            // Matrix.setRectToRect(src=(0,0,dw,dh), dst=(0,0,vw,vh), fit), which is
            // what AOSP reaches for these three. Uniform scale by the smaller ratio
            // so the whole image fits, then the slack on the one axis that has any
            // is distributed according to the alignment. Not rounded, matching
            // Skia - setRectToRect works in floats throughout.
            const float scale = (std::min)(vw / dw, vh / dh);
            t.scaleX = t.scaleY = scale;

            if (mScaleType == ScaleType::FIT_CENTER) {
                t.transX = (vw - dw * scale) * 0.5f;
                t.transY = (vh - dh * scale) * 0.5f;
            } else if (mScaleType == ScaleType::FIT_END) {
                t.transX = vw - dw * scale;
                t.transY = vh - dh * scale;
            }
            // FIT_START leaves both at zero.
            break;
        }
    }

    mDrawTransform = t;
    mHasDrawTransform = true;
}

void ImageView::onDraw(graphics::Canvas& canvas) {
    view::View::onDraw(canvas);

    if (!mDrawable) return;

    // Cheap when clean. Needed because setImageDrawable can land after layout, and
    // nothing here promises a layout pass will follow a requestLayout.
    if (mBoundsDirty) configureBounds();

    // AOSP's exact test, and note it is == 0 rather than <= 0: a drawable with no
    // intrinsic size reports -1, and that case was given the whole content box by
    // configureBounds and does have something to draw.
    if (mDrawableWidth == 0 || mDrawableHeight == 0) return;

    if (!mHasDrawTransform && mPaddingLeft == 0 && mPaddingTop == 0) {
        // Nothing to set up, so no save/restore either. Worth the branch: this is
        // the common case for a FIT_XY background-style image and for any image
        // whose intrinsic size already matches its view.
        mDrawable->draw(canvas);
        return;
    }

    canvas.save();

    if (mCropToPadding) {
        // Off by default in AOSP, which means a CENTER_CROP image is normally
        // allowed to spill across this view's own padding. View::draw has already
        // clipped to the view's bounds; this narrows that to the padding box.
        canvas.clipRect((float)mPaddingLeft, (float)mPaddingTop,
                        (float)(getWidth() - mPaddingRight),
                        (float)(getHeight() - mPaddingBottom));
    }

    // AOSP is translate(padding) then concat(mDrawMatrix), where mDrawMatrix scales
    // about the origin and then translates. The three calls below compose to the
    // same thing, but only in this order: Direct2DCanvas multiplies each new
    // transform on the *left* of the current one, and it uses row vectors
    // (p' = p * M), so the call issued last is the one applied first to a point.
    // Reading upwards from the drawable, then, a source pixel is scaled, offset by
    // the scale type's translation, and finally offset by the padding - which is
    // exactly AOSP's order. Reverse these and the padding gets multiplied by the
    // scale.
    canvas.translate((float)mPaddingLeft, (float)mPaddingTop);

    if (mHasDrawTransform) {
        canvas.translate(mDrawTransform.transX, mDrawTransform.transY);
        canvas.scale(mDrawTransform.scaleX, mDrawTransform.scaleY);
    }

    mDrawable->draw(canvas);

    canvas.restore();
}

void ImageView::invalidateDrawable(graphics::Drawable* who) {
    if (!mDrawable || who != mDrawable.get()) {
        view::View::invalidateDrawable(who);
        return;
    }

    // The intrinsic size is not fixed for the lifetime of a drawable:
    // BitmapDrawable::setTargetDensity and NinePatchDrawable::setTargetDensity both
    // change it and then invalidate. The transform is derived from it, so this is
    // where it gets rechecked.
    const int width = mDrawable->getIntrinsicWidth();
    const int height = mDrawable->getIntrinsicHeight();
    if (width != mDrawableWidth || height != mDrawableHeight) {
        mDrawableWidth = width;
        mDrawableHeight = height;
        mBoundsDirty = true;
        configureBounds();
    }

    // The whole view, not the drawable's rect: working out where the image
    // actually landed means running its bounds back through the transform and the
    // padding, and AOSP concluded that was not worth it either.
    invalidate();
}

void ImageView::drawableStateChanged() {
    view::View::drawableStateChanged();

    if (mDrawable && mDrawable->isStateful()) {
        if (mDrawable->setState(getDrawableState())) {
            invalidate();
        }
    }
}

} // namespace widget
} // namespace setu

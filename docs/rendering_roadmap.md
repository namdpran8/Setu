# Rendering Roadmap

Durable record of the rendering/drawable phase plan for Windroid, kept in-repo so
it survives independent of any chat session. Update this file when the plan
changes; do not let it drift.

**Last updated:** 2026-08-24

---

## Status at a glance

- **Phase 4 (ColorStateList + colour-only tinting): COMPLETE, but uncompiled and
  uncommitted as of 2026-08-24.** All source is on disk and internally
  consistent; it has not yet been compiled against the current tree (which is
  mid-refactor with a large outstanding error count being worked through by
  hand), and the code changes are not yet committed. This doc is committed ahead
  of the code deliberately, at the maintainer's request.
- **`android:backgroundTint` is explicitly OUT of Phase 4** and is not scheduled
  into any later phase here. It was a deliberate scope decision, not an
  oversight. (Note: `android:tint` on a drawable and `android:backgroundTint` on
  a View are different attributes; only the View-level tint is the one ruled
  out.)

---

## Confirmed phase order

```
Phase 4  [DONE, uncompiled]  →  Phase 2 V1.5  →  Phase 5  →  Phase 6  →  Item 9
```

This ordering is confirmed. What follows is the detail for each entry, marked by
how well-grounded it is: **[confirmed]** decisions, **[from code]** scope derived
from TODO/comment anchors in the tree, and **[reconfirm]** items whose detailed
scope lives only in chat history and should be re-agreed before work starts.

---

## Phase 4 — ColorStateList + colour-only tinting  **[DONE, uncompiled]**

The `android.content.res.ColorStateList` value type and the plumbing that lets a
colour follow a view's drawable state.

Delivered:

- `graphics::ColorStateList` value type — `src/graphics/ColorStateList.{h,cpp}`.
  Deliberately **not** a `Drawable` and **not** under `graphics/drawable/`.
- Resource inflation — `src/ui/ColorStateListInflater.{h,cpp}` (a
  `res/color/*.xml` `<selector>`) and the shared state-set parser
  `src/ui/StateSetInflater.{h,cpp}` (moved out of `DrawableInflater`).
- Attribute access — `XmlAttrs::getColorStateList` and `TypedArray::getColorStateList`.
- Consumers wired: `<solid>` and `<stroke>` colour selectors in
  `GradientDrawable`, and `android:textColor` in `TextView`.
- `mTextColorSetFromXml` guard in `EditText` so its default text colour cannot
  wipe an XML-authored colour selector (follows the existing
  `mGravitySetFromXml` precedent).
- All three new `.cpp`s registered in `setu_runtime` only (the
  `constraint_layout_test` target compiles none of the units that include
  `ColorStateList.h`).

Remaining for this phase: **compile and commit.** Both are the maintainer's to
do; per standing rule this project is never built by the assistant.

---

## Phase 2 V1.5 — finish the `<shape>` / GradientDrawable surface  **[NEXT]**

Phase 2 built the background-drawable pipeline: `GradientDrawable` paints a
`<shape>` and `DrawableInflater` reads one out of an APK (see the header note in
`src/ui/DrawableInflater.h`, which calls itself "the other half of Phase 2").
V1.5 is the next increment of that same surface.

**[reconfirm]** The exact line-item list for V1.5 is not captured outside chat.
Re-agree scope before starting. The concrete deferrals the current code itself
points at — the natural candidate scope — are:

- **OVAL / LINE / RING shapes.** `GradientDrawable::draw` currently falls back to
  a rectangle for every non-rectangle shape (`GradientDrawable.cpp`, `default:`
  branch).
- **Ring geometry** — `innerRadius` / `innerRadiusRatio` / `thickness` /
  `thicknessRatio`, read but not yet honoured (`DrawableInflater::inflateShape`).
- **Real gradients.** `<gradient>` is currently approximated by averaging its
  stops into one flat colour, pending a shader on `Paint`
  (`DrawableInflater::inflateShape`, `meanColor` / `meanColor3`).
- **Dashed strokes.** `dashWidth` / `dashGap` are parsed and stored but draw
  solid, because `Paint` has no dash support yet (`GradientDrawable.h` members).

Each of the above is a `[from code]` anchor, not a locked commitment. Confirm
which land in V1.5 versus later.

---

## Phase 5 — RippleDrawable  **[from code]**

`<ripple>` and `<animated-selector>` support. Confirmed by
`DrawableInflater.cpp` `phaseFor()`: these root elements are recognised and
logged as "Phase 5 (RippleDrawable)" rather than silently dropped.

---

## Phase 6 — bitmap pipeline  **[from code]**

Decoding and drawing raster drawables: `<bitmap>`, `<nine-patch>`, and the raw
image formats (`.png`, `.9.png`, `.webp`, `.jpg`). Needs a decoder and
`Canvas::drawBitmap`. Confirmed by `DrawableInflater.cpp`: `phaseFor()` maps
`bitmap`/`nine-patch` to "Phase 6 (bitmap pipeline)", and `inflate()` logs
"Skipping bitmap drawable (Phase 6)" for non-`.xml` drawable paths.

Retires the hand-built flat-colour selector standing in for `btn_default`'s
nine-patch assets in `Button.cpp` (`fade()` / `darken()` / `makeDefaultBackground`).

---

## Item 9  **[reconfirm]**

The terminal roadmap entry in the confirmed order. Its detailed scope is **not**
recorded outside chat history and should be re-established before it comes up.
Do not infer its contents from the Phase 4 build-order (which also happened to
have nine items — a separate, now-completed list).

---

## Not on this roadmap

Recognised by the inflater but deliberately unscheduled here (logged, not
dropped, so a missing background in a real app names its status):

- Vector drawables (`<vector>`, `<animated-vector>`).
- Drawable containers (`<layer-list>`, `<level-list>`, `<transition>`).
- `android:backgroundTint` (View-level tint) — see the status note above.

---

## Where the ground truth lives

When this doc and the code disagree, the code wins — then fix this doc. The
load-bearing anchors:

- `src/ui/DrawableInflater.cpp` — `phaseFor()` maps unsupported root elements to
  the phase that will fix them. The single best index of what belongs where.
- `src/ui/DrawableInflater.h` — header note framing the inflater as "the other
  half of Phase 2".
- `src/graphics/drawable/GradientDrawable.{h,cpp}` — inline notes on every
  deferred `<shape>` feature.

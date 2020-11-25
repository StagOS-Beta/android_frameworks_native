/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <optional>

#include "SkColorMatrix.h"
#include "SkRuntimeEffect.h"
#include "SkShader.h"
#include "ui/GraphicTypes.h"

namespace android {
namespace renderengine {
namespace skia {

/**
 * Arguments for creating an effect that applies color transformations in linear XYZ space.
 * A linear effect is decomposed into the following steps when operating on an image:
 * 1. Electrical-Optical Transfer Function (EOTF) maps the input RGB signal into the intended
 * relative display brightness of the scene in nits for each RGB channel
 * 2. Transformation matrix from linear RGB brightness to linear XYZ, to operate on display
 * luminance.
 * 3. Opto-Optical Transfer Function (OOTF) applies a "rendering intent". This can include tone
 * mapping to display SDR content alongside HDR content, or any number of subjective transformations
 * 4. Transformation matrix from linear XYZ back to linear RGB brightness.
 * 5. Opto-Electronic Transfer Function (OETF) maps the display brightness of the scene back to
 * output RGB colors.
 *
 * For further reading, consult the recommendation in ITU-R BT.2390-4:
 * https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-4-2018-PDF-E.pdf
 *
 * Skia normally attempts to do its own simple tone mapping, i.e., the working color space is
 * intended to be the output surface. However, Skia does not support complex tone mapping such as
 * polynomial interpolation. As such, this filter assumes that tone mapping has not yet been applied
 * to the source colors. so that the tone mapping process is only applied once by this effect. Tone
 * mapping is applied when presenting HDR content (content with HLG or PQ transfer functions)
 * alongside other content, whereby maximum input luminance is mapped to maximum output luminance
 * and intermediate values are interpolated.
 */
struct LinearEffect {
    // Input dataspace of the source colors.
    const ui::Dataspace inputDataspace = ui::Dataspace::SRGB;

    // Working dataspace for the output surface, for conversion from linear space.
    const ui::Dataspace outputDataspace = ui::Dataspace::SRGB;

    // Sets whether alpha premultiplication must be undone.
    // This is required if the source colors use premultiplied alpha and is not opaque.
    const bool undoPremultipliedAlpha = false;
};

static inline bool operator==(const LinearEffect& lhs, const LinearEffect& rhs) {
    return lhs.inputDataspace == rhs.inputDataspace && lhs.outputDataspace == rhs.outputDataspace &&
            lhs.undoPremultipliedAlpha == rhs.undoPremultipliedAlpha;
}

struct LinearEffectHasher {
    // Inspired by art/runtime/class_linker.cc
    // Also this is what boost:hash_combine does
    static size_t HashCombine(size_t seed, size_t val) {
        return seed ^ (val + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
    size_t operator()(const LinearEffect& le) const {
        size_t result = std::hash<ui::Dataspace>{}(le.inputDataspace);
        result = HashCombine(result, std::hash<ui::Dataspace>{}(le.outputDataspace));
        return HashCombine(result, std::hash<bool>{}(le.undoPremultipliedAlpha));
    }
};

sk_sp<SkRuntimeEffect> buildRuntimeEffect(const LinearEffect& linearEffect);

// Generates a shader resulting from applying the a linear effect created from
// LinearEffectARgs::buildEffect to an inputShader. We also provide additional HDR metadata upon
// creating the shader:
// * The max display luminance is the max luminance of the physical display in nits
// * The max mastering luminance is provided as the max luminance from the SMPTE 2086
// standard.
// * The max content luminance is provided as the max light level from the CTA 861.3
// standard.
sk_sp<SkShader> createLinearEffectShader(sk_sp<SkShader> inputShader,
                                         const LinearEffect& linearEffect,
                                         sk_sp<SkRuntimeEffect> runtimeEffect,
                                         float maxDisplayLuminance, float maxMasteringLuminance,
                                         float maxContentLuminance);
} // namespace skia
} // namespace renderengine
} // namespace android
// ============================================================================
// PnordeStateLimits.h — Saved-state validation boundaries
// ============================================================================
// Protects Quick Programs and project loading against unexpected child counts or malformed values.
//
// Beginner notes:
//   - A header (.h) usually DECLARES names, types and helper functions.
//   - A source file (.cpp) usually IMPLEMENTS behaviour.
//   - `noexcept` promises an operation will not throw C++ exceptions.
//   - `const` generally indicates a value cannot be modified through that use.
//   - Avoid changing audio-thread behaviour merely to add documentation.
// See docs/SOURCE_GUIDE.md for terms, architecture and call-flow diagrams.
// ============================================================================

#pragma once

// ============================================================================
// BEGINNER GUIDE: Why does this file exist?
// ============================================================================
// Ableton saves PNORDE's knobs/dials/switches as XML "child" entries.
//
// Example (simplified):
//   <PARAMETERS>
//       <PARAM id="pn_key" value="0" />
//       <PARAM id="pn_scale" value="1" />
//   </PARAMETERS>
//
// The outer PARAMETERS tag is the parent. Each PARAM tag inside is a child.
// When we add a new automatable control, the saved state usually gains one
// more child. PNORDE v0.6.25 had 151 registered host parameters. Its old
// "160 children maximum" safety rule was too small for v0.6.26's 173.
// As a result, Quick Program snapshots failed validation and could not switch.
//
// These functions calculate limits from the REAL number of parameters in the
// running plugin. Adding new controls then automatically increases the normal
// limit. However, we KEEP absolute ceilings to reject suspicious or corrupt
// files rather than allowing unlimited XML data.
//
// This file is just simple C++ maths. It does not know about JUCE, Ableton,
// audio threads, or files; so we can compile and test it on its own.
// ============================================================================

namespace PnordeStateLimits
{
// Preserve acceptance of states that were valid under the old 160-child rule.
constexpr int historicalChildMinimum = 160;

// Extra space for a few migration nodes or future auxiliary state entries.
// This is NOT permission to accept an unlimited amount of unknown XML.
constexpr int spareChildNodes = 32;

// Absolute safety ceiling. Increasing this requires a deliberate review of
// XML depth, size, parsing, and the Program bank's four nested snapshots.
constexpr int absoluteChildMaximum = 384;

// The XML preflight counts opening AND closing tags, whereas JUCE's
// ValueTree::getNumChildren() counts each child only once. These ceilings must
// therefore grow TOGETHER. The extra 16 accounts for root/wrapper tags and a
// small number of future, structurally validated elements.
constexpr int tagOverhead = 16;

// For a normal 151-parameter build: 151 + 32 = 183 children are permitted.
// For a hypothetical 173-parameter build: 173 + 32 = 205 are permitted.
// IMPORTANT: this does not add parameters or change their existing IDs.
constexpr int allowedChildren(int registeredParameters) noexcept
{
    if (registeredParameters < 0)
        return historicalChildMinimum;

    // Prevent overflow on the addition by comparing BEFORE adding.
    if (registeredParameters >= absoluteChildMaximum - spareChildNodes)
        return absoluteChildMaximum;

    const int withSpare = registeredParameters + spareChildNodes;
    return withSpare < historicalChildMinimum ? historicalChildMinimum : withSpare;
}

// An invalid count is NEVER a valid state, even if the allowed limit is large.
constexpr bool acceptsChildren(int actualChildren, int registeredParameters) noexcept
{
    return actualChildren >= 0 && actualChildren <= allowedChildren(registeredParameters);
}

// An XML file needs up to two tags per PARAM: <PARAM ...> and </PARAM>.
// JUCE typically uses self-closing tags, but the safety calculation must not
// assume the serializer will always do that.
constexpr int allowedXmlTags(int registeredParameters) noexcept
{
    return 2 * allowedChildren(registeredParameters) + tagOverhead;
}

// This helps reveal a future architectural change that no longer fits the
// deliberate hard safety ceiling; it does not silently loosen that ceiling.
constexpr bool hasExpansionRoom(int registeredParameters) noexcept
{
    return registeredParameters >= 0 &&
        registeredParameters <= absoluteChildMaximum - spareChildNodes;
}

static_assert(allowedChildren(151) == 183, "Unexpected v0.6.25 state capacity");
static_assert(allowedChildren(173) == 205, "Future parameter growth must fit");
static_assert(allowedXmlTags(173) >= (2 * 173 + 2), "XML tag limit too small");
}

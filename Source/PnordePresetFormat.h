// ============================================================================
// PnordePresetFormat.h — PNORDE PresetFormat
// ============================================================================
// This helper isolates one part of the sequencer so it can be understood and tested without reading the whole plugin.
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
#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <cstring>

// Header-only format helper; all routines run on the JUCE message thread.
// New presets: 8-byte magic, little-endian payload size, FNV-1a integrity
// checksum, and the bounded native JUCE plugin-state payload. This is a
// corruption detector, not a signature or a trust guarantee.
namespace PnordePresetFormat
{
    constexpr size_t maxPayloadBytes = 4u * 1024u * 1024u;
    constexpr size_t headerBytes = 16u;
    constexpr size_t maxDiskBytes = maxPayloadBytes + headerBytes;
    constexpr std::array<juce::uint8, 8> magic {{
        'P', 'N', 'O', 'R', 'D', 'E', '1', '!'
    }};

    inline juce::uint32 fnv1a(const juce::uint8* bytes, size_t length) noexcept
    {
        juce::uint32 hash = 2166136261u;
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        return hash;
    }

    inline juce::uint32 read32(const juce::uint8* p) noexcept
    {
        return static_cast<juce::uint32>(p[0]) |
            (static_cast<juce::uint32>(p[1]) << 8) |
            (static_cast<juce::uint32>(p[2]) << 16) |
            (static_cast<juce::uint32>(p[3]) << 24);
    }
    inline void write32(juce::uint8* p, juce::uint32 value) noexcept
    {
        for (int i = 0; i < 4; ++i)
            p[i] = static_cast<juce::uint8>((value >> (i * 8)) & 0xffu);
    }

    inline bool encode(const juce::MemoryBlock& payload, juce::MemoryBlock& output)
    {
        const auto n = payload.getSize();
        if (n < 16u || n > maxPayloadBytes) return false;
        output.setSize(headerBytes + n, false);
        auto* dst = static_cast<juce::uint8*>(output.getData());
        std::memcpy(dst, magic.data(), magic.size());
        write32(dst + 8, static_cast<juce::uint32>(n));
        write32(dst + 12, fnv1a(static_cast<const juce::uint8*>(payload.getData()), n));
        std::memcpy(dst + headerBytes, payload.getData(), n);
        return true;
    }

    inline bool decode(const juce::MemoryBlock& stored, juce::MemoryBlock& payload)
    {
        const auto n = stored.getSize();
        if (n < 16u || n > maxDiskBytes) return false;
        const auto* src = static_cast<const juce::uint8*>(stored.getData());
        if (n >= headerBytes && std::memcmp(src, magic.data(), magic.size()) == 0)
        {
            const auto count = read32(src + 8);
            if (count < 16u || count > maxPayloadBytes ||
                static_cast<size_t>(count) + headerBytes != n ||
                fnv1a(src + headerBytes, count) != read32(src + 12))
                return false;
            payload.replaceAll(src + headerBytes, count);
            return true;
        }
        // Early v0.5.0 saved raw JUCE plugin state; accept those with the SAME
        // size/XML validation in the processor. Unknown formats are rejected.
        if (n > maxPayloadBytes) return false;
        payload = stored;
        return true;
    }

    inline bool readBounded(const juce::File& file, juce::MemoryBlock& output)
    {
        if (!file.existsAsFile() || !file.hasFileExtension(".pnorde")) return false;
        juce::FileInputStream input(file);
        if (!input.openedOk()) return false;
        const auto length = input.getTotalLength();
        if (length < 16 || length > static_cast<juce::int64>(maxDiskBytes)) return false;
        output.setSize(static_cast<size_t>(length), false);
        // Single bounded read; never call loadFileAsData on an untrusted file.
        return input.read(output.getData(), static_cast<int>(length)) == static_cast<int>(length);
    }

    inline bool writeAtomically(const juce::File& file, const juce::MemoryBlock& data)
    {
        if (!file.hasFileExtension(".pnorde") || data.getSize() > maxDiskBytes ||
            !file.getParentDirectory().isDirectory()) return false;
        juce::TemporaryFile staging(file);
        if (!staging.getFile().replaceWithData(data.getData(), data.getSize()))
            return false;
        return staging.overwriteTargetFileWithTemporary();
    }
}

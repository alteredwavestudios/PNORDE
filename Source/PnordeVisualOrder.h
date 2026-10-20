#pragma once
#include <array>

// POD permutation helper used ONLY by the message-thread display layout.
// It is independent of the real-time MIDI scheduler and allocates nothing.
struct PnordeVisualOrder final
{
    static constexpr int firstRow = 7;
    static constexpr int count = 14;
    using Rows = std::array<int, count>;

    static constexpr Rows identity() noexcept
    {
        Rows rows {};
        for (int i = 0; i < count; ++i) rows[i] = firstRow + i;
        return rows;
    }

    static bool valid(const Rows& rows) noexcept
    {
        unsigned seen = 0;
        for (const auto row : rows)
        {
            if (row < firstRow || row >= firstRow + count) return false;
            const auto bit = 1u << (row - firstRow);
            if (seen & bit) return false;
            seen |= bit;
        }
        return seen == ((1u << count) - 1u);
    }

    static bool move(Rows& rows, int fromRow, int toRow) noexcept
    {
        if (!valid(rows) || fromRow < firstRow || fromRow >= firstRow + count ||
            toRow < firstRow || toRow >= firstRow + count) return false;
        int from = -1, to = -1;
        for (int i = 0; i < count; ++i)
        {
            if (rows[i] == fromRow) from = i;
            if (rows[i] == toRow) to = i;
        }
        if (from < 0 || to < 0) return false;
        if (from == to) return true;
        const auto moving = rows[from];
        if (from < to)
            for (int i = from; i < to; ++i) rows[i] = rows[i + 1];
        else
            for (int i = from; i > to; --i) rows[i] = rows[i - 1];
        rows[to] = moving;
        return true;
    }

    // Row deletion compacts [removedRow..lastActiveRow], but MIDI remains
    // attached to the surviving physical rows. Rebase the UI permutation.
    static bool removeCompacted(Rows& rows, int removedRow, int lastActiveRow) noexcept
    {
        if (!valid(rows) || removedRow < firstRow ||
            lastActiveRow >= firstRow + count || lastActiveRow < removedRow)
            return false;
        for (auto& row : rows)
            row = row == removedRow ? lastActiveRow :
                (row > removedRow && row <= lastActiveRow ? row - 1 : row);
        return valid(rows);
    }
};

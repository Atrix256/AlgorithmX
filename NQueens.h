#pragma once

template <bool EXHAUSTIVE>
void NQueens(int boardSize)
{
    printf("===========================================\n");
    printf(__FUNCTION__ "(%i)\n", boardSize);
    printf("===========================================\n");

    // Set up the items
    auto solver = Solver<EXHAUSTIVE>::AddItems(boardSize + boardSize + (2 * boardSize - 1) + (2 * boardSize - 1), 2 * boardSize);
    {
        for (int i = 0; i < boardSize; ++i)
        {
            sprintf_s(solver.m_items[i].name, "X%i", i);
            sprintf_s(solver.m_items[boardSize + i].name, "Y%i", i);
        }

        for (int i = 0; i < 2 * boardSize - 1; ++i)
        {
            sprintf_s(solver.m_items[2 * boardSize + i].name, "DR%i", i);
            sprintf_s(solver.m_items[2 * boardSize + (2 * boardSize - 1) + i].name, "DL%i", i);
        }
    }

    const int c_beginX = 0;
    const int c_beginY = c_beginX + boardSize;
    const int c_beginDR = c_beginY + boardSize;
    const int c_beginDL = c_beginDR + 2 * boardSize - 1;

    // Set up the options
    for (int i = 0; i < boardSize * boardSize; ++i)
    {
        int x = i % boardSize;
        int y = i / boardSize;
        int dr = x + y;
        int dl = (boardSize - x - 1) + y;
        solver.AddOption({ c_beginX + x, c_beginY + y, c_beginDR + dr, c_beginDL + dl });
    }

    // Solve
    solver.Solve();
}
// TODO: give NROOKS the same treatment
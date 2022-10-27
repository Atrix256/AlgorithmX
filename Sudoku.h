#pragma once

inline void Sudoku()
{
    // This is the board to solve.
    // 0 means empty space.
    // From https://en.wikipedia.org/wiki/Sudoku
    // 30 numbers specified, 51 not specified
    static const int c_board[9 * 9] =
    {
        5,3,0,   0,7,0,   0,0,0,
        6,0,0,   1,9,5,   0,0,0,
        0,9,8,   0,0,0,   0,6,0,

        8,0,0,   0,6,0,   0,0,3,
        4,0,0,   8,0,3,   0,0,1,
        7,0,0,   0,2,0,   0,0,6,

        0,6,0,   0,0,0,   2,8,0,
        0,0,0,   4,1,9,   0,0,5,
        0,0,0,   0,8,0,   0,7,9
    };

    // The number of constraints on a sodoku board is...
    // A) 81 for cells   : The 9x9 grid must have a value in each location
    // B) 81 for rows    : Each of the 9 rows must have each of the 9 values in them
    // C) 81 for columns : Each of the 9 columns must have each of the 9 values in them
    // D) 81 for blocks  : Each of the 9 blocks must have each of the 9 values in them
    // A + B + C + D = 324
    // There is also an extra constraint for "initial state"
    // So, 325 in total
    static const int c_cellsBegin = 0;
    static const int c_rowsBegin = c_cellsBegin + 81;
    static const int c_colsBegin = c_rowsBegin + 81;
    static const int c_blocksBegin = c_colsBegin + 81;
    static const int c_initialState = c_blocksBegin + 81;
    static const int c_numItems = c_initialState + 1;

    // Create the solver
    auto solver = Solver<true>::AddItems(c_numItems);

    // Name the items
    {
        for (int i = 0; i < 81; ++i)
        {
            int x = i % 9;
            int y = i / 9;

            // Cell(x,y) has an item or not
            sprintf_s(solver.m_items[c_cellsBegin + i].name, "Cell%i%i", x, y);

            // Row(x) has item y or not
            sprintf_s(solver.m_items[c_rowsBegin + i].name, "Row%i_%i", x, y);

            // Col(x) has item y or not
            sprintf_s(solver.m_items[c_colsBegin + i].name, "Col%i_%i", x, y);

            // Block(x) has item y or not
            sprintf_s(solver.m_items[c_blocksBegin + i].name, "Blck%i_%i", x, y);
        }

        // Initial state
        sprintf_s(solver.m_items[c_initialState].name, "Init");
    }

    // Make the initial state option
    // This is the only row which has the initial state item covered, so will always be part of the solution.
    {
        std::vector<int> initialState;
        for (int cell = 0; cell < 9 * 9; ++cell)
        {
            if (c_board[cell] == 0)
                continue;

            // This cell has something in it
            initialState.push_back(c_cellsBegin + cell);

            // This row has this value in it
            // Note: we subtract 1 from the values, because 0 is invalid.
            int cellY = cell / 9;
            initialState.push_back(c_rowsBegin + (cellY) * 9 + c_board[cell] - 1);

            // This column has this value in it
            int cellX = cell % 9;
            initialState.push_back(c_colsBegin + (cellX) * 9 + c_board[cell] - 1);

            // This block has this value in it
            int blockX = cellX / 3;
            int blockY = cellY / 3;
            int block = blockY * 3 + blockX;
            initialState.push_back(c_blocksBegin + (block) * 9 + c_board[cell] - 1);
        }

        // Add that this is the initial state
        initialState.push_back(c_initialState);

        // Add this option
        solver.AddOption(initialState);
    }

    // TODO: make options for each 0

    int ijkl = 0;
}
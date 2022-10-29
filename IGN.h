#pragma once

inline void IGN()
{
    printf("===========================================\n");
    printf(__FUNCTION__ "\n");
    printf("===========================================\n");

    // The goal here is to find a 9x9 sudoku solution where not only
    // does every row, column and 3x3 block have all values 1-9,
    // but also every overlapping 3x3 block, not just the major ones.
    // There is no initial state to this sudoku. We are looking for
    // any sudoku solution that satisfies this!

    // The number of constraints are...
    // A)  81 for cells   : The 9x9 grid must have a value in each location
    // B)  81 for rows    : Each of the 9 rows must have each of the 9 values in them
    // C)  81 for columns : Each of the 9 columns must have each of the 9 values in them
    // D) 729 for blocks  : Every cell has a 3x3 block surrounding it, so 81 blocks, that must each have of the 9 values in them.
    // A + B + C + D = 972
    const int c_cellsBegin = 0;
    const int c_rowsBegin = c_cellsBegin + 81;
    const int c_colsBegin = c_rowsBegin + 81;
    const int c_blocksBegin = c_colsBegin + 81;
    const int c_numItems = c_blocksBegin + 729;

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
        }

        for (int i = 0; i < 729; ++i)
        {
            int block = i / 9;
            int value = i % 9;

            // Block(block) has item 'value' or not
            sprintf_s(solver.m_items[c_blocksBegin + i].name, "Blk%i_%i", block, value);
        }
    }

    // Make the 9 options for each spot on the board
    {
        // A lambda to calculate the item index for adding a value to plus shape
        auto BlockItemIndex = [=](int cell, int offsetX, int offsetY, int value)
        {
            const int c_gridSize = 9;
            const int c_numValues = 9;

            int x = cell % c_gridSize;
            int y = cell / c_gridSize;
            x = (x + offsetX + c_gridSize) % c_gridSize;
            y = (y + offsetY + c_gridSize) % c_gridSize;
            int plusIndex = y * c_gridSize + x;
            return c_blocksBegin + plusIndex * c_numValues + value;
        };

        int option[12];
        for (int cell = 0; cell < 9 * 9; ++cell)
        {
            option[0] = c_cellsBegin + cell;

            int cellX = cell % 9;
            int cellY = cell / 9;
            int blockX = cellX / 3;
            int blockY = cellY / 3;
            int block = blockY * 3 + blockX;

            for (int value = 0; value < 9; ++value)
            {
                option[1] = c_rowsBegin + (cellY) * 9 + value;
                option[2] = c_colsBegin + (cellX) * 9 + value;

                for (int offset = 0; offset < 9; ++offset)
                {
                    int offsetX = (offset % 3) - 1;
                    int offsetY = (offset / 3) - 1;
                    option[3 + offset] = BlockItemIndex(cell, offsetX, offsetY, value);
                }

                solver.AddOption(option);
            }
        }
    }

    // Solve and print out the solution
    int solutionCount = 0;
    std::vector<int> solvedBoard(81);
    solver.Solve(
        [&] (const auto& solver)
        {
            solutionCount++;
            printf("Solution #%i...", solutionCount);

            for (int optionIndex : solver.m_solutionOptionNodeIndices)
            {
                // Find the spacer node for this option
                int spacerNode = optionIndex;
                while (solver.m_nodes[spacerNode].itemIndex != -1)
                    spacerNode--;

                // get the cell and value
                int cell = solver.m_nodes[spacerNode + 1].itemIndex - c_cellsBegin;
                int cellY = cell / 9;
                int itemIndex = solver.m_nodes[spacerNode + 2].itemIndex;
                int value = 1 + itemIndex - (c_rowsBegin + (cellY) * 9);

                // set it
                solvedBoard[cell] = value;
            }

            for (int cell = 0; cell < 81; ++cell)
            {
                if (cell > 0 && cell % 27 == 0)
                    printf("\n\n");
                else if (cell % 9 == 0)
                    printf("\n");
                else if (cell % 3 == 0)
                    printf(" ");

                printf("%i ", solvedBoard[cell]);
            }

            printf("\n\n");
        }
    );
}
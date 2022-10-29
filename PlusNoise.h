#pragma once

inline void PlusNoise()
{
    printf("===========================================\n");
    printf(__FUNCTION__ "\n");
    printf("===========================================\n");

    // The goal here is to find a 5x5 matrix of numbers such that each
    // + shape of numbers contains 0,1,2,3,4, even overlapping + shapes.
    // Constraint items:
    //  25: each of the 25 cells must have a value in it
    // 125: each of the 25 plus shapes must have values 0, 1, 2, 3, 4 in them.
    // 150 items total.
    // Note: a plus shape's index is identified by the cell of its center
    // value.

    const int c_gridSize = 5;  // For 5x5
    const int c_numValues = 5; // For 0,1,2,3,4

    const int c_numCells = c_gridSize * c_gridSize;

    const int c_beginCells = 0;
    const int c_beginPluses = c_beginCells + c_numCells;
    const int c_numItems = c_beginPluses + c_numCells * c_numValues;

    // Set up the items
    auto solver = Solver<true>::AddItems(c_numItems);
    {
        for (int i = 0; i < c_numCells; ++i)
            sprintf_s(solver.m_items[c_beginCells + i].name, "C%i%i", i % c_gridSize, i / c_gridSize);

        for (int i = 0; i < c_numCells * c_numValues; ++i)
        {
            int plusIndex = i / c_numValues;
            int value = i % c_numValues;

            sprintf_s(solver.m_items[c_beginPluses + i].name, "P%i%i", plusIndex, value);
        }
    }

    // Set up the options. There is one option per value per cell, so 45 options total.
    // Each option specifies what cell it is in, and also, what value it is putting into each
    // of the 5 Plus shapes it occupies.
    {
        // A lambda to calculate the item index for adding a value to plus shape
        auto PlusValueItemIndex = [=](int cell, int offsetX, int offsetY, int value)
        {
            int x = cell % c_gridSize;
            int y = cell / c_gridSize;
            x = (x + offsetX + c_gridSize) % c_gridSize;
            y = (y + offsetY + c_gridSize) % c_gridSize;
            int plusIndex = y * c_gridSize + x;
            return c_beginPluses + plusIndex * c_numValues + value;
        };

        int option[6];
        for (int i = 0; i < c_numCells * c_numValues; ++i)
        {
            // Calculate cell and value
            int cell = i / c_numValues;
            int value = i % c_numValues;

            // What cell is covered
            option[0] = c_beginCells + cell;

            // Put this value into each of the plus shapes
            option[1] = PlusValueItemIndex(cell,  0,  0, value);
            option[2] = PlusValueItemIndex(cell, -1,  0, value);
            option[3] = PlusValueItemIndex(cell,  1,  0, value);
            option[4] = PlusValueItemIndex(cell,  0, -1, value);
            option[5] = PlusValueItemIndex(cell,  0,  1, value);

            // Add the option
            solver.AddOption(option);
        }
    }

    // Solve and show solutions
    int solutionCount = 0;
    solver.Solve([&](const auto& solver)
        {
            if (solutionCount >= 4)
                return;

            solutionCount++;
            printf("Solution #%i...", solutionCount);

            // Fill out the result
            std::vector<int> solution(c_numCells);
            for (int optionNodeIndex : solver.m_solutionOptionNodeIndices)
            {
                int spacerIndex = optionNodeIndex;
                while (solver.m_nodes[spacerIndex].itemIndex != -1)
                    spacerIndex--;

                int optionIndex = (spacerIndex - solver.m_rootItemIndex) / 7;
                int cell = optionIndex / c_numValues;
                int value = optionIndex % c_numValues;

                solution[cell] = value;
            }

            // print the result
            for (int cell = 0; cell < c_numCells; ++cell)
            {
                if (cell % c_gridSize == 0)
                    printf("\n");

                printf("%i", solution[cell]);
            }

            printf("\n\n");
        }
    );

}

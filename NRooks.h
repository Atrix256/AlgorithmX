#pragma once

template <bool EXHAUSTIVE>
void NRooks(int boardSize)
{
    printf("===========================================\n");
    printf(__FUNCTION__ "(%i)\n", boardSize);
    printf("===========================================\n");

    // Set up the items
    auto solver = Solver<EXHAUSTIVE>::AddItems(boardSize + boardSize);
    {
        for (int i = 0; i < boardSize; ++i)
        {
            sprintf_s(solver.m_items[i].name, "X%i", i);
            sprintf_s(solver.m_items[boardSize + i].name, "Y%i", i);
        }
    }

    const int c_beginX = 0;
    const int c_beginY = c_beginX + boardSize;

    // Set up the options
    for (int i = 0; i < boardSize * boardSize; ++i)
    {
        int x = i % boardSize;
        int y = i / boardSize;
        solver.AddOption({ c_beginX + x, c_beginY + y});
    }

    // Solve
    int solutionCount = 0;
    solver.Solve([&] (const auto& solver)
        {
            if (solutionCount > 5)
                return;

            solutionCount++;
            printf("Solution #%i...", solutionCount);

            // Fill out the board
            std::vector<char> solution(boardSize * boardSize, '.');
            for (int optionIndex : solver.m_solutionOptionNodeIndices)
            {
                int spacerIndex = optionIndex;
                while (solver.m_nodes[spacerIndex].itemIndex != -1)
                    spacerIndex--;

                int optionIndex = (spacerIndex - (boardSize + boardSize)) / 3;

                solution[optionIndex] = 'R';
            }

            // print the board
            for (int cell = 0; cell < boardSize * boardSize; ++cell)
            {
                if (cell % boardSize == 0)
                    printf("\n");

                printf("%c", solution[cell]);
            }

            printf("\n\n");
        }
    );
}
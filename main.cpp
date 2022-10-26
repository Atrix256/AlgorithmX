// Indices are used instead of pointers because they don't invalidate when dynamic arrays resize.
// Knuth also notes that you can use types that are smaller than a pointer type when they are indices.
// I didn't do things quite as bare metal efficient as Knuth, so check out his code if you want to squeeze out more perf!
// For instance, his implementation does not use recursive functions, while mine does.

#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#define DETERMINISTIC() false

std::mt19937 GetRNG()
{
#if DETERMINISTIC()
    std::mt19937 rng;
#else
    std::random_device rd;
    std::mt19937 rng(rd());
#endif
    return rng;
}

// An item is something to be covered
struct Item
{
    char name[8];
    int leftItemIndex = -1;
    int rightItemIndex = -1;
    int optionCount = -1;
};

// An option is a sequential list of nodes.
struct Node
{
    // Spacer nodes use upNodeIndex as the index of the previous spacer node, and downNodeIndex as the next spacer node.
    // Non spacer nodes use these to get to the next option for the current item
    int upNodeIndex = -1;
    int downNodeIndex = -1;

    // What item a node belongs to.
    // -1 means it is a spacer node.
    // There is a spacer node before and after every option.
    // An option is a sequential list of nodes.
    int itemIndex = -1;
};

template <bool EXHAUSTIVE>
class Solver
{
public:

    // Comma seperated list
    static Solver AddItems(const char* itemNames, int firstOptionalItem = -1)
    {
        // Parse and add the items
        Solver ret;
        if (itemNames)
        {
            const char* start = itemNames;
            while (true)
            {
                const char* end = strchr(start, ',');
                if (!end)
                    end = &start[strlen(start)];

                Item newItem;
                if (end - start >= _countof(newItem.name))
                {
                    printf("item %i name is too long, max length is %i\n", (int)ret.m_items.size(), (int)_countof(newItem.name) - 1);
                    ret.m_error = true;
                    return ret;
                }

                memcpy(newItem.name, start, (end - start));
                newItem.name[end - start] = 0;
                ret.m_items.push_back(newItem);

                if (end[0] == 0)
                    break;

                start = &end[1];
            }
        }

        // Having no items is an error case
        if (ret.m_items.size() == 0)
        {
            printf("No items given\n");
            ret.m_error = true;
            return ret;
        }

        // Add a root node item to the end
        ret.m_rootItemIndex = (int)ret.m_items.size();
        ret.m_items.resize(ret.m_items.size() + 1);
        ret.m_items[ret.m_rootItemIndex].name[0] = 0;
        if (firstOptionalItem < 0)
            ret.m_firstOptionalItem = ret.m_rootItemIndex;
        else
            ret.m_firstOptionalItem = std::min(ret.m_rootItemIndex, firstOptionalItem);

        // make the doubly linked list of items
        for (int index = 0; index < (int)ret.m_items.size(); ++index)
        {
            ret.m_items[index].leftItemIndex = int((index + ret.m_items.size() - 1) % ret.m_items.size());
            ret.m_items[index].rightItemIndex = int((index + 1) % ret.m_items.size());
        }

        // Make a node for each item except the root node
        ret.m_nodes.resize(ret.m_items.size() - 1);
        for (int index = 0; index < (int)ret.m_nodes.size(); ++index)
        {
            ret.m_nodes[index].upNodeIndex = index;
            ret.m_nodes[index].downNodeIndex = index;
            ret.m_nodes[index].itemIndex = index;
        }

        return ret;
    }

    // Comma seperated list
    Solver& AddOption(const char* items)
    {
        if (m_error || !items || !items[0])
            return *this;

        // Add a spacer node
        {
            int newNodeIndex = (int)m_nodes.size();
            m_nodes.resize(m_nodes.size() + 1);
            Node& newNode = m_nodes[newNodeIndex];
            newNode.itemIndex = -1;
        }

        // Add the nodes for this option
        const char* start = items;
        while (true)
        {
            const char* end = strchr(start, ',');
            if (!end)
                end = &start[strlen(start)];

            bool foundItem = false;
            for (int itemIndex = 0; itemIndex < m_items.size() - 1; ++itemIndex)
            {
                Item& item = m_items[itemIndex];
                if ((strlen(item.name) == end - start) && (memcmp(item.name, start, end - start) == 0))
                {
                    // Make a new node
                    foundItem = true;
                    int newNodeIndex = (int)m_nodes.size();
                    m_nodes.resize(m_nodes.size() + 1);
                    Node& newNode = m_nodes[newNodeIndex];
                    newNode.itemIndex = itemIndex;

                    // hook it into the doubly linked list for the item.  Put it at the end of the list
                    Node& itemNode = m_nodes[itemIndex];
                    newNode.upNodeIndex = itemNode.upNodeIndex;
                    newNode.downNodeIndex = itemIndex;
                    m_nodes[newNode.upNodeIndex].downNodeIndex = newNodeIndex;
                    m_nodes[newNode.downNodeIndex].upNodeIndex = newNodeIndex;
                }
                if (foundItem)
                    break;
            }

            if (!foundItem)
            {
                printf("Could not find item in option\n");
                m_error = true;
                return *this;
            }

            if (end[0] == 0)
                break;

            start = &end[1];
        }

        return *this;
    }

    void Solve()
    {
        if (m_error)
        {
            printf("There was an error, not running solver.\n");
            return;
        }

        if(EXHAUSTIVE)
            m_rng = GetRNG();

        // Precalculations to help the solver
        SetOptionPointers();
        CountItemOptions();

        // Solve!
        m_start = std::chrono::high_resolution_clock::now();
        SolveInternal();

        // report how long the solve took
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(now - m_start);
        std::string elapsed = MakeDurationString((float)timeSpan.count());
        printf("Total time = %s\n\n", elapsed.c_str());
    }

private:
    std::vector<Item> m_items;
    std::vector<Node> m_nodes;
    int m_rootItemIndex = -1;
    int m_firstOptionalItem = -1;
    bool m_error = false;
    std::vector<int> m_solutionOptionNodeIndices;
    std::mt19937 m_rng;
    int m_solutionsFound = 0;
    std::chrono::high_resolution_clock::time_point m_start;

    std::string MakeDurationString(float durationInSeconds)
    {
        std::string ret;

        static const float c_oneMinute = 60.0f;
        static const float c_oneHour = c_oneMinute * 60.0f;

        int hours = int(durationInSeconds / c_oneHour);
        durationInSeconds -= float(hours) * c_oneHour;

        int minutes = int(durationInSeconds / c_oneMinute);
        durationInSeconds -= float(minutes) * c_oneMinute;

        int seconds = int(durationInSeconds);

        char buffer[1024];
        if (hours < 10)
            sprintf_s(buffer, "0%i:", hours);
        else
            sprintf_s(buffer, "%i:", hours);
        ret = buffer;

        if (minutes < 10)
            sprintf_s(buffer, "0%i:", minutes);
        else
            sprintf_s(buffer, "%i:", minutes);
        ret += buffer;

        if (seconds < 10)
            sprintf_s(buffer, "0%i", seconds);
        else
            sprintf_s(buffer, "%i", seconds);
        ret += buffer;

        return ret;
    }

    void CoverItem(int itemIndex)
    {
        // Remove this item from the item list
        m_items[m_items[itemIndex].leftItemIndex].rightItemIndex = m_items[itemIndex].rightItemIndex;
        m_items[m_items[itemIndex].rightItemIndex].leftItemIndex = m_items[itemIndex].leftItemIndex;

        // Remove all options of this item from the lists of the other items
        int optionNodeIndex = m_nodes[itemIndex].downNodeIndex;
        while (optionNodeIndex != itemIndex)
        {
            int nodeIndex = optionNodeIndex + 1;
            while (nodeIndex != optionNodeIndex)
            {
                // if we reached the end of the list, wrap around
                if (m_nodes[nodeIndex].itemIndex == -1)
                {
                    nodeIndex = m_nodes[nodeIndex].upNodeIndex + 1;
                    continue;
                }

                // Remove the option from this item's list
                m_nodes[m_nodes[nodeIndex].upNodeIndex].downNodeIndex = m_nodes[nodeIndex].downNodeIndex;
                m_nodes[m_nodes[nodeIndex].downNodeIndex].upNodeIndex = m_nodes[nodeIndex].upNodeIndex;

                // Remember that an option has been removed
                m_items[m_nodes[nodeIndex].itemIndex].optionCount--;

                // go to the next node in the option
                nodeIndex++;
            }

            // go to the next option
            optionNodeIndex = m_nodes[optionNodeIndex].downNodeIndex;
        }
    }

    void UncoverItem(int itemIndex)
    {
        // Add this item back to the list
        m_items[m_items[itemIndex].leftItemIndex].rightItemIndex = itemIndex;
        m_items[m_items[itemIndex].rightItemIndex].leftItemIndex = itemIndex;

        // Add all options of this item back to the lists of the other items
        int optionNodeIndex = m_nodes[itemIndex].downNodeIndex;
        while (optionNodeIndex != itemIndex)
        {
            // Start just beyond this node, and go through all the other nodes until we reach this one again
            int nodeIndex = optionNodeIndex + 1;
            while (nodeIndex != optionNodeIndex)
            {
                // if we reached the end of the list, wrap around to the beginning again
                if (m_nodes[nodeIndex].itemIndex == -1)
                {
                    nodeIndex = m_nodes[nodeIndex].upNodeIndex + 1;
                    continue;
                }

                // Add the option back into this item's list
                m_nodes[m_nodes[nodeIndex].upNodeIndex].downNodeIndex = nodeIndex;
                m_nodes[m_nodes[nodeIndex].downNodeIndex].upNodeIndex = nodeIndex;

                // Remember that an option has been restored
                m_items[m_nodes[nodeIndex].itemIndex].optionCount++;

                // go to the next node in the option
                nodeIndex++;
            }

            // go to the next option
            optionNodeIndex = m_nodes[optionNodeIndex].downNodeIndex;
        }
    }

    void PrintSolution()
    {
        printf("Solution #%i...\n", m_solutionsFound);

        // Show the options in a deterministic order - the same order they were given
        std::vector<int> solutionOptionNodeIndices = m_solutionOptionNodeIndices;
        std::sort(solutionOptionNodeIndices.begin(), solutionOptionNodeIndices.end());

        // for each option
        for (int optionNodeIndex : solutionOptionNodeIndices)
        {
            // Get to the start of the option list
            int nodeIndex = optionNodeIndex;
            while (m_nodes[nodeIndex].itemIndex != -1)
                nodeIndex--;

            // for each item in that option
            nodeIndex++;
            while (m_nodes[nodeIndex].itemIndex != -1)
            {
                printf("%s ", m_items[m_nodes[nodeIndex].itemIndex].name);
                nodeIndex++;
            }
            printf("\n");
        }

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(now - m_start);
        std::string elapsed = MakeDurationString((float)timeSpan.count());
        printf("%s\n\n", elapsed.c_str());
    }

    void SolveInternal()
    {
        // For non exhaustive, return after finding the first solution
        if (!EXHAUSTIVE && m_solutionsFound > 0)
            return;

        // If we've found a solution, print it out
        if (m_items[m_rootItemIndex].rightItemIndex >= m_firstOptionalItem)
        {
            m_solutionsFound++;
            PrintSolution();
            return;
        }

        // If we are exhaustive, cover the left most item.
        // Otherwise, find the item with the lowest option count.
        // Randomizing ties might make for better results.
        int chosenItemIndex = m_items[m_rootItemIndex].rightItemIndex;
        if (!EXHAUSTIVE)
        {
            int itemIndex = m_items[m_rootItemIndex].rightItemIndex;
            int lowestItemIndex = itemIndex;
            int lowestItemCount = m_items[itemIndex].optionCount;

            itemIndex = m_items[itemIndex].rightItemIndex;
            while (itemIndex < m_firstOptionalItem)
            {
                if (m_items[itemIndex].optionCount < lowestItemCount)
                {
                    lowestItemCount = m_items[itemIndex].optionCount;
                    lowestItemIndex = itemIndex;
                }

                itemIndex = m_items[itemIndex].rightItemIndex;
            }

            chosenItemIndex = lowestItemIndex;
        }

        // Mark this item as covered.
        // We aren't sure which of the options we are going to use, but it will be one of the options
        CoverItem(chosenItemIndex);

        // If we are exhaustive, we can try options top to bottom. Otherwise we will try options in a randomized order.
        {
            auto TryOption = [&](int optionNodeIndex)
            {
                // Add this option onto our solution stack
                m_solutionOptionNodeIndices.push_back(optionNodeIndex);

                // Cover each item from this option, except the current item
                for (int nodeIndex = optionNodeIndex + 1; nodeIndex != optionNodeIndex; nodeIndex++)
                {
                    if (m_nodes[nodeIndex].itemIndex == -1)
                    {
                        nodeIndex = m_nodes[nodeIndex].upNodeIndex;
                        continue;
                    }

                    CoverItem(m_nodes[nodeIndex].itemIndex);
                }

                // Recurse
                SolveInternal();

                // Uncover each item from this option, except the current item
                for (int nodeIndex = optionNodeIndex + 1; nodeIndex != optionNodeIndex; nodeIndex++)
                {
                    if (m_nodes[nodeIndex].itemIndex == -1)
                    {
                        nodeIndex = m_nodes[nodeIndex].upNodeIndex;
                        continue;
                    }

                    UncoverItem(m_nodes[nodeIndex].itemIndex);
                }

                // remove this option from our solution stack
                m_solutionOptionNodeIndices.pop_back();
            };

            if (EXHAUSTIVE)
            {
                for (int optionNodeIndex = m_nodes[chosenItemIndex].downNodeIndex; optionNodeIndex != chosenItemIndex; optionNodeIndex = m_nodes[optionNodeIndex].downNodeIndex)
                    TryOption(optionNodeIndex);
            }
            else
            {
                std::vector<int> options;
                for (int optionNodeIndex = m_nodes[chosenItemIndex].downNodeIndex; optionNodeIndex != chosenItemIndex; optionNodeIndex = m_nodes[optionNodeIndex].downNodeIndex)
                    options.push_back(optionNodeIndex);

                std::shuffle(options.begin(), options.end(), m_rng);
                for (int optionNodeIndex : options)
                    TryOption(optionNodeIndex);
            }
        }

        // Uncover this item
        UncoverItem(chosenItemIndex);
    }

    void CountItemOptions()
    {
        for (int itemIndex = 0; itemIndex < m_items.size() - 1; ++itemIndex)
        {
            Item& item = m_items[itemIndex];
            item.optionCount = 0;
            Node* itemNode = &m_nodes[itemIndex];
            while (itemNode->downNodeIndex != itemIndex)
            {
                itemNode = &m_nodes[itemNode->downNodeIndex];
                item.optionCount++;
            }
        }
    }

    void SetOptionPointers()
    {
        // Add a node to the end to be part of the options doubly linked list.
        // This lets us simplify logic, knowing that spacer nodes are always at the start and end of every option.
        m_nodes.emplace_back();

        int lastOptionNodeIndex = int(m_items.size() - 1); // The first spacer node
        int nextOptionNodeIndex = lastOptionNodeIndex + 1; // The first node from the first option

        while (true)
        {
            while (nextOptionNodeIndex < m_nodes.size() && m_nodes[nextOptionNodeIndex].itemIndex != -1)
                nextOptionNodeIndex++;

            if (nextOptionNodeIndex == m_nodes.size())
                break;

            m_nodes[lastOptionNodeIndex].downNodeIndex = nextOptionNodeIndex;
            m_nodes[nextOptionNodeIndex].upNodeIndex = lastOptionNodeIndex;

            lastOptionNodeIndex = nextOptionNodeIndex;
            nextOptionNodeIndex++;
        }

        // Fix up the links of the first and last option to point to each other
        m_nodes[lastOptionNodeIndex].downNodeIndex = int(m_items.size() - 1);
        m_nodes[m_items.size() - 1].upNodeIndex = lastOptionNodeIndex;
    }
};

int main(int argc, char** argv)
{
    // From https://www-cs-faculty.stanford.edu/~knuth/programs/dlx1.w
    // 1 Unique Solution: AD, CEF, BG
    Solver<true>::AddItems("A,B,C,D,E,F,G", 5)
        .AddOption("C,E,F")
        .AddOption("A,D,G")
        .AddOption("B,C,F")
        .AddOption("A,D")
        .AddOption("B,G")
        .AddOption("D,E,G")
        .Solve();

    // From https://en.wikipedia.org/wiki/Exact_cover#Detailed_example
    // 1 Unique Solution: 14, 356, 27
    Solver<true>::AddItems("1,2,3,4,5,6,7")
        .AddOption("1,4,7")   // A
        .AddOption("1,4")     // B
        .AddOption("4,5,7")   // C
        .AddOption("3,5,6")   // D
        .AddOption("2,3,6,7") // E
        .AddOption("2,7")     // F
        .Solve();

    // Exact hitting set, transpose of last example. From https://en.wikipedia.org/wiki/Exact_cover#Exact_hitting_set
    Solver<true>::AddItems("A,B,C,D,E,F")
        .AddOption("A,B")     // 1
        .AddOption("E,F")     // 2
        .AddOption("D,E")     // 3
        .AddOption("A,B,C")   // 4
        .AddOption("C,D")     // 5
        .AddOption("D,E")     // 6
        .AddOption("A,C,E,F") // 7
        .Solve();

    return 0;
}

/*
TODO:
- report progress.
- pentominos
- sudoku
- N queens (and N rooks)
*/

/*
Algorithm description:

! explain item vs option

1) Choose an item.
 * We are doing left to right because we are going exhaustive.
 * Going from fewest to most choices is often better if not going exhaustive apparently.
 * Any way you go about it, so long as you try all items, you won't miss anything.
2) Remove this item from the list of items. (doesn't need to be undone later!)
3) For every option that has this item, remove that option from all items. These options are no longer available. One will be chosen as the one covering this item.
4) We will try each option one by one. Top to bottom is fine for exhausive. Random order is better if not exhausive apparently.
 a) for each item in the current option, except the last item chosen, mark that item as covered.
*/

/*
REFS:
* https://www-cs-faculty.stanford.edu/~knuth/programs.html
* https://www.youtube.com/watch?v=_cR9zDlvP88
* https://en.wikipedia.org/wiki/Knuth%27s_Algorithm_X
* https://en.wikipedia.org/wiki/Dancing_Links
* https://en.wikipedia.org/wiki/Exact_cover

More Algos: https://www-cs-faculty.stanford.edu/~knuth/programs.html
*/
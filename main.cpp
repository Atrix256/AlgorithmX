// Note: indices are used instead of pointers because they don't invalidate when dynamic arrays resize.
// Knuth also notes that you can use types that are smaller than a pointer type when they are indices.
// I didn't do things quite as bare metal efficient as Knuth, so check out his code if you want to squeeze out a little more perf!

#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>

#define DETERMINISTIC() true

std::mt19937& GetRNG()
{
#if DETERMINISTIC()
    static std::mt19937 rng;
#else
    static std::random_device rd;
    static std::mt19937 rng(rd());
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
    int upNodeIndex = -1;
    int downNodeIndex = -1;

    int itemIndex = -1; // What item it belongs to
};

class Solver
{
public:

    // Comma seperated list
    static Solver AddItems(const char* itemNames, int firstOptionalItem = -1)
    {
        Solver ret;

        ret.m_firstOptionalItem = firstOptionalItem >= 0 ? firstOptionalItem + 1 : -1;
        if (!itemNames || !itemNames[0])
            return ret;

        // Add a root node item
        ret.m_items.resize(1);
        ret.m_items[0].name[0] = 0;

        // Add the items
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
            ret.m_nodes[index].itemIndex = index + 1;
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
            for (int itemIndex = 1; itemIndex < m_items.size(); ++itemIndex)
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
                    Node& itemNode = m_nodes[itemIndex - 1];
                    newNode.upNodeIndex = itemNode.upNodeIndex;
                    newNode.downNodeIndex = itemIndex - 1;
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

        // Do some setup work to make solving faster and easier
        SetOptionPointers();
        CountItemOptions();

        // Solve!
        SolveInternal();
    }

private:
    std::vector<Item> m_items;
    std::vector<Node> m_nodes;
    int m_firstOptionalItem = -1;
    bool m_error = false;
    std::unordered_set<int> m_solutionOptionNodeIndices;

    void SolveInternal()
    {
        // We want to try choosing options for the columns with the lowest counts first.
        // So, let's get the list of item counts and sort by count.
        struct ItemCount
        {
            int itemIndex;
            int optionCount;
        };
        std::vector<ItemCount> itemCounts;
        {
            bool foundRequiredColumn = false;
            int itemIndex = 0;
            while (m_items[itemIndex].rightItemIndex != 0)
            {
                itemIndex = m_items[itemIndex].rightItemIndex;
                itemCounts.push_back({ itemIndex, m_items[itemIndex].optionCount });
                foundRequiredColumn = foundRequiredColumn || itemIndex < m_firstOptionalItem;
            }
            if (!foundRequiredColumn)
            {
                // TODO: print out solution! It's all the removed options
                printf("Found a solution!\n");
                for (int optionIndex : m_solutionOptionNodeIndices)
                    printf("%i\n", optionIndex);
                return;
            }
            std::sort(itemCounts.begin(), itemCounts.end(), [](const ItemCount& A, const ItemCount& B) { return A.optionCount < B.optionCount; });
        }

        // TODO: i think we only have to try the first item here? If so don't need to gather and sort. Just keep lowest one.
        std::vector<int> optionNodeIndices;
        for (int itemCountIndex = 0; itemCountIndex < itemCounts.size(); ++itemCountIndex)
        {
            int chosenItemIndex = itemCounts[itemCountIndex].itemIndex;

            // gather up all the options that cover this item
            optionNodeIndices.clear();
            int nodeIndex = chosenItemIndex - 1;
            while (m_nodes[nodeIndex].downNodeIndex != chosenItemIndex - 1)
            {
                nodeIndex = m_nodes[nodeIndex].downNodeIndex;

                int optionNodeIndex = nodeIndex;
                while (m_nodes[optionNodeIndex].itemIndex != -1)
                    optionNodeIndex--;

                optionNodeIndices.push_back(optionNodeIndex);
            }

            // Remove item
            m_items[m_items[chosenItemIndex].leftItemIndex].rightItemIndex = m_items[chosenItemIndex].rightItemIndex;
            m_items[m_items[chosenItemIndex].rightItemIndex].leftItemIndex = m_items[chosenItemIndex].leftItemIndex;

            // Try removing each option and recursing.
            // Try in a random order.
            std::mt19937& rng = GetRNG();
            std::shuffle(optionNodeIndices.begin(), optionNodeIndices.end(), rng);
            for (int optionIndex : optionNodeIndices)
            {
                // Remove option
                m_solutionOptionNodeIndices.insert(optionIndex);
                m_nodes[m_nodes[optionIndex].upNodeIndex].downNodeIndex = m_nodes[optionIndex].downNodeIndex;
                m_nodes[m_nodes[optionIndex].downNodeIndex].upNodeIndex = m_nodes[optionIndex].upNodeIndex;

                // TODO: remove options which have items that this option also has

                // Recurse
                SolveInternal();

                // TODO: restore options which have items that this option also has

                // Restore option
                m_solutionOptionNodeIndices.erase(optionIndex);
                m_nodes[m_nodes[optionIndex].upNodeIndex].downNodeIndex = optionIndex;
                m_nodes[m_nodes[optionIndex].downNodeIndex].upNodeIndex = optionIndex;
            }

            // Restore item
            m_items[m_items[chosenItemIndex].leftItemIndex].rightItemIndex = chosenItemIndex;
            m_items[m_items[chosenItemIndex].rightItemIndex].leftItemIndex = chosenItemIndex;
        }

        // TODO: keep track of how many things we have tried, and how many things there are to try total and report a percentage!
        // TODO: can this be multi threaded at all? I don't think so...
        // TODO: look at knuth's code. You have a lot of temporary storage!
    }

    void CountItemOptions()
    {
        for (int itemIndex = 1; itemIndex < m_items.size(); ++itemIndex)
        {
            Item& item = m_items[itemIndex];
            item.optionCount = 0;
            Node* itemNode = &m_nodes[itemIndex - 1];
            while (itemNode->downNodeIndex != itemIndex - 1)
            {
                itemNode = &m_nodes[itemNode->downNodeIndex];
                item.optionCount++;
            }
        }
    }

    void SetOptionPointers()
    {
        int lastOptionNodeIndex = int(m_items.size() - 1);
        int nextOptionNodeIndex = lastOptionNodeIndex + 1;

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
    Solver::AddItems("A,B,C,D,E,F,G", 5)
        .AddOption("C,E,F")
        .AddOption("A,D,G")
        .AddOption("B,C,F")
        .AddOption("A,D")
        .AddOption("B,G")
        .AddOption("D,E,G")
        .Solve();

    /*
    Options = A, B, C, D, E, F, G.  F& G are optional.
C E F
A D G
B C F
A D
B G
D E G

Unique solution =  A D and E F C and B G
    */

    return 0;
}

/*
REFS:
* https://www-cs-faculty.stanford.edu/~knuth/programs.html
* https://www.youtube.com/watch?v=_cR9zDlvP88
* https://en.wikipedia.org/wiki/Knuth%27s_Algorithm_X
* https://en.wikipedia.org/wiki/Dancing_Links
*/
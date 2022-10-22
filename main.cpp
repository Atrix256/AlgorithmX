#include <vector>

// Note: indices are used instead of pointers because they don't invalidate when dynamic arrays resize.
// Knuth also notes that you can use types that are smaller than a pointer type when they are indices.

// An item is something to be covered
struct Item
{
    char name[8];
    int leftItemIndex = -1;
    int rightItemIndex = -1;
};

// An option is a sequential list of nodes.
struct Node
{
    int upNodeIndex = -1;
    int downNodeIndex = -1;

    int itemIndex = -1; // What item it belongs to
};

struct Solver
{
    std::vector<Item> m_items;
    std::vector<Node> m_nodes;
    int m_firstOptionalItem = -1;
    bool m_error = false;

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
        if (!items || !items[0])
            return *this;

        // Add a spacer node
        {
            int newNodeIndex = (int)m_nodes.size();
            m_nodes.resize(m_nodes.size() + 1);
            Node& newNode = m_nodes[newNodeIndex];
            newNode.itemIndex = -1;
            // TODO: set the up and down to be the last and next option, respectively.
        }

        // TODO: doesn't every option need an extra node between it?

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
                    m_nodes.resize(m_nodes.size()+1);
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
                printf("Could not find item in option");
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
            return;

        // TODO: this!
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
TODO:
- rename to options and items
- support primary and secondary.  primary must be filled. secondary are optional to be filled but can only be filled once.

REFS:
* https://www-cs-faculty.stanford.edu/~knuth/programs.html
* https://www.youtube.com/watch?v=_cR9zDlvP88
*/
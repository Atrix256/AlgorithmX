#include <vector>

// An item is something to be covered
struct Item
{
    char name[8];
    Item* left = nullptr;
    Item* right = nullptr;
};

// An option is a sequential list of nodes.
struct Node
{
    Node* up = nullptr;
    Node* down = nullptr;

    Item* item = nullptr;
};

struct Solver
{
    std::vector<Item> m_items;
    std::vector<Node> m_options;
    int m_firstOptionalItem;

    void Clear()
    {
        m_items.clear();
        m_options.clear();
    }

    // Comma seperated list
    bool AddItems(const char* itemNames, int firstOptionalItem = -1)
    {
        m_firstOptionalItem = firstOptionalItem;
        if (!itemNames || !itemNames[0])
            return true;

        // Add the items
        int startIndex = 0;
        int endIndex = 0;
        while (true)
        {
            while (itemNames[endIndex] != ',' && itemNames[endIndex] != 0)
                endIndex++;

            Item newItem;
            if (endIndex - startIndex >= _countof(newItem.name))
            {
                printf("item %i name is too long, max length is %i\n", (int)m_items.size(), (int)_countof(newItem.name) - 1);
                Clear();
                return false;
            }

            memcpy(newItem.name, &itemNames[startIndex], (endIndex - startIndex));
            newItem.name[endIndex - startIndex] = 0;
            m_items.push_back(newItem);

            if (itemNames[endIndex] == 0)
                break;

            startIndex = endIndex + 1;
            endIndex = startIndex;
        }

        // make the doubly linked list
        for (size_t index = 0; index < m_items.size(); ++index)
        {
            m_items[index].left = &m_items[(index + m_items.size() - 1) % m_items.size()];
            m_items[index].right = &m_items[(index + 1) % m_items.size()];
        }

        return true;
    }

    // Comma seperated list
    bool AddOption(const char* options)
    {
        // TODO: could use strtok or something to tokenize these strings.

        return true;
    }
};

int main(int argc, char** argv)
{
    Solver solver;

    // TOOD: could have add functions return the solver object so you can do a . after each call
    // TODO: could have the solver have an internal bool if it is in an error state or not. Don't solve if in an error state.
    if (!solver.AddItems("A,B,C,D,E,F,G", 5) ||
        !solver.AddOption("C,E,F") ||
        !solver.AddOption("A,D,G") ||
        !solver.AddOption("B,C,F") ||
        !solver.AddOption("A,D") ||
        !solver.AddOption("B,G") ||
        !solver.AddOption("D,E,G"));
        return 1;

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
#include<iostream>
#include<vector>
using namespace std;

struct Node {
    int key;
    int degree;
    bool mark;
    Node* parent;
    Node* child;
    Node* left;
    Node* right;

    Node(int k) : key(k), degree(0), mark(false), parent(nullptr), child(nullptr) {
        left = right = this;
    }
};

class FibHeap {
public:
    Node* minimum;
    int n; // number of nodes

    FibHeap() : minimum(nullptr), n(0) {}

    // insert a new key into the heap and return pointer to the node
    Node* insert(int key) {
        Node* x = new Node(key);
        if (minimum == nullptr) {
            minimum = x;
        } else {
            // splice x into root list (to the left of minimum)
            x->left = minimum->left;
            x->right = minimum;
            minimum->left->right = x;
            minimum->left = x;
            if (x->key < minimum->key) minimum = x;
        }
        ++n;
        return x;
    }

    Node* get_min() const { return minimum; }

    // merge (meld) another heap into this one
    void merge(FibHeap& H) {
        if (H.minimum == nullptr) return;
        if (minimum == nullptr) {
            minimum = H.minimum;
            n = H.n;
            return;
        }
        // splice two circular lists: this->minimum and H.minimum
        Node* a = minimum->left;
        Node* b = H.minimum->left;
        a->right = H.minimum;
        H.minimum->left = a;
        b->right = minimum;
        minimum->left = b;
        if (H.minimum->key < minimum->key) minimum = H.minimum;
        n += H.n;
    }

    // remove node x from its circular doubly linked list (isolates x)
    static void remove_from_list(Node* x) {
        if (x->left == x && x->right == x) {
            // single node; nothing to link
            x->left = x->right = x;
            return;
        }
        x->left->right = x->right;
        x->right->left = x->left;
        x->left = x->right = x;
    }

    // make y a child of x (y becomes a child of x)
    void link(Node* y, Node* x) {
        // y should be a root in the root list before linking
        remove_from_list(y);
        // attach y to x's child circular list
        if (x->child == nullptr) {
            x->child = y;
            y->left = y->right = y;
        } else {
            y->left = x->child->left;
            y->right = x->child;
            x->child->left->right = y;
            x->child->left = y;
        }
        y->parent = x;
        x->degree += 1;
        y->mark = false;
    }

    void consolidate() {
        if (minimum == nullptr) return;
        // safe bound for degree; phi-based is tighter but log2 is simpler
        int D = std::floor(std::log2(std::max(1, n))) + 2;
        vector<Node*> A(D, nullptr);

        // snapshot root list because we'll mutate it
        vector<Node*> roots;
        Node* cur = minimum;
        if (cur) {
            do {
                roots.push_back(cur);
                cur = cur->right;
            } while (cur != minimum);
        }

        for (Node* w : roots) {
            Node* x = w;
            int d = x->degree;
            // ensure A can hold up to d
            while (d >= (int)A.size()) A.resize(A.size() * 2, nullptr);
            while (A[d] != nullptr) {
                Node* y = A[d];
                if (x->key > y->key) std::swap(x, y);
                link(y, x);
                A[d] = nullptr;
                ++d;
                while (d >= (int)A.size()) A.resize(A.size() * 2, nullptr);
            }
            A[d] = x;
        }

        // rebuild root list from A
        minimum = nullptr;
        for (Node* node : A) {
            if (node != nullptr) {
                // isolate node
                node->left = node->right = node;
                if (minimum == nullptr) {
                    minimum = node;
                } else {
                    // insert node into root list (to the left of minimum)
                    node->left = minimum->left;
                    node->right = minimum;
                    minimum->left->right = node;
                    minimum->left = node;
                    if (node->key < minimum->key) minimum = node;
                }
            }
        }
    }

    Node* extract_min() {
        Node* z = minimum;
        if (z != nullptr) {
            // 1) Move each child of z to the root list.
            if (z->child != nullptr) {
                // gather children first
                vector<Node*> children;
                Node* c = z->child;
                do {
                    children.push_back(c);
                    c = c->right;
                } while (c != z->child);

                // Insert each child into root list while z is still in the root list.
                for (Node* x : children) {
                    // detach x from child list
                    remove_from_list(x);
                    x->parent = nullptr;
                    x->mark = false;
                    // insert x into root list next to z
                    x->left = z->left;
                    x->right = z;
                    z->left->right = x;
                    z->left = x;
                }
                z->child = nullptr;
            }

            // 2) Remove z from root list
            if (z->right == z) {
                // z was the only node in root list
                minimum = nullptr;
            } else {
                // set a temporary minimum before removing z to keep a valid anchor
                minimum = z->right;
                remove_from_list(z);
                // 3) consolidate to ensure at most one root of each degree
                consolidate();
            }
            --n;
        }
        return z;
    }

    void cut(Node* x, Node* y) {
        // x is a child of y. Remove x from y's child list and make it a root.
        if (x->right == x) {
            // x was the only child
            y->child = nullptr;
        } else {
            if (y->child == x) y->child = x->right;
            remove_from_list(x);
        }
        y->degree -= 1;
        // add x to root list
        x->parent = nullptr;
        if (minimum == nullptr) {
            x->left = x->right = x;
            minimum = x;
        } else {
            x->left = minimum->left;
            x->right = minimum;
            minimum->left->right = x;
            minimum->left = x;
        }
        x->mark = false;
    }

    void cascading_cut(Node* y) {
        Node* z = y->parent;
        if (z != nullptr) {
            if (!y->mark) {
                y->mark = true;
            } else {
                cut(y, z);
                cascading_cut(z);
            }
        }
    }

    void decrease_key(Node* x, int k) {
        if (k > x->key) {
            cerr << "new key is greater than current key\n";
            return;
        }
        x->key = k;
        Node* y = x->parent;
        if (y != nullptr && x->key < y->key) {
            // cut x and move to root list
            cut(x, y);
            cascading_cut(y);
        }
        if (minimum == nullptr || x->key < minimum->key) minimum = x;
    }

    void delete_node(Node* x) {
        decrease_key(x, numeric_limits<int>::min());
        Node* removed = extract_min();
        if (removed) delete removed;
    }

    // helper for debugging: print root list keys
    void print_roots() const {
        if (minimum == nullptr) {
            cout << "<empty>\n";
            return;
        }
        Node* cur = minimum;
        cout << "Root list: ";
        do {
            cout << cur->key << "(deg=" << cur->degree << ") ";
            cur = cur->right;
        } while (cur != minimum);
        cout << "\n";
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FibHeap H;
    int n;
    if (!(cin >> n)) return 0;
    vector<int> vals(n);
    for (int i = 0; i < n; ++i) cin >> vals[i];

    vector<Node*> handles;
    for (int v : vals) handles.push_back(H.insert(v));

    cout << "After inserts:\n";
    H.print_roots();

    Node* m = H.get_min();
    if (m) cout << "Min = " << m->key << "\n";

    cout << "Extracting min...\n";
    Node* ex = H.extract_min();
    if (ex) {
        cout << "Extracted: " << ex->key << "\n";
        delete ex; // free node memory returned to caller
    }

    H.print_roots();
    if (H.get_min()) cout << "New min = " << H.get_min()->key << "\n";

    // demonstrate decrease-key on value 15 -> 1 if exists
    cout << "Decreasing key of node with original value 15 to 1...\n";
    for (Node* h : handles) {
        if (h->key == 15) {
            H.decrease_key(h, 1);
            break;
        }
    }
    H.print_roots();
    if (H.get_min()) cout << "Min after decrease-key = " << H.get_min()->key << "\n";

    cout << "Extracting all remaining keys: ";
    while (H.get_min() != nullptr) {
        Node* r = H.extract_min();
        if (r) {
            cout << r->key << " ";
            delete r;
        }
    }
    cout << "\nDone.\n";
    return 0;
}

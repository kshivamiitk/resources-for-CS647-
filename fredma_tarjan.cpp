#include <bits/stdc++.h>
using namespace std;

// ------------------- Edge structure -------------------
struct Edge {
    int u, v;
    double w;
};

// ------------------- Disjoint Set Union -------------------
struct DSU {
    vector<int> parent, rank;
    DSU(int n) {
        parent.resize(n);
        rank.assign(n, 0);
        iota(parent.begin(), parent.end(), 0);
    }
    int find(int x) {
        return parent[x] == x ? x : parent[x] = find(parent[x]);
    }
    bool unite(int x, int y) {
        x = find(x);
        y = find(y);
        if (x == y) return false;
        if (rank[x] < rank[y]) swap(x, y);
        parent[y] = x;
        if (rank[x] == rank[y]) rank[x]++;
        return true;
    }
};

// ------------------- Fibonacci Heap (minimal version) -------------------
struct FibNode {
    Edge edge; 
    double key;
    FibNode *parent, *child, *left, *right;
    int degree;
    bool mark;
    FibNode(const Edge &e) : edge(e), key(e.w), parent(nullptr), child(nullptr),
                             degree(0), mark(false) {
        left = right = this;
    }
};

struct FibHeap {
    FibNode* minNode;
    int nNodes;

    FibHeap() : minNode(nullptr), nNodes(0) {}

    void insertNode(FibNode* x) {
        if (!minNode) {
            minNode = x;
        } else {
            // insert x into root list
            x->left = minNode;
            x->right = minNode->right;
            minNode->right->left = x;
            minNode->right = x;
            if (x->key < minNode->key)
                minNode = x;
        }
        nNodes++;
    }

    void insertEdge(const Edge& e) {
        insertNode(new FibNode(e));
    }

    FibNode* extractMin() {
        FibNode* z = minNode;
        if (z) {
            // add children to root list
            if (z->child) {
                FibNode* c = z->child;
                do {
                    FibNode* next = c->right;
                    // remove c from child list
                    c->parent = nullptr;
                    // add to root list
                    c->left = minNode;
                    c->right = minNode->right;
                    minNode->right->left = c;
                    minNode->right = c;
                    c = next;
                } while (c != z->child);
            }
            // remove z from root list
            z->left->right = z->right;
            z->right->left = z->left;
            if (z == z->right)
                minNode = nullptr;
            else {
                minNode = z->right;
                consolidate();
            }
            nNodes--;
        }
        return z;
    }

    void consolidate() {
        int D = 64; // max degree
        vector<FibNode*> A(D, nullptr);
        vector<FibNode*> rootList;
        if (!minNode) return;
        FibNode* start = minNode;
        FibNode* w = start;
        do {
            rootList.push_back(w);
            w = w->right;
        } while (w != start);

        for (FibNode* w : rootList) {
            FibNode* x = w;
            int d = x->degree;
            while (A[d]) {
                FibNode* y = A[d];
                if (x->key > y->key) swap(x, y);
                fibHeapLink(y, x);
                A[d] = nullptr;
                d++;
            }
            A[d] = x;
        }
        minNode = nullptr;
        for (FibNode* a : A) {
            if (a) {
                if (!minNode) {
                    minNode = a->left = a->right = a;
                } else {
                    a->left = minNode;
                    a->right = minNode->right;
                    minNode->right->left = a;
                    minNode->right = a;
                    if (a->key < minNode->key)
                        minNode = a;
                }
            }
        }
    }

    void fibHeapLink(FibNode* y, FibNode* x) {
        // remove y from root list
        y->left->right = y->right;
        y->right->left = y->left;
        // make y child of x
        y->parent = x;
        if (!x->child) {
            x->child = y->left = y->right = y;
        } else {
            y->left = x->child;
            y->right = x->child->right;
            x->child->right->left = y;
            x->child->right = y;
        }
        x->degree++;
        y->mark = false;
    }

    void meld(FibHeap& other) {
        if (!other.minNode) return;
        if (!minNode) {
            minNode = other.minNode;
            nNodes = other.nNodes;
            return;
        }
        // concatenate root lists
        FibNode* min1Right = minNode->right;
        FibNode* min2Left = other.minNode->left;
        minNode->right = other.minNode;
        other.minNode->left = minNode;
        min1Right->left = min2Left;
        min2Left->right = min1Right;
        if (other.minNode->key < minNode->key)
            minNode = other.minNode;
        nNodes += other.nNodes;
    }

    bool empty() const {
        return nNodes == 0;
    }
};

// ------------------- Fredman-Tarjan MST -------------------
vector<Edge> fredmanTarjanMST(int n, const vector<Edge>& edges) {
    DSU dsu(n);
    // Build adjacency for each component’s heap
    vector<FibHeap> heaps(n);
    for (auto &e : edges) {
        int u = e.u, v = e.v;
        heaps[u].insertEdge(e);
        heaps[v].insertEdge({v,u,e.w}); // undirected
    }

    vector<Edge> mst;
    int components = n;
    while (components > 1) {
        // for each component, pick min edge
        for (int i = 0; i < n; i++) {
            int ci = dsu.find(i);
            // Skip empty heaps
            if (heaps[ci].empty()) continue;
            FibNode* node = heaps[ci].extractMin();
            if (!node) continue;
            Edge e = node->edge;
            int u = e.u, v = e.v;
            int cu = dsu.find(u), cv = dsu.find(v);
            if (cu == cv) continue; // already same component
            mst.push_back(e);
            // Union components
            if (dsu.unite(cu, cv)) {
                components--;
                int newRoot = dsu.find(cu);
                int oldRoot = (newRoot == cu ? cv : cu);
                heaps[newRoot].meld(heaps[oldRoot]);
            }
        }
    }
    return mst;
}

// ------------------- Driver -------------------
int main() {
    int n, m;
    cin >> n >> m;
    vector<Edge> edges(m);
    for (int i = 0; i < m; i++) {
        cin >> edges[i].u >> edges[i].v >> edges[i].w;
    }

    auto mst = fredmanTarjanMST(n, edges);
    double total = 0;
    for (auto &e : mst) total += e.w;

    cout << "MST weight: " << total << "\n";
    for (auto &e : mst) {
        cout << e.u << " - " << e.v << " : " << e.w << "\n";
    }
    return 0;
}

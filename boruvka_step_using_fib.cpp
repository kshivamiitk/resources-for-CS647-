#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct Edge {
    int u, v;
    ll w;
    Edge() {}
    Edge(int _u, int _v, ll _w): u(_u), v(_v), w(_w) {}
};

// ----------------- Minimal Fibonacci Heap (insert, extractMin, merge) -----------------
struct FibNode {
    int edgeIndex;    // index into global edge list
    ll key;           // weight
    FibNode *left, *right, *child, *parent;
    int degree;
    bool mark;
    FibNode(int ei, ll k) : edgeIndex(ei), key(k),
        left(this), right(this), child(nullptr), parent(nullptr),
        degree(0), mark(false) {}
};

struct FibHeap {
    FibNode *minNode;
    int n;

    FibHeap(): minNode(nullptr), n(0) {}

    bool empty() const { return minNode == nullptr; }

    void insertNode(FibNode *x) {
        if (!minNode) {
            minNode = x;
            x->left = x->right = x;
        } else {
            // insert x into root list (right of minNode)
            x->left = minNode;
            x->right = minNode->right;
            minNode->right->left = x;
            minNode->right = x;
            if (x->key < minNode->key) minNode = x;
        }
        ++n;
    }

    FibNode* makeNode(int edgeIndex, ll key) {
        return new FibNode(edgeIndex, key);
    }

    // merge heaps: splice root lists and update min and size
    static FibHeap* mergeHeaps(FibHeap* A, FibHeap* B) {
        if (!A) return B;
        if (!B) return A;
        // splice root lists
        FibNode* aRight = A->minNode->right;
        FibNode* bLeft  = B->minNode->left;

        A->minNode->right = B->minNode;
        B->minNode->left  = A->minNode;
        aRight->left = bLeft;
        bLeft->right = aRight;

        if (B->minNode->key < A->minNode->key) A->minNode = B->minNode;
        A->n += B->n;
        // delete B container but keep nodes
        delete B;
        return A;
    }

    // Helper: remove node x from root list (assumes x is in some list)
    void removeFromRootList(FibNode* x) {
        x->left->right = x->right;
        x->right->left = x->left;
    }

    // link y under x (y becomes child of x)
    void link(FibNode* y, FibNode* x) {
        removeFromRootList(y);
        // make y a child of x
        y->parent = x;
        if (!x->child) {
            x->child = y;
            y->left = y->right = y;
        } else {
            // insert y into child's root list
            y->left = x->child;
            y->right = x->child->right;
            x->child->right->left = y;
            x->child->right = y;
        }
        x->degree++;
        y->mark = false;
    }

    // Consolidate after extractMin
    void consolidate() {
        if (!minNode) return;
        int D = std::max(16, (int)(log2(max(1, n)) + 2)); // safe bound
        vector<FibNode*> A(D, nullptr);

        // gather root list nodes
        vector<FibNode*> roots;
        FibNode* cur = minNode;
        do {
            roots.push_back(cur);
            cur = cur->right;
        } while (cur != minNode);

        for (FibNode* w : roots) {
            FibNode* x = w;
            int d = x->degree;
            while (d >= (int)A.size()) A.resize(A.size()*2, nullptr);
            while (A[d] != nullptr) {
                FibNode* y = A[d];
                if (x->key > y->key) swap(x, y);
                link(y, x);
                A[d] = nullptr;
                ++d;
                if (d >= (int)A.size()) A.resize(A.size()*2, nullptr);
            }
            A[d] = x;
        }

        // rebuild root list and find new min
        minNode = nullptr;
        for (FibNode* node : A) {
            if (!node) continue;
            node->left = node->right = node;
            if (!minNode) {
                minNode = node;
            } else {
                // insert node into minNode's root list
                node->left = minNode;
                node->right = minNode->right;
                minNode->right->left = node;
                minNode->right = node;
                if (node->key < minNode->key) minNode = node;
            }
        }
    }

    // Extract min root and return it
    FibNode* extractMin() {
        FibNode* z = minNode;
        if (!z) return nullptr;

        // move children of z to root list
        if (z->child) {
            vector<FibNode*> children;
            FibNode* c = z->child;
            do {
                children.push_back(c);
                c = c->right;
            } while (c != z->child);

            for (FibNode* x : children) {
                x->parent = nullptr;
                // insert x into root list (right of minNode)
                x->left = minNode;
                x->right = minNode->right;
                minNode->right->left = x;
                minNode->right = x;
            }
        }

        // remove z from root list
        if (z->right == z) {
            minNode = nullptr;
        } else {
            removeFromRootList(z);
            minNode = z->right;
            consolidate();
        }
        --n;
        // detach z fields so it is a standalone node
        z->left = z->right = z;
        z->child = nullptr;
        z->parent = nullptr;
        z->degree = 0;
        z->mark = false;
        return z;
    }

    // insert an edge (index) with key weight
    void insertEdge(int edgeIndex, ll weight) {
        FibNode* node = makeNode(edgeIndex, weight);
        insertNode(node);
    }
};

// ----------------- DSU -----------------
struct DSU {
    int n;
    vector<int> parent, r;
    DSU(int n=0): n(n), parent(n), r(n,0) {
        for (int i=0;i<n;i++) parent[i]=i;
    }
    int find(int x){ return parent[x]==x?x:parent[x]=find(parent[x]); }
    int unite(int a,int b){
        a=find(a); b=find(b);
        if(a==b) return a;
        if(r[a]<r[b]) swap(a,b);
        parent[b]=a;
        if(r[a]==r[b]) r[a]++;
        return a; // return new root
    }
};

// ----------------- Boruvka Step Using FibHeap -----------------
// Performs one Boruvka phase: selects cheapest outgoing edge per component
// and unions components as chosen. Returns edges added in this step.
vector<int> boruvka_step_using_fibheap(
    int n,
    const vector<Edge>& edges,
    DSU &dsu,
    ll &added_weight,
    vector<pair<int,int>> &added_edges // pairs (edgeIndex, dummy)
) {
    added_weight = 0;
    added_edges.clear();

    // map each component root to its FibHeap pointer
    vector<FibHeap*> heaps(n, nullptr);

    // Build heaps: for each undirected edge (u,v), if they lie in different components,
    // insert edge index into both component heaps.
    for (int ei = 0; ei < (int)edges.size(); ++ei) {
        int u = edges[ei].u;
        int v = edges[ei].v;
        ll w = edges[ei].w;
        int cu = dsu.find(u);
        int cv = dsu.find(v);
        if (cu == cv) continue;
        if (!heaps[cu]) heaps[cu] = new FibHeap();
        if (!heaps[cv]) heaps[cv] = new FibHeap();
        heaps[cu]->insertEdge(ei, w);
        heaps[cv]->insertEdge(ei, w);
    }

    int comps = 0;
    vector<int> compRoots;
    compRoots.reserve(n);
    for (int i=0;i<n;i++){
        if (dsu.find(i)==i){
            comps++;
            compRoots.push_back(i);
        }
    }

    // For each component, find its cheapest outgoing edge (valid at time of extraction)
    vector<int> chosenEdge(n, -1); // chosen edge index per component root

    for (int root : compRoots) {
        FibHeap* H = heaps[root];
        if (!H) continue;
        while (!H->empty()) {
            FibNode* mn = H->extractMin();
            int ei = mn->edgeIndex;
            delete mn; // not needed any more
            int u = edges[ei].u, v = edges[ei].v;
            int cu = dsu.find(u), cv = dsu.find(v);
            // If edge becomes internal (both endpoints in same component), skip it
            if (cu == cv) continue;
            // assign chosen edge for component root (may be updated later if root changed)
            chosenEdge[root] = ei;
            break;
        }
    }

    // Collect edges chosen and union
    // Important: multiple components may pick the same edge; handle carefully
    vector<char> edgeTaken(edges.size(), false);
    for (int root : compRoots) {
        int ei = chosenEdge[root];
        if (ei == -1) continue;
        if (edgeTaken[ei]) continue; // already used by other component
        int u = edges[ei].u, v = edges[ei].v;
        int ru = dsu.find(u), rv = dsu.find(v);
        if (ru == rv) continue;
        // unite and mark edge taken
        int newRoot = dsu.unite(ru, rv);
        edgeTaken[ei] = true;
        added_edges.push_back({ei, 0});
        added_weight += edges[ei].w;

        // After union, merge heaps of old roots into new root so next phase can reuse them.
        FibHeap* H1 = heaps[ru];
        FibHeap* H2 = heaps[rv];
        if (ru == newRoot && rv == newRoot) {
            // both were same -- nothing
        } else {
            int keep = newRoot;
            int other = (keep == ru ? rv : ru);
            // if one heap is null, set heap to the other
            if (!heaps[keep]) heaps[keep] = heaps[other];
            else if (heaps[other]) heaps[keep] = FibHeap::mergeHeaps(heaps[keep], heaps[other]);
            heaps[other] = nullptr;
        }
    }

    // cleanup leftover heaps
    for (int i=0;i<n;i++){
        if (heaps[i]) {
            // free nodes are not explicitly freed here (for brevity) — production code should free heap nodes
            delete heaps[i];
            heaps[i] = nullptr;
        }
    }

    // Return vector of chosen edge indices per component (optional)
    // We return a list of edge indices that were chosen (edgeTaken true)
    vector<int> result;
    for (size_t i=0;i<edgeTaken.size();++i) if (edgeTaken[i]) result.push_back((int)i);
    return result;
}

// ----------------- Example usage -----------------
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    // Input: n m
    // then m lines: u v w  (1-indexed)
    if (!(cin >> n >> m)) return 0;
    vector<Edge> edges;
    edges.reserve(m);
    for (int i=0;i<m;i++){
        int u,v; ll w;
        cin >> u >> v >> w;
        --u; --v;
        edges.emplace_back(u,v,w);
    }

    DSU dsu(n);
    ll added_weight = 0;
    vector<pair<int,int>> added_edges_info;

    // Perform a single Boruvka step
    vector<int> edges_added = boruvka_step_using_fibheap(n, edges, dsu, added_weight, added_edges_info);

    cout << "Edges added in this Boruvka step: " << edges_added.size() << "\n";
    cout << "Total weight added: " << added_weight << "\n";
    for (int ei : edges_added) {
        cout << edges[ei].u + 1 << " " << edges[ei].v + 1 << " " << edges[ei].w << "\n";
    }

    return 0;
}
